#include "base.h"

unsigned int frame_count = 0;

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

int load_wsa()
{
#ifdef _WIN32
    WSADATA wsData = {0};
    if(0 != WSAStartup(0x202, &wsData))
    {
        DEBUG("WSAStartup  fail");
        WSACleanup();
        return ERROR;
    }
#endif
    return SUCCESS;
}

void unload_wsa()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

/* 关闭socket */
void close_fd(int fd)
{
	if(fd)
	{
#ifdef _WIN32
        closesocket(fd);
#else
        close(fd);
#endif
	}
}

/* 接收数据 */
int recv_msg(const int fd,char* buf, const int len)
{
    char *tmp = buf;
    int cnt = 0;
    int read_cnt = 0;
    while(read_cnt != len)
    {
        cnt = recv(fd,tmp + read_cnt ,len - read_cnt, 0);
        if(cnt == 0)
        {
            return ERROR;
        }
        else
        {
            if(cnt < 0)
            {
                if(errno == EINTR || errno == EAGAIN)
                {
                    continue;
                }
                return ERROR;
            }
        }
        read_cnt += cnt;
    }
    return SUCCESS;
}

/* 发送数据 */
int send_msg(const int fd, const char *buf, const int len)
{
    const char* tmp = buf;
    int cnt = 0;
    int send_cnt = 0;
    while(send_cnt != len)
    {
        cnt = send(fd, tmp + send_cnt ,len - send_cnt, 0);
        if(cnt < 0)
        {
            if(errno == EINTR || errno == EAGAIN)
                continue;
            return ERROR;
        }
        send_cnt += cnt;
    }
    return SUCCESS;
}

struct sock_udp create_udp(char *ip, int port)
{
    DEBUG("udp create ip %s, port %d", ip, port);
    int fd = -1;
    int sock_opt = 0;
    struct sockaddr_in send_addr,recv_addr;
    struct sock_udp udp;

#ifdef _WIN32
    int socklen =  sizeof (struct sockaddr_in);
#else
    socklen_t socklen = sizeof (struct sockaddr_in);
#endif
    fd = socket (AF_INET, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        DEBUG("socket creating err in udptalk");
    }

    if(NULL != ip)
    {
        memset (&send_addr, 0, socklen);
        send_addr.sin_family = AF_INET;
        send_addr.sin_port = htons (port);
        send_addr.sin_addr.s_addr = inet_addr(ip);
    }

    sock_opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR , (char *)&sock_opt, sizeof(sock_opt)) < 0)
    {
        DEBUG("setsocksock_sock_sock_opt SO_REUSEADDR");
    }

    sock_opt = 521 * 1024;//设置为512K
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&sock_opt, sizeof (sock_opt)) == -1)
    {
        DEBUG("IP_MULTICAST_LOOP set fail!");
    }

    sock_opt = 512 * 1024;//设置为512K
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&sock_opt, sizeof (sock_opt)) == -1)
    {
        DEBUG("IP_MULTICAST_LOOP set fail!");
    }

#ifdef _WIN32
    sock_opt  = 1;
    if (ioctlsocket(fd, FIONBIO,(u_long *)&sock_opt) == SOCKET_ERROR)
    {
        DEBUG("fcntl F_SETFL fail");
    }
#endif

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    recv_addr.sin_port = htons(port);

    /* 绑定自己的端口和IP信息到socket上 */
    if (bind(fd, (struct sockaddr *) &recv_addr,sizeof (struct sockaddr_in)) == -1)
    {
        DEBUG("bind port %d error", port);
    }
	if(fd)
    	udp.fd = fd;
    udp.port = port;
    udp.recv_addr = recv_addr;
    udp.send_addr = send_addr;

	DEBUG("udp.fd %d", udp.fd);
    return udp;
}


