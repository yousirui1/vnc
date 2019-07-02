#include "base.h"
#include "msg.h"
#include "queue.h"

static int sockfd = 0;
static struct sockaddr_in send_addr,recv_addr;
static int total_connections = 0;

/* h264 data queue */
unsigned char **vids_buf = NULL;
QUEUE *vids_queue = NULL;

rfb_request *request_ready = NULL;   //ready list head



unsigned char read_msg_syn(unsigned char* buf)
{
    return *(unsigned char*)&buf[DATA_SYN_OFFSET];
}

unsigned short read_msg_order(unsigned char * buf)
{
    return *(unsigned short *)&buf[DATA_ORDER_OFFSET];
}

int read_msg_size(unsigned char * buf)
{
    return *(int*)&buf[DATA_LEN_OFFSET];
}

int recv_msg(const int fd,char* buf, const int len)
{
    char * tmp = buf;
    int cnt = 0;
    int read_cnt = 0;
    while(read_cnt != len)
    {
        cnt = recv(fd,tmp + read_cnt ,len - read_cnt,0);
        if(cnt == 0)
        {
            return 1;
        }
        else
        {
            if(cnt < 0)
            {
                if(errno == EINTR || errno == EAGAIN)
                {
                    continue;
                }
                return 1;
            }
        }
        read_cnt += cnt;
    }
    return 0;
}

int send_msg(const int fd, const char *buf, const int len)
{
    const char* tmp = buf;
    int cnt = 0;
    int send_cnt = 0;
    while(send_cnt != len)
    {
        cnt =  send(fd, tmp + send_cnt ,len - send_cnt,0);
        if(cnt < 0)
        {
            if(errno == EINTR || errno == EAGAIN)
                continue;
            return 1;
        }
        send_cnt += cnt;
    }
    return 0;
}


void create_udp(char *ip, int port, int flag)
{
    DEBUG("ip %s, port %d", ip, port);
    int ret = -1;
    struct ip_mreq mreq;
    int opt = 0;

#ifdef _WIN32
    WSADATA wsData = {0};
    if(0 != WSAStartup(0x202, &wsData))
    {
        DEBUG("WSAStartup  fail");
        WSACleanup();
        return -1;
    }
    int socklen =  sizeof (struct sockaddr_in);
#else
    socklen_t socklen = sizeof (struct sockaddr_in);
#endif
    /* 创建 socket 用于UDP通讯 */
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        DIE("socket creating err in udptalk");
    }
    if(flag) //组播flag
    {
        /* 设置要加入组播的地址 */
        memset(&mreq, 0, sizeof (struct ip_mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(ip);
        /* 设置发送组播消息的源主机的地址信息 */
        mreq.imr_interface.s_addr = htonl (INADDR_ANY);

        /* 把本机加入组播地址，即本机网卡作为组播成员，只有加入组才能收到组播消息 */
        if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP , (char *)&mreq,sizeof (struct ip_mreq)) == -1)
        {
            //DIE("IP_ADD_MEMBERSHIP");
            DIE ("setsockopt");
        }

        opt = 1;
        if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&opt,sizeof (opt)) == -1)
        {
            DEBUG("IP_MULTICAST_LOOP set fail!\n");
        }
    }
    memset (&send_addr, 0, socklen);
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons (port);
    send_addr.sin_addr.s_addr = inet_addr(ip);

    opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR , (char *)&opt, sizeof(opt)) < 0)
    {
        DIE("setsockopt");
    }

    opt = 512*1024;//设置为32K
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&opt, sizeof (opt)) == -1)
    {
        DEBUG("IP_MULTICAST_LOOP set fail!\n");
    }

    opt = 512*1024;//设置为32K
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&opt, sizeof (opt)) == -1)
    {
        DEBUG("IP_MULTICAST_LOOP set fail!\n");
    }

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    recv_addr.sin_port = htons(port);
    /* 绑定自己的端口和IP信息到socket上 */
    if (bind(sockfd, (struct sockaddr *) &recv_addr,sizeof (struct sockaddr_in)) == -1)
    {
        DIE("Bind error");
    }
}


