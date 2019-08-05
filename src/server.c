#include "base.h"
#include "msg.h"

int display_size = 0;
static int server_s = -1;


static int recv_login(rfb_request *req)
{
	if(read_msg_order(req->head_buf) == 0x01)
	{
		int server_major = 0, server_minor = 0;
		if (sscanf((char *)req->data_buf, VERSIONFORMAT,
                    &server_major, &server_minor) != 2)
				;
		sprintf((char *)req->data_buf, VERSIONFORMAT, 3, server_minor + 1);
		req->status = OPTIONS;
		return send_request(req);
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
			DEBUG("is NO display");
			usleep(2000);
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
        req->status = DONE;
        return send_request(req);
	}
}


static int send_options(rfb_request *req, rfb_format *_fmt)
{
	int ret = -1;
	set_request_head(req, 0x02, sizeof(rfb_format));
	req->data_buf = malloc(sizeof(rfb_format) + 1);
	rfb_format *fmt = (rfb_format *)&req->data_buf[0];	

	memcpy(fmt, _fmt, sizeof(rfb_format));

	ret = send_request(req);
	free(req->data_buf);
	req->data_buf = NULL;
	return ret;
}



void pause_vids(rfb_display *cli)
{
	cli->fmt.play_flag = cli->play_flag = -1;    //暂停状态 
	send_options(cli->req, &(cli->fmt));
}


void pause_all_vids()
{
	int i = 0;
	for(i = 0; i < display_size; i++)
	{
		if(displays[i].play_flag == 1)
		{
			pause_vids(&displays[i]);
		}
	}
}

void start_control(rfb_display *cli)
{
	rfb_format fmt = {0};

	fmt.width = screen_width;
	fmt.height = screen_height;
	fmt.play_flag = 0x02;
	fmt.data_port = cli->fmt.data_port;
	fmt.bps = 4000000;
	fmt.fps = 25;

	cli->fmt.play_flag = cli->play_flag = 2;    //控制状态
	send_options(cli->req, &(fmt));
	clear_texture();
}

void stop_control(rfb_display *cli)
{
	cli->fmt.play_flag = cli->play_flag = -1;    //控制状态
	send_options(cli->req, &(cli->fmt));
	clear_texture();
}

void revert_vids(rfb_display *cli)
{
	cli->fmt.play_flag = cli->play_flag = 1;    //暂停状态 
	send_options(cli->req, &(cli->fmt));
}

void revert_all_vids()
{
	int i = 0;
	for(i = 0; i < display_size; i++)
	{
		if(displays[i].play_flag == -1)
		{
			revert_vids(&displays[i]);
		}
	}
}





#if 0
void pause_vids(rfb_display *cli)
{
	cli->fmt.play_flag = cli->play_flag = -1;    //暂停状态 
	send_options(cli->req, &(cli->fmt));
}

void pause_all_vids()
{
	int i = 0;
	for(i = 0; i < display_size; i++)
	{
		if(displays[i].play_flag == 1)
		{
			pause_vids(displays[i]);
		}
	}
}

void start_control(rfb_display *cli)
{
	cli->fmt.width = screen_width;
	cli->fmt.height = screen_height;
	cli->fmt.data_port = h264_port + 1;
	cli->fmt.play_flag = 0x02;
	cli->fmt.bps = 4000000;
	cli->fmt.fps = 25;

	cli->fmt.play_flag = cli->play_flag = 2;    //控制状态
	send_options(cli->req, &(cli->fmt));
	clear_texture();
}


void play_vids(rfb_display *cli)
{
	cli->fmt.play_flag = cli->play_flag = -1;    //暂停状态 
	send_options(cli->req, &(cli->fmt));
}


void revert_all_vids()
{
	int i = 0;
	for(i = 0; i < display_size; i++)
	{
		if(displays[i].play_flag == -1)
		{
			play_vids(displays[i]);
		}
	}
}
#endif



int process_server_msg(rfb_request *req)
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
			//ret = control_msg(req);	
        case DONE:

        case DEAD:
        default:
            break;
    }
    return ret;
}




void init_server()
{
	int ret;
	pthread_t pthread_tcp, pthread_udp , pthread_display;

	display_size = window_size * window_size;
	run_flag = 1;

	create_display();
	
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

	DEBUG("server_s %d", server_s);

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


#ifndef DLL
	void *tret = NULL;
    pthread_join(pthread_display, &tret);  //等待线程同步
    DEBUG("pthread_exit %d display", (int)tret);
    pthread_join(pthread_tcp, &tret);  //等待线程同步
    DEBUG("pthread_exit %d tcp", (int)tret);
    pthread_join(pthread_udp, &tret);  //等待线程同步
    DEBUG("pthread_exit %d udp", (int)tret);
#endif
}


#if 0
void stop_control(rfb_request *req)
{
	pause_vids(req);
}
#endif


#if 0
void start_control(rfb_request *req)
{
	rfb_format fmt = {0};
	fmt.width = screen_width;
	fmt.height = screen_height;
	fmt.data_port = h264_port + 1;
	fmt.play_flag = 0x02;
	fmt.bps = 4000000;
	fmt.fps = 25;

	pause_all_vids();
	send_options(req, &fmt);
	clear_texture();
}
#endif

