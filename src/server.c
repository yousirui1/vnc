#include "base.h"
#include "msg.h"

int display_size = 0;
static int server_s = -1;
static pthread_t pthread_tcp, pthread_udp , pthread_display;


static int recv_login(rfb_request *req)
{
	if(read_msg_order(req->head_buf) == 0x01)
	{
		int server_major = 0, server_minor = 0;
		if (sscanf((char *)req->data_buf, VERSIONFORMAT,
                    &server_major, &server_minor) != 2)
				;
		sprintf((char *)req->data_buf, VERSIONFORMAT, 3, server_minor + 1);
		DEBUG("recv_login server_major %d server_minor %d", server_major, server_minor);
		req->status = OPTIONS;
		return send_request(req);
	}
    else
    {
		return -1;
    }
}

static int recv_options(rfb_request *req)
{
	if(read_msg_order(req->head_buf) == 0x02)
	{
		int i = 0;
		rfb_format *fmt = (rfb_format *)&req->data_buf[0];
        DEBUG("fmt->width %d, fmt->height %d fmt->code %d fmt->data_port %d", 
				fmt->width, fmt->height, fmt->code, fmt->data_port);	
		
		fmt->play_flag = 0;
		fmt->data_port = 0;
	
		while(!displays)
		{
			usleep(2000);
		}	
		
		if(status == CONTROL)
		{
			return send_request(req);	
		}
	
	
		for(i = 0; i < display_size; i++)   //ready play
        {
            if(!displays[i].play_flag)      //存在空闲的分屏, 发送play命令
            {
                displays[i].req = req;
                displays[i].play_flag = 1;  
                fmt->width = vids_width;
                fmt->height = vids_height;
                fmt->play_flag = 0x01;
                fmt->data_port = i + h264_port + 1;
                req->display_id = i;
				memcpy(&(displays[i].fmt), fmt, sizeof(rfb_format));
                DEBUG("req->display_id %d", i);
                break;
            }
        }

		if(fmt->play_flag)
		{
        	req->status = DONE;
			status = PLAY;
		}
        return send_request(req);
	}
	else
	{
		return -1;	
	}
}

static int send_control(rfb_request *req)
{
	DEBUG("send_control");
	int ret = -1;
	set_request_head(req, 0x02, sizeof(rfb_format));	
	req->data_buf = malloc(sizeof(rfb_format) + 1);	
	memset(req->data_buf, 0, sizeof(rfb_format));
	rfb_format *fmt = (rfb_format *)&req->data_buf[0];

	fmt->width = screen_width;
    fmt->height = screen_height;
    fmt->play_flag = 0x02;
    fmt->bps = 4000000;
    fmt->fps = 22;
	fmt->data_port = control_display->fmt.data_port;

	ret = send_request(req);		
	free(req->data_buf);
    req->data_buf = NULL;
	req->status = DONE;
    return ret;
}


static int send_play(rfb_request *req)
{
	int ret;
	req->status = DONE;
	set_request_head(req, 0x02, sizeof(rfb_format));	

	rfb_format *fmt = (rfb_format *)&req->data_buf[0];
	fmt->play_flag = 0x01;

	DEBUG("fmt->width %d fmt->height %d", fmt->width, fmt->height );
	ret = send_request(req);		
	free(req->data_buf);
    req->data_buf = NULL;
	req->status = DONE;
	DEBUG("req->play_id %d", req->display_id);
    return ret;
}

static int send_done(rfb_request *req)
{
	DEBUG("send_done ");
	int ret = -1;
	req->status = OPTIONS;
	set_request_head(req, 0x02, sizeof(rfb_format));	
	req->data_buf = malloc(sizeof(rfb_format) + 1);	
	memset(req->data_buf, 0, sizeof(rfb_format));
	ret = send_request(req);		
	free(req->data_buf);
    req->data_buf = NULL;
	DEBUG("req->play_id %d", req->display_id);
    return ret;
}


int process_server_msg(rfb_request *req)
{
	DEBUG("process_server_msg");
    int ret = 0;
    switch(req->status)
    {
		case NORMAL:
        case READ_HEADER:
        case LOGIN:
            ret = recv_login(req);
            break;
        case OPTIONS:					//被动请求开始显示
            ret = recv_options(req);
            break;
        case PLAY:						//主动要求显示
			ret = send_play(req);
			break;
        case CONTROL:
            ret = send_control(req);
			break;
        case DONE:
            ret = send_done(req);
			break;
        case DEAD:

        default:
            break;
    }
    return ret;
}




void init_server()
{
	int ret = -1;
	display_size = window_size * window_size;

#ifdef _WIN32
    load_wsa();
#endif
	server_s = create_tcp();

	if(server_s == -1)
	{
		DIE("create socket err");
	}	
	if(bind_server(server_s, client_port) == -1)
    {
        DIE("bind port %d err", client_port);
    }
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

}


void exit_server()
{
    void *tret = NULL;
    pthread_join(pthread_display, &tret);  //等待线程同步
    DEBUG("pthread_exit %d display", (int)tret);
    pthread_join(pthread_tcp, &tret);  //等待线程同步
    DEBUG("pthread_exit %d tcp", (int)tret);
    pthread_join(pthread_udp, &tret);  //等待线程同步
    DEBUG("pthread_exit %d udp", (int)tret);

	destroy_socket();
	destroy_display();
}