int create_tcp()
{
    int fd, sock_opt = -1;
#ifdef _WIN32
    WSADATA wsData = {0};
    if(0 != WSAStartup(0x202, &wsData))
    {
        DEBUG("WSAStartup  fail");
        WSACleanup();
        return -1;
    }
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        DIE("unable to create socket");
    }
    if (fcntl(fd, F_SETFD, 1) == -1) {
        DIE("can't set close-on-exec on server socket!");
    }
    if ((sock_opt = fcntl(fd, F_GETFL, 0)) < -1) {
        DIE("can't set close-on-exec on server socket!");
    }
     if (fcntl(fd, F_SETFL, sock_opt | O_NONBLOCK) == -1) {
        DIE("fcntl: unable to set server socket to nonblocking");
    }

    int on = 1 ;
    int keepAlive = 1;      //heart echo open
    int keepIdle = 15;      //if no data come in or out in 15 seconds,send tcp probe, not send ping
    int keepInterval = 3;   //3seconds inteval
    int keepCount = 5;      //retry count

    if( setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive)) != 0) goto end_out;
    if( setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle)) != 0) goto end_out;
    if( setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)) != 0) goto end_out;
    if( setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)) != 0) goto end_out;

    if( setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) !=0) goto end_out;
#endif
    return fd;
end_out:
    close(fd);
    return 0;
}


void bind_server(int fd, int port)
{
    struct sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof server_sockaddr);

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sockaddr.sin_port = htons(client_port);

     /* internet family-specific code encapsulated in bind_server()  */
    if (bind(fd, (struct sockaddr *) &server_sockaddr,
                sizeof (server_sockaddr)) == -1) {
        DEBUG("unable to bind");
        goto end_out;
    }

        /* listen: large number just in case your kernel is nicely tweaked */
    if (listen(fd, max_connections) == -1) {
        DEBUG("listen err");
        goto end_out;
    }
    return fd;

end_out:
    close(fd);
    return -1;
}

void connect_server(int fd, const char *ip, int port)
{

    struct sockaddr_in client_sockaddr;
    memset(&client_sockaddr, 0, sizeof client_sockaddr);

    client_sockaddr.sin_family = AF_INET;
    //client_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_sockaddr.sin_port = htons(port);

#if 0
    if(inet_aton(ip, (struct in_addr *)&client_sockaddr.sin_addr.s_addr) == 0)
    {
        DIE("inet_aton err");
        return ;
    }
#endif

    while(connect(fd, (struct sockaddr *)&client_sockaddr, sizeof(client_sockaddr)) != 0)
    {
        DEBUG("connection failed reconnection after 1 seconds");
        sleep(1);
    }
    return;
}

void close_socket(int fd)
{
    if(fd)
        close(fd);
}
/************************* request ***********************************/

static rfb_request *new_request()
{
    rfb_request* req = (rfb_request *) malloc(sizeof (rfb_request));
    if(!req)
    {
        DEBUG("malloc for new request");
        return NULL;
    }
    memset(req, 0, sizeof (rfb_request));

    req->display_id = -1;
    req->data_buf = NULL;

    return req;
}


static rfb_request* remove_request(rfb_request *req)
{
    rfb_request *next = req->next;

    if(req->data_buf)
        free(req->data_buf);
    req->data_buf = NULL;

#if 0
    if(req->display_id != -1)
    {
        displays[req->display_id].req = NULL;
        displays[req->display_id].play_flag = 0;
    }
#endif

    dequeue(&request_ready, req);

    req->next = NULL;
    req->prev = NULL;
    free(req);
    total_connections--;
    return next;
}

int send_request(rfb_request *req)
{
    if(!req || !req->fd)
        return 0;

    char *tmp = malloc(HEAD_LEN + req->data_size + 1);
    if(!tmp)
    {
        return 0;
    }
    memcpy(tmp, req->head_buf, HEAD_LEN);
    memcpy(tmp + HEAD_LEN, req->data_buf, req->data_size);
    send_msg(req->fd, tmp, HEAD_LEN + req->data_size);
    DEBUG("send_request %d fd", req->status, req->fd);

    free(tmp);
}


