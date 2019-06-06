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


#define REQUEST_TIMEOUT 10

void sever_loop()
{
	int i, ret, nready;
	struct timeval tv;
		
	tv.tv_sec = REQUEST_TIMEOUT;
	tv.tv_usec = 0l;

	int sockfd = -1;
	socklen_t socklen = sizeof (struct sockaddr_in);

#if 0
	unsigned int total_size = 0;
	unsigned pos = 0;
	unsigned char *tmp = NULL;
#endif

	    unsigned int total_size = 0;
    unsigned int tmp_size = 0;
    unsigned char buf[MAX_VIDSBUFSIZE] = {0};
    unsigned int offset = 0;
    unsigned char *tmp;


    unsigned short count = 0;
    unsigned short current_count = 0;


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
			ret = recvfrom(sockfd, (char *)clients[i].frame_buf + clients[i].frame_pos, clients[i].frame_size - clients[i].frame_pos, 0,
					(struct sockaddr*)&(clients[i].recv_addr), &socklen);

            tmp = &(clients[i].frame_buf[clients[i].frame_pos]);
            clients[i].frame_pos += ret;
            if(errno == EINTR || errno == EAGAIN)
            {   
                continue;
            }   
            else
            {   
                //emit sigNoRecv();  //发送信号
                //break;
            }   

             if(tmp[0] == 0xff && tmp[1] == 0xff)
            {   
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
                        en_queue(&vids_queue[i], clients[i].frame_buf + 8, offset - 8, 0x0);
                        clients[i].frame_pos = 0;
                        clients[i].frame_size = 0;
                        clients[i].current_count = 0;
                    }   
                }   
            }   

		

#if 0
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
#endif
				if(--nready <= 0)
					break;
			} 	
		}
	}
}


