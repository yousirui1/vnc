#include "base.h"
#include "msg.h"

int display_size = 0;

int read_login(rfb_request *req)
{
    if(read_msg_order(req->head_buf) == 0x01)
    {
        int server_major, server_minor;

        if (sscanf((char *)req->data_buf, VERSIONFORMAT,
                    &server_major, &server_minor) != 2)
                ;
        sprintf((char *)req->data_buf, VERSIONFORMAT, 3, server_minor + 1);
        send_request(req);
        req->status = OPTIONS;
		return 0;
    }      
    else
    {
        DEBUG("read_login %d", read_msg_order(req->head_buf));
		return 1;
    }
}

int read_options(rfb_request *req)
{
   	if(read_msg_order(req->head_buf) == 0x02)
   	{
       	int i ;
       	rfb_format *fmt = (rfb_format *)&req->data_buf[0];
       	DEBUG("fmt->width %d, fmt->height %d fmt->code %d fmt->data_port %d", fmt->width, fmt->height, fmt->code, fmt->data_port);

       	fmt->play_flag = 0;
       	fmt->data_port = 0;
       	for(i = 0; i < display_size; i++)   //ready play
       	{
           	if(!displays[i].play_flag)      //存在空闲的分屏, 发送play命令
           	{
               	displays[i].req = req;
               	displays[i].play_flag = 1;
                fmt->width = vids_width;
                fmt->height = vids_height;
               	fmt->play_flag = 0x01;
               	fmt->vnc_flag = 0x00;       //副码流
               	fmt->data_port = i + h264_port + 1;
               	req->display_id = i;
               	DEBUG("req->display_id %d", i);
               	break;
           	}
       	}
       	send_request(req);
       	req->status = DONE;
		return 0;
    }
	else
	{
        DEBUG("read_login %d", read_msg_order(req->head_buf));
		return 1;
	}
}

int process_msg(rfb_request *req)
{

    int ret = 0;

    switch(req->status)
    {
        case READ_HEADER:
        case LOGIN:
            ret = read_login(req);
            break;
        case OPTIONS:
            ret = read_options(req);
            break;
        case DONE:

        case DEAD:
        default:
            break;
    }
	return ret;
}

void init_server()
{
    int ret, server_s;
    pthread_t pthread_tcp, pthread_udp , pthread_display;

    server_s = create_tcp();
	
	run_flag = 1;

	display_size = window_size * window_size;
    if(server_s == -1) 
    {   
        DIE("create socket err");
    }   

    if(bind_server(server_s, client_port) == -1) 
    {   
        DIE("bind port %d err", client_port);
    }   
	DEBUG("server_s %d", server_s);

	ret = pthread_create(&pthread_display, NULL, thread_display, NULL);
    if(0 != ret)
    {
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    }

    ret = pthread_create(&pthread_tcp, NULL, thread_server_tcp, &server_s);
    if(0 != ret)
    {
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    }

    ret = pthread_create(&pthread_udp, NULL, thread_server_udp, NULL);
    if(0 != ret)
    {
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    }

    void *tret = NULL;
    pthread_join(pthread_display, &tret);  //等待线程同步
	DEBUG("pthread_join %d", (int)tret);
    pthread_join(pthread_tcp, &tret);  //等待线程同步
	DEBUG("pthread_join %d", (int)tret);
    pthread_join(pthread_udp, &tret);  //等待线程同步
	DEBUG("pthread_join %d", (int)tret);
}