void h264_send_data(char *data, int len, int key_flag)
{
    int num = (len + DATA_SIZE - 1)/ DATA_SIZE;
    int pending = len;
    int dataLen = 0;
    char *ptr = data;
    char buf [DATA_SIZE + sizeof(rfb_packet)] = {0};
    rfb_packet *head = (rfb_packet *)&buf[0];
    short index = 0;
    head->total_num = num;
    head->b_keyFrame = key_flag;

    while(pending > 0)
    {
        dataLen = min(pending, DATA_SIZE);
        memcpy(buf + sizeof(rfb_packet), ptr, dataLen);
        head->num = index++;
        head->length = dataLen;
        ptr += dataLen;
        sendto(sockfd, buf,  dataLen + sizeof(rfb_packet), 0, (struct  sockaddr *)&send_addr, sizeof (send_addr));
        pending -= dataLen;
    }
}



/**************client *******************/
void *thread_client_udp(void *param)
{
    int ret;
    pthread_attr_t st_attr;
    struct sched_param sched;

    rfb_format *fmt = (rfb_format *)param;

    ret = pthread_attr_init(&st_attr);
    if(ret)
    {
        DEBUG("ThreadMain attr init error ");
    }
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {
        DEBUG("ThreadMain set SCHED_FIFO error");
    }
    sched.sched_priority = SCHED_PRIORITY_UDP;
    ret = pthread_attr_setschedparam(&st_attr, &sched);

    create_udp(server_ip, fmt->data_port, 0x0);
    client_udp_loop();
}


void *thread_client_tcp(void *param)
{
    int ret;
    pthread_attr_t st_attr;
    struct sched_param sched;

    ret = pthread_attr_init(&st_attr);
    if(ret)
    {
        DEBUG("ThreadMain attr init error ");
    }
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {
        DEBUG("ThreadMain set SCHED_FIFO error");
    }
    sched.sched_priority = SCHED_PRIORITY_UDP;
    ret = pthread_attr_setschedparam(&st_attr, &sched);

    client_tcp_loop();
}



void client_udp_loop()
{
    int ret;    
    fd_set fds;
    socklen_t socklen = sizeof (struct sockaddr_in);

    for(;;)
    {   
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        ret = select(sockfd + 1, &fds, NULL, NULL, NULL);
    
        if(ret < 0)
            continue;
    
        if(FD_ISSET(sockfd, &fds))
        {   
            //ret = recvfrom(sockfd, );
    
        }   
    }    
}

void client_tcp_loop()
{
    for(;;)
    {   

    }   
}


/***************server **************************/
void *thread_server_tcp(void *param)
{
    int ret, listenfd;
    pthread_attr_t st_attr;
    struct sched_param sched;

    listenfd = *(int *)param;

    ret = pthread_attr_init(&st_attr);
    if(ret)
    {
        DEBUG("ThreadMain attr init error ");
    }
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {
        DEBUG("ThreadMain set SCHED_FIFO error");
    }
    sched.sched_priority = SCHED_PRIORITY_UDP;
    ret = pthread_attr_setschedparam(&st_attr, &sched);

    server_tcp_loop(listenfd);
}



void *thread_server_udp(void *param)
{
    int ret, listenfd;
    pthread_attr_t st_attr;
    struct sched_param sched;

    listenfd = *(int *)param;

    ret = pthread_attr_init(&st_attr);
    if(ret)
    {
        DEBUG("ThreadMain attr init error ");
    }
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {
        DEBUG("ThreadMain set SCHED_FIFO error");
    }
    sched.sched_priority = SCHED_PRIORITY_UDP;
    ret = pthread_attr_setschedparam(&st_attr, &sched);

    server_tcp_loop(listenfd);
}


