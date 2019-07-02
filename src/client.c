#include "base.h"
#include "msg.h"

void init_client()
{
	int ret;
	pthread_t pthread_tcp;
	rfb_format fmt = {0};

	int server_s = create_tcp();
	if(!server_s)
	{
		DIE("create tcp err");
	}	
	connect_server(server_s, server_ip, server_port);

	ret = login_server(server_s, &fmt);
	if(0 != ret)
	{
		close_socket(server_s);
		DIE("login_server err");
	}
	
	ret = init_dev();
	if(0 != ret)
	{
		close_socket(server_s);
        DIE("init device err %d,  %s",ret,strerror(ret));
	}

    ret = pthread_create(&pthread_tcp, NULL, thread_client_tcp, NULL);
    if(0 != ret)
    {   
		close_socket(server_s);
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    } 

	DEBUG("fmt.play_flag %d", fmt.play_flag);
	if(fmt.play_flag)
	{
		start_encode(&fmt);
	}	

	int **tret = NULL;
	pthread_join(pthread_tcp, (void**)tret);  //等待线程同步		
}

int login_server(int fd, rfb_format *fmt)
{
	unsigned char buf[1024] = {0};
	unsigned char *tmp = &buf[8];
	int server_major = 0, server_minor = 0;
		
	rfb_head *head = (rfb_head *)&buf[0];
	head->syn = 0xff;
	head->encrypt_flag = 0;	//加密
	head->cmd = 1;
	head->data_size = sz_verformat;
	
	/* version */
    sprintf(tmp, VERSIONFORMAT, server_major, server_minor);    

	send_msg(fd, buf, HEAD_LEN + sz_verformat);
	recv_msg(fd, buf, HEAD_LEN + sz_verformat);
		
	sscanf(buf + 8, VERSIONFORMAT, &server_major, &server_minor); 
    DEBUG(VERSIONFORMAT, server_major, server_minor);

	
	/* format */
	head->cmd = 2;
	head->data_size = sizeof(rfb_format);

    rfb_format *_fmt = (rfb_format *)&tmp[0];
    _fmt->width = 1280;
    _fmt->height = 720;
    _fmt->code = 0;
    _fmt->data_port = 0;
    _fmt->play_flag = 0;
    _fmt->vnc_flag = 0;	
    _fmt->quality = default_quality;	
    _fmt->fps = default_fps;	
	
	send_msg(fd, buf, HEAD_LEN + sizeof(rfb_format));
	recv_msg(fd, buf, HEAD_LEN + sizeof(rfb_format));

	memcpy(fmt, _fmt, sizeof(rfb_format));

	DEBUG("fmt->width %d fmt->height %d fmt->code %d,"
          " fmt->data_port %d fmt->play_flag %d  fmt->vnc_flag %d fmt->quality %d fmt->fps%d",
             fmt->width, fmt->height,fmt->code, fmt->data_port,fmt->play_flag, fmt->vnc_flag, fmt->quality, fmt->fps);
    if(fmt->width < 0 || fmt->width > 1920 || fmt->height < 0 || fmt->height > 1080 || fmt->data_port < 0 || fmt->data_port > 24000)
    {
        DEBUG("login server fail ");
		return -1;
    }
	return 0;
}

void start_encode(rfb_format *fmt)
{
	int ret;
	pthread_t pthread_udp, pthread_encode;


	ret = pthread_create(&pthread_udp, NULL, thread_client_udp, fmt);
    if(0 != ret)
    {   
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    } 

    ret = pthread_create(&pthread_encode, NULL, thread_encode, fmt);
    if(0 != ret)
    {   
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    }
}
