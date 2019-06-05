#include "base.h"
#include "msg.h"

void create_udp(char *ip, int port, rfb_client *client)
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

        perror("setsockopt");
        exit (-1);
    }

    opt = 32*1024;//设置为32K
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&opt, sizeof (opt)) == -1)
    {
        printf("IP_MULTICAST_LOOP set fail!\n");
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
	client->send_addr = send_addr;

    client->frame_size = 1024 * 1024 - 1;
}


void h264_send_data(char *data, int len, rfb_client *client)
{
    int num = (len + DATA_SIZE - 1)/ DATA_SIZE;
    int pending = len;
    int dataLen = 0;
    char *ptr = data;
    char buf[DATA_SIZE + 8] = {0};
    buf[0] = 0xff;
    buf[1] = 0xff;
    buf[2] = 0xff;
    buf[3] = 0xff;
    *(unsigned int *)(&buf[4]) = len;
    int first = 1;
    while(pending > 0)
    {   
        dataLen = min(pending, DATA_SIZE);

        if(first)  //+8个字节头 1 flag 4 datalen
        {   
            memcpy(buf + 8, ptr, dataLen);
            ptr += dataLen;
            sendto(client->udp_fd, buf,  dataLen + 8, 0, (struct  sockaddr *)&(client->send_addr), sizeof (struct sockaddr_in));
            first = 0;
        }   
        else
        {   
            memcpy(buf, ptr, dataLen);
            ptr += dataLen;
            sendto(client->udp_fd, buf,  dataLen, 0, (struct sockaddr *)&(client->send_addr), sizeof (struct sockaddr_in));
        }   
        pending -= dataLen;
        //usleep(50);
    }   
}