void server_tcp_loop(int listenfd)
{
    int maxfd, connfd, sockfd;
    int nready, ret;
    fd_set rset, allset;

    struct sockaddr_in cliaddr, servaddr;
    socklen_t clilen = sizeof(cliaddr);

    maxfd = listenfd;

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    rfb_request *conn = NULL;
    rfb_request *current = NULL;
    rfb_request *temp = NULL;

    total_connections = 0;

    for(;;)
    {
        rset = allset; // structure assignment
        ret = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if(ret <= 0)
        {
             if(errno == EINTR)
                continue;
            else if(errno != EBADF)
                DIE("select");
        }
        nready = ret;

        if(FD_ISSET(listenfd, &rset))
        {
            connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

             if(connfd < 0)
                continue;

            conn = new_request();
            conn->fd = connfd;
            conn->data_size = HEAD_LEN;

#ifdef _WIN32
            //memcpy(&cliaddr.sin_addr, conn->ip, sizeof(conn->ip));
#else
            ret = fcntl(connfd,F_GETFL,0);
            if(ret < 0)
            {
                close(connfd);
                continue;
            }
            if(fcntl(connfd, F_SETFL, ret | O_NONBLOCK) <0)
            {
                close(connfd);
                continue;
            }

            /* recode client ip */
            if(inet_ntop(AF_INET,&cliaddr.sin_addr, conn->ip, sizeof(conn->ip)) == NULL)
            {
                close(connfd);
                free(conn);
                continue;
            }
#endif
            DEBUG("connfd fd %d", conn->fd);

            enqueue(&request_ready, conn);
            FD_SET(connfd, &allset);
            total_connections++;
            if(connfd > maxfd)
                maxfd = connfd;
             if(--nready <= 0)
                continue;
        }

        current = request_ready;
        while(current)
        {
            if((sockfd = current->fd) < 0)
                continue;

            if(FD_ISSET(sockfd, &rset))
            {
                if(current->status != DEAD || current->status != DONE)
                {
                    if(current->has_read_head == 0)
                    {
                        if((ret = recv(current->fd, current->head_buf+current->data_pos,HEAD_LEN - current ->data_pos, 0)) <= 0)
                        {
                            if(ret < 0)
                            {
                                if(errno == EINTR || errno == EAGAIN)
                                    continue;
                            }
                            DEBUG("close fd %d", sockfd);
                            close(current->fd);
                            FD_CLR(current->fd, &allset);
                            current = remove_request(current);
                            continue;
                        }
                        current->data_pos += ret;
                        if(current->data_pos != HEAD_LEN)
                            continue;
                        if(read_msg_syn(current->head_buf) != DATA_SYN)
                        {
                            close(current->fd);
                            FD_CLR(current->fd, &allset);
                            current  = remove_request(current);
                            continue;
                        }

                        current->has_read_head = 1;
                        current->data_size = read_msg_size(current->head_buf);
                        current->data_pos = 0;

                        if(current->data_size < 0 || current->data_size > CLIENT_BUF)
                        {
                            close(current->fd);
                            FD_CLR(current->fd, &allset);
                            current = remove_request(current);
                            continue;
                        }
                        else
                        {
                            current->data_buf = (unsigned char*)malloc(current->data_size + 1);
                           if(!current->data_buf)
                            {
                                close(current->fd);
                                FD_CLR(current->fd, &allset);
                                current = remove_request(current);
                                continue;
                            }
                            memset(current->data_buf, 0, current->data_size + 1);
                        }
                    }
                    if(current->has_read_head == 1)
                    {
                        if((ret = recv(current->fd, current->data_buf + current->data_pos, current->data_size - current ->data_pos,0)) <= 0)
                        {
                            if(ret < 0)
                            {
                                if(errno == EINTR || errno == EAGAIN)
                                    continue;
                            }
                            DEBUG("close fd %d\n", sockfd);
                            close(current->fd);
                            FD_CLR(current->fd, &allset);
                            current = remove_request(current);
                            continue;
                        }
                        current->data_pos += ret;
                        if(current->data_pos == current->data_size)
                        {
                            if(process_msg(current))
                            {
                                DEBUG("close fd %d\n", sockfd);
                                close(current->fd);
                                FD_CLR(current->fd, &allset);
                                current = remove_request(current);
                                continue;
                            }
                            memset(current->head_buf, 0, HEAD_LEN);
                            current->data_size = 0;
                            current->data_pos = 0;
                            free(current->data_buf);
                            current->data_buf = NULL;
                            current->has_read_head = 0;
                        }

                        if(current->data_pos > current->data_size)
                        {
                            /** wow , something error **/
                            DEBUG("current->pos > current->size");
                            close(current->fd);
                            FD_CLR(current->fd, &allset);
                            current =  remove_request(current);
                            continue;
                        }
                    }
                }
                else
                {
                    DEBUG("close fd %d\n", sockfd);
                    close(current->fd);
                    FD_CLR(current->fd, &allset);
                    current = remove_request(current);
                    continue;
                }
                if(--nready <= 0)
                    break;
            }
            current = current->next;
        }
    }
}

