#include "base.h"
#include "msg.h"
#include "queue.h"

/* h264 data queue */
unsigned char **vids_buf;
QUEUE *vids_queue;
static int total_connections = 0;

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


void create_udp(char *ip, int port, rfb_display *display)
{
    int sockfd;
    struct sockaddr_in send_addr,recv_addr;

    DEBUG("ip  %s, port %d", ip, port);

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

    int opt = 0;
    /* 创建 socket 用于UDP通讯 */
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        DIE("socket creating err in udptalk");
    }

    if(NULL != ip)
    {
        memset (&send_addr, 0, socklen);
        send_addr.sin_family = AF_INET;
        send_addr.sin_port = htons (port);
        send_addr.sin_addr.s_addr = inet_addr(ip);
    }
    opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR , (char *)&opt, sizeof(opt)) < 0)
    {
        DIE("setsockopt SO_REUSEADDR");
    }

    opt = 521 * 1024;//设置为512K
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&opt, sizeof (opt)) == -1)
    {
        DEBUG("IP_MULTICAST_LOOP set fail!");
    }

    opt = 512 * 1024;//设置为512K
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&opt, sizeof (opt)) == -1)
    {
        DEBUG("IP_MULTICAST_LOOP set fail!");
    }

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    recv_addr.sin_port = htons(port);

    /* 绑定自己的端口和IP信息到socket上 */
    if (bind(sockfd, (struct sockaddr *) &recv_addr,sizeof (struct sockaddr_in)) == -1)
    {
        DIE("bind port %d err", port);
    }

	if(display)
	{
    	display->fd = sockfd;
    	display->port =  port;
    	display->recv_addr = recv_addr;
    	display->send_addr = send_addr;

    	display->frame_size = 1024 * 1024 - 1;
    	display->frame_pos = 0;
	}
	DEBUG("display->fd %d", display->fd);
}


int create_tcp()
{
    int fd, sock_opt = -1;
    int on = 1 ;
    int keepAlive = 1;      //heart echo open
    int keepIdle = 15;      //if no data come in or out in 15 seconds,send tcp probe, not send ping
    int keepInterval = 3;   //3seconds inteval
    int keepCount = 5;      //retry count


#ifdef _WIN32
    WSADATA wsData = {0};
    if(0 != WSAStartup(0x202, &wsData))
    {
        DEBUG("WSAStartup  fail");
        WSACleanup();
        return -1;
    }
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if( setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) !=0) goto end_out;

#if 0
	sock_opt = 1;
	if(ioctlsocket(fd, FIONBIO, &sock_opt) == NO_ERROR)
	{
        DIE("can't set close-on-exec on server socket!");
	}
#endif
	
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


int bind_server(int fd, int port)
{
	DEBUG("server bind port %d", port);
	
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
    return 0;

end_out:
    close(fd);
    return -1;
}

void connect_server(int fd, const char *ip, int port)
{
    struct sockaddr_in client_sockaddr;
    memset(&client_sockaddr, 0, sizeof client_sockaddr);

    client_sockaddr.sin_family = AF_INET;
    client_sockaddr.sin_addr.s_addr = inet_addr(ip);
    client_sockaddr.sin_port = htons(port);

    while(connect(fd, (struct sockaddr *)&client_sockaddr, sizeof(client_sockaddr)) != 0)
    {
        DEBUG("connection failed reconnection after 1 seconds");
        sleep(1);
    }
    return;
}


/************************* request ***********************************/

rfb_request *new_request()
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

    if(req->display_id != -1)
    {
        displays[req->display_id].req = NULL;
        displays[req->display_id].play_flag = 0;
		clear_texture();	
    }

    dequeue(&request_ready, req);

    req->next = NULL;
    req->prev = NULL;
    free(req);
    total_connections--;
    return next;
}

void set_request_head(rfb_request *req, short cmd, int data_size)
{
    rfb_head *head = (rfb_head *)&(req->head_buf[0]);
    head->syn = 0xff;
    head->encrypt_flag = 0; //加密
    head->cmd = cmd;
    req->data_size = head->data_size = data_size;
}


int send_request(rfb_request *req)
{
    if(!req || !req->fd)
        return -1;

    char *tmp = malloc(HEAD_LEN + req->data_size + 1);
    if(!tmp)
    {
        return -1;
    }
    memcpy(tmp, req->head_buf, HEAD_LEN);
    memcpy(tmp + HEAD_LEN, req->data_buf, req->data_size);
    send_msg(req->fd, tmp, HEAD_LEN + req->data_size);

    free(tmp);
	return 0;
}

static int frame_count = 0;

