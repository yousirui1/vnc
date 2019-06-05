#include "base.h"

void init_server(int window_size)
{
	

}


void init_udp(char *ip, int port, int flag)
{
	
    DEBUG("ip %s, port %d", ip, port);
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
    
    if(flag) //组播flag
    {   
    	struct ip_mreq mreq;

        /* 设置要加入组播的地址 */
        memset(&mreq, 0, sizeof (struct ip_mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(ip);
        /* 设置发送组播消息的源主机的地址信息 */
        mreq.imr_interface.s_addr = htonl (INADDR_ANY);

        /* 把本机加入组播地址，即本机网卡作为组播成员，只有加入组才能收到组播消息 */
        if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP , (char *)&mreq,sizeof (struct ip_mreq)) == -1) 
        {   
            perror ("setsockopt");
            exit (-1);
        }   

        opt = 1;
        if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&opt,sizeof (opt)) == -1) 
        {   
            printf("IP_MULTICAST_LOOP set fail!\n");
        }   
    }   

	memset (&send_addr, 0, socklen);
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons (port);
    send_addr.sin_addr.s_addr = inet_addr(ip);

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
}


