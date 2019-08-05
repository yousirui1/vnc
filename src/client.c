#include "base.h"
#include "msg.h"

rfb_request *client_req = NULL;
rfb_display client_display = {0};

static int server_s = -1;


static int recv_options(rfb_request *req)
{
    if(read_msg_order(req->head_buf) == 0x02)
    {
        rfb_format *fmt = (rfb_format *)&req->data_buf[0];
         DEBUG("fmt->width %d fmt->height %d fmt->code %d,"
          " fmt->data_port %d fmt->play_flag %d  fmt->bps %d fmt->fps%d",
             fmt->width, fmt->height,fmt->code, fmt->data_port,fmt->play_flag, fmt->bps, fmt->fps);

		client_display.play_flag = fmt->play_flag;
		memcpy(&(client_display.fmt), fmt, sizeof(rfb_format));

		if(fmt->play_flag == 1)
		{
			req->status = PLAY;
            create_encode(&client_display.fmt);
		}

		if(fmt->play_flag == 2)
		{
			/* 获取服务端分辨率用于控制坐标转换 */
			vids_width = fmt->width;   
			vids_height = fmt->height;
			req->status = CONTROL;
            create_encode(&client_display.fmt);
		}
		else
		{
			req->status = OPTIONS;
		}
    }
}

static int send_options(rfb_request *req)
{
    req->data_size = sizeof(rfb_format);
    req->data_buf = malloc(req->data_size + 1);
    rfb_format *fmt = (rfb_format *)&req->data_buf[0];
    fmt->width = 1280;
    fmt->height = 720;
    fmt->code = 0;
    fmt->data_port = 0;
    fmt->play_flag = 0;
    fmt->bps = 2000000;
    fmt->fps = default_fps;

    set_request_head(req, 2, sizeof(rfb_format));
    req->status = OPTIONS;
    return send_request(req);
}

static int recv_login(rfb_request *req)
{
    if(read_msg_order(req->head_buf) == 0x01)
    {
        int server_major = 0, server_minor = 0;
        sscanf(req->data_buf, VERSIONFORMAT, &server_major, &server_minor);
        DEBUG(VERSIONFORMAT, server_major, server_minor);
        free(req->data_buf);
        send_options(req);
    }
}

static int send_login(rfb_request *req)
{
    int ret;
    int server_major = 0, server_minor = 0;

	set_request_head(req, 0x01, sz_verformat);
	
    req->data_size = sz_verformat;
    req->data_buf = malloc(req->data_size + 1);

    sprintf(req->data_buf, VERSIONFORMAT, server_major, server_minor);

    set_request_head(req, 1, sz_verformat);
    req->status = LOGIN;

	DEBUG("req->fd %d", req->fd);
    ret = send_request(req);
    free(client_req->data_buf);
    return ret;
}



void create_encode(rfb_format *fmt)
{
    int ret;
#if 0
    rfb_format fmt = {0};
    memcpy(&fmt, _fmt, sizeof(rfb_format));
#endif
    pthread_t pthread_udp, pthread_encode;

         DEBUG("fmt->width %d fmt->height %d fmt->code %d,"
          " fmt->data_port %d fmt->play_flag %d  fmt->bps %d fmt->fps%d",
             fmt->width, fmt->height,fmt->code, fmt->data_port,fmt->play_flag, fmt->bps, fmt->fps);


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



int process_client_msg(rfb_request *req)
{
    int ret = 0;
    switch(req->status)
    {
        case READ_HEADER:
        case LOGIN:
            ret = recv_login(req);
            break;
        case OPTIONS:
		case PLAY:
            ret = recv_options(req);
            break;
		case CONTROL:
			if(read_msg_order(req->head_buf) == 0x02)
				ret = recv_options(req);
			else	
				ret = control_msg(req);
        case DONE:

        case DEAD:
        default:
            break;
    }
    return ret;
}



void init_client()
{
    int ret = -1;
    pthread_t pthread_tcp, pthread_display;
	
    server_s = create_tcp();
    if(!server_s)
    {
        DIE("create tcp err");
    }
    connect_server(server_s, server_ip, server_port);
	
	client_req = new_request();
	if(!client_req)
	{
        DIE("create tcp err");
	}
	client_req->fd = server_s;

	DEBUG("server_s %d", client_req->fd );

	ret = pthread_create(&pthread_display, NULL, thread_display, NULL);
    if(0 != ret)
    {
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    }

	
	ret = pthread_create(&pthread_tcp, NULL, thread_client_tcp, &server_s);
    if(0 != ret)
    {
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    }
	
	DEBUG("server_s %d", client_req->fd);
	ret = send_login(client_req);
	if(0 != ret)
	{
		DIE("login_server err");
	}

#ifndef DLL
    void *tret = NULL;
    pthread_join(pthread_tcp, (void**)tret);  //等待线程同步
	DEBUG("pthread_exit client tcp");
#endif
}