void h264_send_data(char *data, int len, int key_flag)
{
    //int num = (len + DATA_SIZE - 1)/ DATA_SIZE;
    int pending = len;
    int dataLen = 0;
    char *ptr = data;
    char buf[DATA_SIZE + 8] = {0};
    buf[0] = 0xff;
    buf[1] = 0xff;
    //buf[2] = 0xff;
    //buf[3] = 0xff;
    *(unsigned short *)(&buf[2]) = frame_count++;
    *(unsigned int *)(&buf[4]) = len;
    int first = 1;
    while(pending > 0)
    {   
        dataLen = min(pending, DATA_SIZE);

        if(first)  //+8个字节头 1 flag 4 datalen
        {   
            memcpy(buf + 8, ptr, dataLen);
            ptr += dataLen;
            sendto(client_display.fd, buf,  dataLen + 8, 0, 
					(struct  sockaddr *)&(client_display.send_addr), sizeof (client_display.send_addr));
            first = 0;
        }   
        else
        {   
            memcpy(buf, ptr, dataLen);
            ptr += dataLen;
            sendto(client_display.fd, buf,  dataLen, 0, 
				(struct sockaddr *)&(client_display.send_addr), sizeof (client_display.send_addr));
        }   
        pending -= dataLen;
    }    
}



/**************client *******************/
void client_tcp_loop(int sockfd)
{
	int ret;    
    fd_set fds;
	rfb_request *current = NULL;
	current = client_req;

	struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

	while(run_flag)
    {   
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        ret = select(sockfd + 1, &fds, NULL, NULL, &tv);

		if(ret == -1)
		{
			if(errno == EINTR)
				continue;
			else if(errno != EBADF)
				DIE("select %s", strerror(ret));
		}		
		if(FD_ISSET(sockfd, &fds))
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
					goto run_end;
                }
                current->data_pos += ret;
                if(current->data_pos != HEAD_LEN)
                    continue;
                if(read_msg_syn(current->head_buf) != DATA_SYN)
                {   
					current->data_pos = 0;
                    current->has_read_head = 0;
                    continue;
                }
                
                current->has_read_head = 1;
                current->data_size = read_msg_size(current->head_buf);
                current->data_pos = 0;
                
                if(current->data_size < 0 || current->data_size > CLIENT_BUF)
                {   
					current->data_pos = 0;
                    current->has_read_head = 0;
                    continue;
                }
                else
                {
                    current->data_buf = (unsigned char*)malloc(current->data_size + 1);
                   if(!current->data_buf)
                    {
						goto run_end;
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
                    DEBUG("close fd %d", sockfd);
					goto run_end;
                }
                current->data_pos += ret;
                if(current->data_pos == current->data_size)
                {   
                    process_client_msg(current);
                    memset(current->head_buf, 0, HEAD_LEN);
                    current->data_size = 0;
                    current->data_pos = 0;
                    free(current->data_buf);
                    current->data_buf = NULL;
                    current->has_read_head = 0;
                }
                
                if(current->data_pos > current->data_size)
                {   
					current->data_pos = 0;
					current->has_read_head = 0;
                    continue;
                }
            }
        }
    }    


run_end:
	if(run_flag)
		run_flag = 0;
	if(current)
		free(current);
	closesocket(sockfd);
	
#ifdef _WIN32
	WSACleanup();
#endif
}


void *thread_client_tcp(void *param)
{
    int ret,fd;
    pthread_attr_t st_attr;
    struct sched_param sched;

	fd = *(int *)param;	

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

    client_tcp_loop(fd);
	return (void *)0;
}


void client_udp_loop()
{
    int ret;    
    fd_set fds;
    socklen_t socklen = sizeof (struct sockaddr_in);
	int sockfd = client_display.fd;

	rfb_display *client = &client_display;
	
	while(run_flag)
    {   

		if(client->play_flag == -1)
			break;
#if 0
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        ret = select(sockfd + 1, &fds, NULL, NULL, NULL);
    
        if(ret < 0)
            continue;
#endif
		usleep(20000);
#if 0
        if(FD_ISSET(sockfd, &fds))
        {   
			ret = recvfrom(sockfd, (char *)client->frame_buf + client->frame_pos, clients->frame_size -> clients[i].f    rame_pos, 0,
492                     (struct sockaddr*)&(clients[i].recv_addr), &socklen);

    
        }   
#endif
    }    
	closesocket(sockfd);
}

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

    create_udp(server_ip, fmt->data_port, &client_display);
    client_udp_loop();
	return (void *)0;
}