int create_tcp()
{
	int fd ,sock_opt;
    int keepAlive = 1;      //heart echo open
    int keepIdle = 15;      //if no data come in or out in 15 seconds,send tcp probe, not send ping
    int keepInterval = 3;   //3seconds inteval
    int keepCount = 5;      //retry count

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
    {
        DEBUG("unable to create socket");
        goto run_out;
    }

    sock_opt = 1;
    if( setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt)) !=0) goto run_out;
#ifdef _WIN32
		
#else
    if(fcntl(fd, F_SETFD, 1) == -1)
    {
        DEBUG("can't set close-on-exec on server socket!");
        goto run_out;
    }
    if((sock_opt = fcntl(fd, F_GETFL, 0)) < -1)
    {
        DEBUG("can't set close-on-exec on server socket!");
        goto run_out;
    }
    if(fcntl(fd, F_SETFL, sock_opt | O_NONBLOCK) == -1)
    {
        DEBUG("fcntl: unable to set server socket to nonblocking");
        goto run_out;
    }
    if( setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive)) != 0) goto run_out;
    if( setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle)) != 0) goto run_out;
    if( setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)) != 0) goto run_out;
#endif
    return fd;
run_out:
    close_fd(fd);
    return -1;
}

int bind_socket(int fd, int port)
{
    DEBUG("tcp bind port %d", port);

    struct sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof server_sockaddr);

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sockaddr.sin_port = htons(port);

    /* internet family-specific code encapsulated in bind_server()  */
    if (bind(fd, (struct sockaddr *) &server_sockaddr,
                sizeof (server_sockaddr)) == -1)
    {
        DEBUG("unable to bind");
        goto run_out;
    }

    /* listen: large number just in case your kernel is nicely tweaked */
    if (listen(fd, max_connections) == -1)
    {
        DEBUG("listen max_connect: %d error", max_connections);
        goto run_out;
    }

    return SUCCESS;
run_out:
    close_fd(fd);
    return ERROR;
}

/* 连接服务器 */
int connect_server(int fd, const char *ip, int port)
{
    int count = 10;
    struct sockaddr_in client_sockaddr;
    memset(&client_sockaddr, 0, sizeof client_sockaddr);

    client_sockaddr.sin_family = AF_INET;
    client_sockaddr.sin_addr.s_addr = inet_addr(ip);
    client_sockaddr.sin_port = htons(port);

    while(connect(fd, (struct sockaddr *)&client_sockaddr, sizeof(client_sockaddr)) != 0)
    {
        DEBUG("connection failed reconnection after 1 seconds");
        sleep(1);
        if(!count--)
        {
            return ERROR;
        }
    }
    return SUCCESS;
}

void set_request_head(char *buf, char encrypt_flag, short cmd, int data_size)
{
	if(!buf)
		return;
    req_head *head = (req_head *)buf;
    head->syn = 0xff;
    head->encrypt_flag = encrypt_flag;
    head->cmd = cmd;
    head->data_size = data_size;
}

/* 发送请求 */
int send_request(struct client *cli)
{
	int ret = ERROR;
    if(!cli || !cli->fd)
        return ERROR;

#if 0
    char *tmp = malloc(HEAD_LEN + cli->data_size + 1);
    if(!tmp)
    {
        DEBUG("send_request malloc size: %d error: %s", HEAD_LEN + cli->data_size + 1, strerror(errno));
        return ERROR;
    }
#endif
    //memcpy(tmp, cli->head_buf, HEAD_LEN);
    //memcpy(tmp + HEAD_LEN, cli->data_buf, cli->data_size);
    ret = send_msg(cli->fd, cli->head_buf, HEAD_LEN);
	if(cli->data_buf && cli->data_size)
    	ret = send_msg(cli->fd, cli->data_buf, cli->data_size);
    //free(tmp);
    return ret;
}

void create_h264_socket()
{
    int i, maxfd = -1;
    fd_set allset;
    FD_ZERO(&allset);


    for(i = 0; i <= display_size; i++)
    {
        displays[i].h264_udp = create_udp(NULL, i + h264_port);
        FD_SET(displays[i].h264_udp.fd, &allset);
        maxfd = maxfd > displays[i].h264_udp.fd ? maxfd : displays[i].h264_udp.fd;
    }
    server_udp_loop(maxfd, allset);
}

