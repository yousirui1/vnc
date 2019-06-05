#include "base.h"
#include "msg.h"

rfb_client *clients = NULL;

fd_set fds;
fd_set _fds;
int max_fd;
int maxi;
	
void init_server(int window_size)
{
	int i, j, ret, id;

	clients = (rfb_client *)malloc(sizeof(rfb_client) * window_size * window_size);
	memset(clients, 0, sizeof(rfb_client) * window_size * window_size);

	FD_ZERO(&_fds);
	FD_ZERO(&fds);
	max_fd = -1;
	maxi = window_size * window_size;
	
	for(i = 0; i < window_size; i++)
	{
		for(j = 0; j < window_size; j++)
		{
			id = i + j * window_size;
			create_udp(NULL, id + h264_port + 1,  &(clients[id]));
	    	FD_SET(clients[id].udp_fd, &_fds);      //加入select 轮训队列
    		max_fd = max_fd > clients[id].udp_fd ? max_fd : clients[id].udp_fd;
		}
	}	
	sever_loop();
}
#if 0
void create_udp(int port, rfb_client *client)
{

    int ret = -1; 
    int sockfd;
    struct sockaddr_in send_addr,recv_addr;
    
    socklen_t socklen = sizeof (struct sockaddr_in);

    int opt = 0;
    /* 创建 socket 用于UDP通讯 */
    sockfd = socket (AF_INET, SOCK_DGRAM, 0); 
    if (sockfd < 0)
    {   
        printf ("socket creating err in udptalk\n");
        exit (1);
    }   
    opt = 32*1024;//设置为32K
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&opt, sizeof (opt)) == -1) 
    {   
        printf("IP_MULTICAST_LOOP set fail!\n");
    }   

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    recv_addr.sin_port = htons(port);

    /* 绑定自己的端口和IP信息到socket上 */
    if (bind(sockfd, (struct sockaddr *) &recv_addr,sizeof (struct sockaddr_in)) == -1) 
    {   
        perror("Bind error");
        exit (0);
    }   

    client->udp_fd = sockfd;
    client->port =  port;
    client->recv_addr = recv_addr;

    client->frame_size = 1024 * 1024 - 1;
}
#endif



#define REQUEST_TIMEOUT 10

void sever_loop()
{
	int i, ret, nready;
	struct timeval tv;
		
	tv.tv_sec = REQUEST_TIMEOUT;
	tv.tv_usec = 0l;

	int sockfd = -1;
	socklen_t socklen = sizeof (struct sockaddr_in);

	unsigned int total_size = 0;
	unsigned pos = 0;
	unsigned char *tmp = NULL;

	for(;;)
	{
		fds = _fds;
		ret = select(max_fd + 1, &fds, NULL, NULL, &tv);
		if(ret <= 0)
			continue;

		nready = ret;
		for(i = 0; i < maxi; i++)
		{
			if((sockfd = clients[i].udp_fd) < 0)
				continue;
			if(FD_ISSET(sockfd, &fds))
			{
				ret = recvfrom(sockfd, (char *)clients[i].frame_buf, clients[i].frame_size - clients[i].frame_pos, 0,
							 (struct sockaddr*)&(clients[i].recv_addr), &socklen);
				if(ret < 0)
				{
					continue;
				}
						
				tmp = &(clients[i].frame_buf[clients[i].frame_pos]);
				clients[i].frame_pos += ret;
				
				if(tmp[0] == 0xff && tmp[1] == 0xff)
				{
					total_size = *((unsigned int *)&tmp[4]);	
					if(clients[i].frame_size == 0)		//开头包
					{
						clients[i].frame_size = total_size;
					}
					else
					{
						if(total_size != clients[i].frame_size)		//之前包丢失,把收到的数据入队列
						{
							clients[i].frame_size = total_size;
							en_queue(&vids_queue, clients[i].frame_buf + 8, clients[i].frame_pos - ret - 8, 0x0);	
							memcpy(clients[i].frame_buf, tmp, ret);
							clients[i].frame_pos = ret;
						}	
					}	
				}
				
				if(clients[i].frame_pos == clients[i].frame_size + 8)
				{
					en_queue(&vids_queue, clients[i].frame_buf + 8, clients[i].frame_pos - 8, 0x0);
					clients[i].frame_pos = 0;
					clients[i].frame_size = 0;
				}
				if(--nready <= 0)
					break;
			} 	
		}
	}
}