/***************server **************************/
void server_udp_loop(int display_size, int maxfd, fd_set  allset, rfb_display *clients)
{
    int i, ret, nready;
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int sockfd = -1;
    socklen_t socklen = sizeof (struct sockaddr_in);
    unsigned int total_size = 0;
    unsigned int offset = 0;
    unsigned char *tmp;
    unsigned short count = 0;

	while(run_flag)
    {
        fds = allset;
        ret = select(maxfd + 1, &fds, NULL, NULL, &tv);
        if(ret == 0)
            continue;

        nready = ret;
        for(i = 0; i < display_size; i++)
        {
            if((sockfd = clients[i].fd) < 0)
                continue;
            if(FD_ISSET(sockfd, &fds))
            {
            	ret = recvfrom(sockfd, (char *)clients[i].frame_buf + clients[i].frame_pos, clients[i].frame_size - clients[i].frame_pos, 0,
                    (struct sockaddr*)&(clients[i].recv_addr), &socklen);

				if(ret < 0)
					continue;
	
	            tmp = &(clients[i].frame_buf[clients[i].frame_pos]);
	            clients[i].frame_pos += ret;
	            if(tmp[0] == 0xff && tmp[1] == 0xff)
	            {
#if 0
					if(total_size == 0)
					{
	                   	total_size = *((unsigned int *)&tmp[4]);
						continue;
					}	

					if(clients[i].frame_pos == total_size + 8)
					{
						total_size = 0;
						DEBUG("en_queue");
						en_queue(&vids_queue[i], clients[i].frame_buf + 8, clients[i].frame_pos - 8, 0x0);
	                    clients[i].frame_pos = 0;
	                    //clients[i].frame_size = 0;
	                    clients[i].current_count = 0;
					}
					if(clients[i].frame_pos > total_size + 8)
					{
						DEBUG("clients[i].frame_pos %d> total_size + 8 %d", clients[i].frame_pos, total_size +8);
						total_size = 0;
	                    clients[i].frame_pos = 0;
	                    //clients[i].frame_size = 0;
	                    clients[i].current_count = 0;
					}
#else
	               count = *((unsigned short *)&tmp[2]);
	               if(count != clients[i].current_count)
	               {
	                   if(clients[i].current_count == 0)
	                   {
	                       clients[i].current_count = count;
	                       continue;
	                   }
	                   total_size = *((unsigned int *)&tmp[4]);
	                   en_queue(&vids_queue[i], clients[i].frame_buf + 8,  clients[i].frame_pos - ret - 8, 0x0);
	                   memcpy(clients[i].frame_buf, tmp, ret);
	                   clients[i].frame_pos = ret;
	                   clients[i].current_count = count;
	               }
	               else
	               {
	                   if(clients[i].frame_pos == clients[i].frame_size + 8)
	                   {
						   DEBUG("en_queue");
	                       en_queue(&vids_queue[i], clients[i].frame_buf + 8, offset - 8, 0x0);
	                       clients[i].frame_pos = 0;
	                       clients[i].frame_size = 0;
	                       clients[i].current_count = 0;
	                   }
	               }
#endif
	          	}
				if(--nready <= 0)
	              break;
	       }
        }
    }


	for(i = 0; i< display_size; i++)
	{
		if(vids_buf[i])
			free(vids_buf[i]);
		if(clients[i].fd)
			closesocket(clients[i].fd);
		DEBUG("udp close fd %d", clients[i].fd);
	}

	if(vids_queue)
		free(vids_queue);
	if(vids_buf)
		free(vids_buf);
	DEBUG("server udp close end");
}

void create_h264_socket()
{
    int i, maxfd = -1;
    fd_set allset;
    FD_ZERO(&allset);
    vids_queue = (QUEUE *)malloc(sizeof(QUEUE) * display_size);
    vids_buf = (unsigned char **)malloc(display_size * sizeof(unsigned char *));
    DEBUG("display_size %d", display_size);

	while(!displays)
	{
		usleep(10000);
	}

    for(i = 0; i < display_size; i++)
    {
        create_udp(NULL, i + 1 + h264_port, &displays[i]);
        FD_SET(displays[i].fd, &allset);
        maxfd = maxfd > displays[i].fd ? maxfd : displays[i].fd;
        vids_buf[i] = (unsigned char *)malloc(MAX_VIDSBUFSIZE * sizeof(unsigned char));
        memset(vids_buf[i], 0, MAX_VIDSBUFSIZE);
        /* 创建窗口的对应队列 */
        init_queue(&(vids_queue[i]), vids_buf[i], MAX_VIDSBUFSIZE);
    }
    server_udp_loop(display_size, maxfd, allset, displays);
}



void server_tcp_loop(int listenfd)
{
    int maxfd, connfd, sockfd;
    int nready, ret;
    fd_set rset, allset;

    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    maxfd = listenfd;
	

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    rfb_request *conn = NULL;
    rfb_request *current = NULL;

    total_connections = 0;

	while(run_flag)
    {
        rset = allset; // structure assignment
        ret = select(maxfd + 1, &rset, NULL, NULL, &tv);
        if(ret == -1)
        {
             if(errno == EINTR)
                continue;
            else if(errno != EBADF)
				DIE("select %s", strerror(ret));
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
                            closesocket(current->fd);
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
                                closesocket(current->fd);
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
                            closesocket(current->fd);
                            FD_CLR(current->fd, &allset);
                            current = remove_request(current);
                            continue;
                        }
                        current->data_pos += ret;
                        if(current->data_pos == current->data_size)
                        {
                            if(process_server_msg(current))
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

	DEBUG("server_tcp end");
run_end:
	current = request_ready;
	while(current)
	{
		DEBUG("while current");
		if(current->fd)
		{
			closesocket(current->fd);
			DEBUG("close client fd %d", current->fd);
		}
		current = remove_request(current);
	}

	closesocket(listenfd);
	DEBUG("server_tcp end");
#ifdef _WIN32
	WSACleanup();
#endif
}

void *thread_server_tcp(void *param)
{
    int ret, listenfd = -1;
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
	return (void *)0;
}

void *thread_server_udp(void *param)
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

	create_h264_socket();
	return (void *)0;
}