void h264_send_data(char *data, int len, struct sock_udp udp)
{
    int pending = len;
    int dataLen = 0;
    char *ptr = data;
    char buf[DATA_SIZE + 8] = {0};
    buf[0] = 0xff;
    buf[1] = 0xff;
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
            sendto(udp.fd, buf,  dataLen + 8, 0,
                    (struct  sockaddr *)&(udp.send_addr),
                    sizeof (udp.send_addr));
            first = 0;
        }
        else
        {
            memcpy(buf, ptr, dataLen);
            ptr += dataLen;
            sendto(udp.fd, buf,  dataLen, 0,
                (struct sockaddr *)&(udp.send_addr), sizeof (udp.send_addr));
        }
        pending -= dataLen;
    }
}

void close_client(struct client *cli)
{
	char buf[128] = {0};
	if(displays[cli->display_id].play_flag)
	{
		displays[cli->display_id].cli = NULL;
		displays[cli->display_id].play_flag = 0;
		 *(int *)&buf[HEAD_LEN] = cli->display_id;
		set_request_head(buf, 0x0, SER_DONE_MSG, sizeof(int));
		send_msg(pipe_tcp[0], buf, sizeof(int) + HEAD_LEN);
	}
	close_fd(cli->fd);
	free(cli->head_buf);
	if(cli->data_buf)
		free(cli->data_buf);
	free(cli);
}


