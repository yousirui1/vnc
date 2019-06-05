#include "base.h"
#include "msg.h"

void *client_socket(void *param)
{
	int sockfd = 0;
    struct sockaddr_in ser_addr;
         
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {   
        DIE("client socket err");
    }   

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(server_port);
        
    if(inet_aton(server_ip, (struct in_addr *)&ser_addr.sin_addr.s_addr) == 0)
    {   
        DIE("client inet_aton err");
    }   
        
    while(connect(sockfd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) != 0)
    {   
        DEBUG("connection failed reconnection after 3 seconds");
        sleep(3);
    }   

	login_server();
	select_loop();
}


void login_server()
{
	unsigned char buf[1024] = {0};
    unsigned char *tmp = &buf[8];

	int server_major = 0, server_minor = 1;

	rfb_head *head = (rfb_head *)&buf[0];
	head->syn = 0xff;
    head->enry_flag = 0;
	
	/* version */
	sprintf(tmp, VERSIONFORMAT, server_major, server_minor);	

	send(sockfd, buf, HEAD_LEN + sz_verformat, 0);
	read(sockfd, buf, HEAD_LEN + sz_verformat);

	//sscanf(buf + 8, VERSIONFORMAT, &server_major, &server_minor);	
	DEBUG(VERSIONFORMAT, server_major, server_minor);
	

	/* format */
	rfb_format *fmt = (rfb_format *)&tmp[0];
	fmt->width = 1280;
    fmt->height = 720;
    fmt->code = 0;
    fmt->data_port = 0;
    fmt->play_flag = 0;
    fmt->vnc_flag = 0;
	
	send(sockfd, buf, 8 + sizeof(rfb_format), 0);
    read(sockfd, buf, 8 + sizeof(rfb_format), 0);
	
	DEBUG("fmt->width %d fmt->height %d fmt->code %d,"
		  " fmt->data_port %d fmt->play_flag %d  fmt->vnc_flag %d",
			 fmt->width, fmt->height,fmt->code, fmt->data_port,fmt->play_flag, fmt->vnc_flag);
	if(fmt->width < 0 || fmt->width > 1920 || fmt->height < 0 || fmt->height > 1080 || fmt->data_port < 0 || fmt->data_port > 24000)
	{
		DIE("login server fail ");
	}

	//init_udp(fmt->data_port);
}




void select_loop()
{


}