void client_udp_loop()
{
    int ret, maxfd = 0, nready;
    char buf[DATA_SIZE] = {0};
    int data_len = 0;
    rfb_display *client = &cli_display;
    struct sock_udp *control_udp = &client->cli->control_udp;

    socklen_t socklen = sizeof (struct sockaddr_in);

    fd_set fds;
    FD_ZERO(&fds);

   	FD_SET(control_udp->fd, &fds);
 	FD_SET(pipe_udp[0], &fds);	

    maxfd = maxfd > control_udp->fd ? maxfd : control_udp->fd;
    maxfd = maxfd > pipe_udp[0] ? maxfd : pipe_udp[0];

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
	for(;;)
    {
		tv.tv_sec = 1;
		DEBUG("client upd select !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        ret = select(maxfd + 1, &fds, NULL, NULL, NULL);
			/* pipe msg */
        	if(FD_ISSET(pipe_udp[0], &fds))
        	{
				DEBUG("recv pipe udp msg");
            	ret = recv(pipe_udp[0], (void *)buf, sizeof(buf), 0);
				if(ret == 1)
				{
					if(buf[0] == 'S')
						break;
				}
            	if(--nready <= 0)
                	continue;
        	}		
            if(FD_ISSET(control_udp->fd, &fds))
            {
                ret = recvfrom(control_udp->fd, buf, DATA_SIZE, 0, (struct sockaddr*)&(control_udp->recv_addr), &socklen);
				if(client->play_flag == 2)
				{
                if(ret > HEAD_LEN)
                {
                    control_msg(buf);
                }
				
				}
            }
    }
    close_fd(control_udp->fd);
	DEBUG("client udp thread exit");
}


void client_tcp_loop(int sockfd)
{

    int maxfd = 0;
    int nready, ret, i, maxi = 0;
    fd_set reset, allset;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    FD_ZERO(&allset);
    FD_SET(sockfd, &allset);
    FD_SET(pipe_tcp[0], &allset);

    maxfd = maxfd > sockfd ? maxfd : sockfd;
    maxfd = maxfd > pipe_tcp[0] ? maxfd : pipe_tcp[0];

    char buf[DATA_SIZE] = {0};
    char *tmp = &buf[HEAD_LEN];

    struct client *current = &m_client;
    char stop = 'S';

	DEBUG("sockfd %d maxfd %d", sockfd, maxfd);
    for(;;)
    {
        tv.tv_sec = 1;
        reset = allset;
        ret = select(maxfd + 1, &reset, NULL, NULL, &tv);
        if(ret == -1)
        {
            if(errno == EINTR)
                continue;
            else if(errno != EBADF)
			{
               	DEBUG("select %s", strerror(ret));
				break;	
			}
        }
        nready = ret;
        /* pipe msg */
        if(FD_ISSET(pipe_tcp[0], &reset))
        {
            ret = recv(pipe_tcp[0], (void *)buf, sizeof(buf), 0);
            if(ret >= HEAD_LEN)
            {
                //process_client_pipe(buf, ret);
            }
            else if(ret == 1)
            {
                if(buf[0] == 'S')
				{
					DEBUG("event thread pipe msg exit");	
                    break;
				}
            }
            if(--nready <= 0)
                continue;
        }
        if(FD_ISSET(sockfd, &reset))
        {
            if(current->has_read_head == 0)
            {
                if((ret = recv(current->fd, current->head_buf+current->pos,HEAD_LEN - current ->pos, 0)) <= 0)
                {
                    if(ret < 0)
                    {
                        if(errno == EINTR || errno == EAGAIN)
                            continue;
                    }
                    send_msg(pipe_event[1], &stop, 1);
                    DEBUG("close fd %d", sockfd);
					break;
                }
                current->pos += ret;
                if(current->pos != HEAD_LEN)
                    continue;
                if(read_msg_syn(current->head_buf) != DATA_SYN)
                {
                    current->pos = 0;
                    current->has_read_head = 0;
                    continue;
                }

                current->has_read_head = 1;
                current->data_size = read_msg_size(current->head_buf);
                current->pos = 0;

                if(current->data_size < 0 || current->data_size > CLIENT_BUF)
                {
                    current->pos = 0;
                    current->has_read_head = 0;
                    continue;
                }
                else if(current->data_size > 0)
                {
                    current->data_buf = (unsigned char*)malloc(current->data_size + 1);
                    if(!current->data_buf)
                    {
                        DEBUG("current->data_buf malloc error : %s ", strerror(errno));
						send_msg(pipe_event[1], &stop, 1);	
						break;
                    }
                    memset(current->data_buf, 0, current->data_size + 1);
                }
            }
            if(current->has_read_head == 1)
            {
				if(current->pos < current->data_size)
				{
                	if((ret = recv(current->fd, current->data_buf + current->pos, current->data_size - current ->pos,0)) <= 0)
                	{
                    	if(ret < 0)
                    	{
                        	if(errno == EINTR || errno == EAGAIN)
                            	continue;
                    	}
                    	send_msg(pipe_event[1], &stop, 1);
                    	DEBUG("close fd %d", sockfd);
						break;
                	}
                	current->pos += ret;
				}
                if(current->pos == current->data_size)
                {
					if(process_client_msg(current))
					{

					}
                    memset(current->head_buf, 0, HEAD_LEN);
                    current->data_size = 0;
                    current->pos = 0;
                    if(current->data_buf)
                        free(current->data_buf);
                    current->data_buf = NULL;
                    current->has_read_head = 0;
                }

                if(current->pos > current->data_size)
                {
                    current->pos = 0;
                    current->has_read_head = 0;
                    continue;
                }
            }
            if(--nready <= 0)
                continue;
        }
    }

run_end:
    close_fd(sockfd);
}


void server_udp_loop(int maxfd, fd_set allset)
{
    int i, ret, nready;
    fd_set fds;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    int sockfd = -1;
    unsigned char *tmp = NULL;
	char buf[128] = {0};				//pipe msg buf
    socklen_t socklen = sizeof (struct sockaddr_in);

    FD_SET(pipe_udp[0], &allset);	
    maxfd = maxfd > pipe_udp[0] ? maxfd : pipe_udp[0];
	
    for(;;)
    {
        fds = allset;
        ret = select(maxfd + 1, &fds, NULL, NULL, NULL);
        if(ret == 0)
            continue;

        nready = ret;
		/* pipe msg */
        if(FD_ISSET(pipe_udp[0], &fds))
        {
			DEBUG("recv pipe udp msg");
            ret = recv(pipe_udp[0], (void *)buf, sizeof(buf), 0);
			if(ret == 1)
			{
				if(buf[0] == 'S')
					break;
			}
            if(--nready <= 0)
                continue;
        }
		pthread_mutex_lock(&display_mutex);
        for(i = 0; i <= display_size; i++)
        {
            if((sockfd = displays[i].h264_udp.fd) < 0 || !displays[i].cli)
                continue;
            if(FD_ISSET(sockfd, &fds))
            {
                ret = recvfrom(sockfd, (char *)displays[i].frame_buf + displays[i].frame_pos, MAX_VIDSBUFSIZE, 
					0, (struct sockaddr*)&(displays[i].h264_udp.recv_addr), &socklen);

                if(ret < 0)
                    continue;

                tmp = &(displays[i].frame_buf[displays[i].frame_pos]);
                displays[i].frame_pos += ret;
                if(tmp[0] == 0xff && tmp[1] == 0xff)
                {
                    displays[i].frame_size = *((unsigned int *)&tmp[4]);
                }
                if(displays[i].frame_pos == displays[i].frame_size + 8)
                {
                    en_queue(&vids_queue[i], displays[i].frame_buf + 8,  displays[i].frame_pos - 8, 0x0);
                    pthread_cond_signal(&(displays[i].cond));
                    displays[i].frame_pos = 0;
                    displays[i].frame_size = 0;
                }
                else if(displays[i].frame_pos > displays[i].frame_size + 8 || displays[i].frame_pos >= MAX_VIDSBUFSIZE)
                {
                    displays[i].frame_pos = 0;
                    displays[i].frame_size = 0;
                }

                if(--nready <= 0)
                  break;
            }
        }
		pthread_mutex_unlock(&display_mutex);
    }
	DEBUG("server thread udp loop end !!");
	pthread_cond_signal(&display_cond);
}

void server_tcp_loop(int listenfd)
{
    int maxfd = 0, connfd, sockfd;
    int nready, ret, i, maxi = 0;
	int total_connections = 0;
	char buf[DATA_SIZE] = {0};			//pipe msg buf
	char *tmp = &buf[HEAD_LEN];
	
	struct client *cli = NULL;

    fd_set reset, allset;

    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
	
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	
	FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    FD_SET(pipe_tcp[0], &allset);	

    maxfd = maxfd > listenfd ? maxfd : listenfd;
    maxfd = maxfd > pipe_tcp[0] ? maxfd : pipe_tcp[0];

	for(;;)
	{
		tv.tv_sec = 1;
		reset = allset;
		ret = select(maxfd + 1, &reset, NULL, NULL, &tv);	
		if(ret == -1)
        {
             if(errno == EINTR)
                continue;
            else if(errno != EBADF)
            {
                DEBUG("select %d %s ", errno, strerror(errno));
				return;
            }
        }
		nready = ret;
		/* pipe msg */
        if(FD_ISSET(pipe_tcp[0], &reset))
        {
            DEBUG("pipe cli msg !!!!");
            ret = recv(pipe_tcp[0], (void *)buf, sizeof(buf), 0);
            if(ret >= HEAD_LEN)
            {
                //process_server_pipe(buf, ret);
            }
			if(ret == 1)
			{
				if(buf[0] == 'S')
				{
					DEBUG("event thread send stop msg");
					break;
				}
				if(buf[0] == 'C')
				{
					pthread_mutex_lock(&clients_mutex);
					for(i = 0 ; i <= maxi; i++)
					{
						if(clients[i] == NULL || clients[i]->fd < 0)			
							continue;

						FD_CLR(clients[i]->fd, &allset);
						close_client(clients[i]);
						clients[i] = NULL;
						total_connections--;
					}
					pthread_mutex_unlock(&clients_mutex);
				}
			}
            if(--nready <= 0)
                continue;
        }
		/* new connect */
		if(FD_ISSET(listenfd, &reset))
		{
			connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
			if(connfd < 0)
				continue;
			cli = (struct client *)malloc(sizeof(struct client));
			if(!cli)
			{
				DEBUG("new connect and malloc struct client error :%s", strerror(errno));
				continue;
			}
			memset(cli, 0, sizeof(struct client));
			cli->fd = connfd;
			cli->data_size = HEAD_LEN;
			cli->head_buf = (unsigned char *)malloc(HEAD_LEN);
			if(!cli->head_buf)
			{
                DEBUG("fcntl connfd: %d  F_GETFL error :%s", connfd, strerror(errno));
                close_fd(connfd);
                free(cli);
                continue;
			}	
#ifdef _WIN32
			memcpy(cli->ip,inet_ntoa(cliaddr.sin_addr), sizeof(cli->ip));
#else
            ret = fcntl(connfd, F_GETFL, 0);
            if(ret < 0)
            {
                DEBUG("fcntl connfd: %d  F_GETFL error :%s", connfd, strerror(errno));
                close_fd(connfd);
                free(cli->head_buf);
                free(cli);
                continue;
            }

            if(fcntl(connfd, F_SETFL, ret | O_NONBLOCK) < 0)
            {
                DEBUG("fcntl connfd: %d F_SETFL O_NONBLOCK error :%s", connfd, strerror(errno));
                close_fd(connfd);
                free(cli->head_buf);
                free(cli);
                continue;
            }
            /* recode client ip */
            if(inet_ntop(AF_INET, &cliaddr.sin_addr, cli->ip, sizeof(cli->ip)) == NULL)
            {
                DEBUG("connfd: %d inet_ntop error ",connfd, strerror(errno));
                close_fd(connfd);
                free(cli->head_buf);
                free(cli);
                continue;
            }
#endif
			FD_SET(connfd, &allset);
            for(i = 0; i < max_connections; i++)
            {
                //if(clients[i] == NULL || (sockfd = clients[i]->fd) < 0)
                if(clients[i] == NULL)
                    clients[i] = cli;
                break;
            }
            total_connections++;
            if(i > maxi)
                maxi = i;
            if(connfd > maxfd)
                maxfd = connfd;

            DEBUG("client index: %d total_connections: %d maxi: %d connfd ip: %s",i, total_connections, maxi, cli->ip);
            if(--nready <= 0)
                continue;
		}
		/* client msg */
		//pthread_mutex_lock();
		for(i = 0 ; i <= maxi; i++)
		{
			if(clients[i] == NULL || (sockfd = clients[i]->fd) < 0)			
				continue;
			if(FD_ISSET(sockfd, &reset))
			{
				if(clients[i]->has_read_head == 0)
				{
					if((ret = recv(sockfd, (void *)clients[i]->head_buf + clients[i]->pos, 
										HEAD_LEN-clients[i]->pos, 0)) <= 0)			
					{
						if(ret < 0)
						{
							if(errno == EINTR || errno == EAGAIN)
								continue;
						}
						DEBUG("client close index: %d ip: %s port %d", clients[i]->index,
								 	clients[i]->ip, clients[i]->port);

						FD_CLR(clients[i]->fd, &allset);
						close_client(clients[i]);
						clients[i] = NULL;
						total_connections--;
						continue;
					}
					clients[i]->pos += ret;
					if(clients[i]->pos != HEAD_LEN)
						continue;
				
					if(read_msg_syn(clients[i]->head_buf) != DATA_SYN)
					{
						DEBUG(" %02X client send SYN flag error close client index: %d ip: %s port %d",	
							read_msg_syn(clients[i]->head_buf), clients[i]->index, clients[i]->ip, 
							clients[i]->port);

						FD_CLR(clients[i]->fd, &allset);
						close_client(clients[i]);
						clients[i] = NULL;
						total_connections--;
						continue;
					}
					
					clients[i]->has_read_head = 1;
					clients[i]->data_size = read_msg_size(clients[i]->head_buf);
					clients[i]->pos = 0;
							
					if(clients[i]->data_size >= 0 && clients[i]->data_size < CLIENT_BUF)
					{
						clients[i]->data_buf = (unsigned char*)malloc(clients[i]->data_size + 1);
						if(!clients[i]->data_buf)
						{
							DEBUG("malloc data buf error: %s close client index: %d ip: %s port %d",
									strerror(errno), clients[i]->index, clients[i]->ip, clients[i]->port);

							FD_CLR(clients[i]->fd, &allset);
							close_client(clients[i]);
							clients[i] = NULL;
							total_connections--;
							continue;
						}
						memset(clients[i]->data_buf, 0, clients[i]->data_size);							
					}
					else
					{
						DEBUG("client send size: %d error close client index: %d ip: %s port %d",
								clients[i]->data_size, clients[i]->index, clients[i]->ip, clients[i]->port);

						FD_CLR(clients[i]->fd, &allset);
						close_client(clients[i]);
						clients[i] = NULL;
						total_connections--;
						continue;
					}
				}	
				
				if(clients[i]->has_read_head == 1)
				{
					if(clients[i]->pos < clients[i]->data_size)
					{
						if((ret = recv(sockfd,clients[i]->data_buf+clients[i]->pos, 
										clients[i]->data_size - clients[i]->pos,0)) <= 0)	
						{
							if(ret < 0)
							{
								if(errno == EINTR || errno == EAGAIN)
									continue;
								DEBUG("client close index: %d ip: %s port %d",
										clients[i]->index, clients[i]->ip, clients[i]->port);

								FD_CLR(clients[i]->fd, &allset);
								close_client(clients[i]);
								clients[i] = NULL;
								total_connections--;
								continue;
							}
						}
						clients[i]->pos += ret;
					}
			
					if(clients[i]->pos == clients[i]->data_size)	
					{
						if(process_server_msg(clients[i]))
						{
							DEBUG("process msg error client index: %d ip: %s port %d",
									clients[i]->index, clients[i]->ip, clients[i]->port);

							FD_CLR(clients[i]->fd, &allset);
							close_client(clients[i]);
							clients[i] = NULL;
							total_connections--;
							continue;
						}
						memset(clients[i]->head_buf, 0, HEAD_LEN);
						clients[i]->data_size = HEAD_LEN;
						clients[i]->pos = 0;
						if(clients[i]->data_buf)
							free(clients[i]->data_buf);
						clients[i]->data_buf = NULL;
						clients[i]->has_read_head = 0;
					}
					if(clients[i]->pos > clients[i]->data_size)
					{
						DEBUG("loss msg data client index: %d ip: %s port %d", clients[i]->index,
								clients[i]->ip, clients[i]->port);
						FD_CLR(clients[i]->fd, &allset);
						close_client(clients[i]);
						clients[i] = NULL;
						total_connections--;
						continue;
					}
				}	
				if(--nready <= 0)
					break;
			}
		}
		//pthread_mutex_lock();
	}
	DEBUG("thread server tcp end");	
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

void *thread_server_tcp(void *param)
{
	int ret, listenfd = -1;
    pthread_attr_t st_attr;
    struct sched_param sched;

    listenfd = *(int *)param;

    ret = pthread_attr_init(&st_attr);
    if(ret)
    {
        DEBUG("Thread Server TCP attr init error ");
    }
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {
        DEBUG("Thread Server TCP set SCHED_FIFO error");
    }
    sched.sched_priority = SCHED_PRIORITY_TCP;
    pthread_attr_setschedparam(&st_attr, &sched);

    server_tcp_loop(listenfd);
	return (void*)ret;
}


void *thread_client_udp(void *param)
{
    int ret;
    pthread_attr_t st_attr;
    struct sched_param sched;

    //int port = *(int *)param;

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
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);     //线程可以被取消掉
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);//立即退出

    client_udp_loop();
    return (void *)0;
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


