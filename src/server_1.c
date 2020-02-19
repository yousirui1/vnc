#include "base.h"
#include "msg.h"

int display_size = 0;
static pthread_t pthread_tcp, pthread_udp , pthread_display;

static void do_exit()
{
    void *tret = NULL;
    pthread_join(pthread_tcp, &tret);  //等待线程同步
    DEBUG("pthread_exit %d tcp", (int)tret);
    pthread_join(pthread_udp, &tret);  //等待线程同步
    DEBUG("pthread_exit %d udp", (int)tret);

    pthread_join(pthread_display, &tret);  //等待线程同步
    DEBUG("pthread_exit %d display", (int)tret);
	destroy_socket();
	//destroy_display();
	DEBUG("exit_server");
}

static int send_pipe(char *buf, short cmd, int size)
{
    int ret = -1;
    set_request_head(buf, 0x0, cmd, size);
    ret = send_msg(pipe_ser[0], buf, size + HEAD_LEN);
    return ret;
}

static int send_done(rfb_request *req)
{
	int ret;
	req->status = OPTIONS;
	set_request_head(req->head_buf, 0, DONE_MSG, 0);	
	req->data_size = 0;
	ret = send_request(req);	
	DEBUG("send_done ");
    return ret;
}

#if 0
static int send_control(rfb_request *req)
{
	DEBUG("send_control");
	int ret = -1;
	set_request_head(req->head_buf, 0, 0x02, sizeof(rfb_format));	
	req->data_size = sizeof(rfb_format);
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
	set_request_head(req->head_buf, 0, 0x02, sizeof(rfb_format));	
	req->data_size = sizeof(rfb_format);

	if(!req->data_buf)
		req->data_buf = (unsigned char *)malloc(sizeof(rfb_format));	

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
#endif

static int send_ready(rfb_request *req)
{
	int ret;
	unsigned char *buf = NULL;
	*(int *)&req->data_buf[0] = PLAY;
	set_request_head(req->head_buf, 0, READY_MSG_RET, sizeof(int));
	ret = send_request(req);
	buf = (unsigned char *)malloc(sizeof(int) + HEAD_LEN + 1);
	if(!buf)
		return ERROR;

	//set_request_head(buf, 0, SER_PLAY_MSG, sizeof(int));
	//*(int *)&buf[HEAD_LEN] = 0;
	
	//ret = send_pipe(buf, SER_PLAY_MSG, sizeof(int));
		
	if(SUCCESS != ret)
		return ERROR;
	if(status == CONTROL)	
		req->status = CONTROL;
	else
		req->status = PLAY;
	//status = PLAY;
	//DEBUG("
	return SUCCESS;
}

static int recv_ready(rfb_request *req)
{
		DEBUG("recv_ready 11111111111111111");
	if(read_msg_order(req->head_buf) == READY_MSG)
	{
		DEBUG("recv_ready 00000000000000000");
		int ret, code;
		code = *(int *)&req->data_buf[0];
		if(code == 200)
		{
			ret = send_ready(req);	
			return SUCCESS;	
		}
	}
	return ERROR;	
}


static int send_options(rfb_request *req)
{
	int i , ret;
	char buf[128] = {0};
	char *tmp = &buf[HEAD_LEN];
	if(req->data_buf)
		free(req->data_buf);
    req->data_buf = malloc(sizeof(rfb_format) + 1);
	if(!req->data_buf)
		return ERROR;

	rfb_format *fmt = (rfb_format *)&req->data_buf[0];
	req->data_size = sizeof(rfb_format);
	memset(fmt, 0, sizeof(rfb_format));
	DEBUG("status %d PLAY %d", status, PLAY);
	if(status == PLAY)
	{
		for(i = 0; i < display_size; i++)   //ready play
        {
            if(!displays[i].play_flag)      //存在空闲的分屏, 发送play命令
            {
                displays[i].req = req;
                displays[i].fmt.play_flag =  displays[i].play_flag = 1;
                fmt->width = vids_width;
                fmt->height = vids_height;
                fmt->play_flag = 0x01;
                fmt->data_port = i + h264_port + 1;
                req->display_id = i;
				memcpy(&(displays[i].fmt), fmt, sizeof(rfb_format));
				*(int *)&tmp[0] = i;
				send_pipe(buf, SER_PLAY_MSG, sizeof(int));
                DEBUG("req->display_id %d", i);
                break;
            }
        }
		DEBUG("displays[1].play_flag %d", displays[1].play_flag);
		DEBUG("total_connections %d", total_connections);
		if(fmt->play_flag)
		{
			req->status = READY;
		}
	}
	if(status == CONTROL)
	{
    	fmt->width = screen_width;
    	fmt->height = screen_height;
    	fmt->play_flag = 0x02;
    	fmt->bps = 4000000;
    	fmt->fps = 22;
    	fmt->data_port = control_display->fmt.data_port;
    	req->status = READY;
	}
	set_request_head(req->head_buf, 0, OPTIONS_MSG_RET, sizeof(rfb_format));	
    ret = send_request(req);

    free(req->data_buf);
    req->data_buf = NULL;

    return ret;
}


static int recv_options(rfb_request *req)
{
	if(read_msg_order(req->head_buf) == OPTIONS_MSG)
	{

	}
}


static int send_login(rfb_request *req)
{
    int ret;
    int server_major = 0, server_minor = 0;
    get_version(&server_major, &server_minor);
    sprintf(req->data_buf, VERSIONFORMAT, server_major, server_minor);
	req->data_size = SZ_VERFORMAT;
    set_request_head(req->head_buf, 0, LOGIN_MSG_RET, SZ_VERFORMAT);
    ret = send_request(req);

    if(SUCCESS != ret)
        return ERROR;
	if(req->data_buf)
		free(req->data_buf);

    req->status = OPTIONS;
    return SUCCESS;
}

static int recv_login(rfb_request *req)
{
	if(read_msg_order(req->head_buf) == LOGIN_MSG)
	{
		int ret;
		int server_major = 0, server_minor = 0;
		int client_major = 0, client_minor = 0;
		get_version(&server_major, &server_minor);
		sscanf((char *)req->data_buf, VERSIONFORMAT, &client_major, &client_minor);
		if(server_major == client_major && server_minor == client_minor)
		{
			ret = send_login(req);
			if(ret == SUCCESS)
			{
				DEBUG("client ip: %s login ok!!", req->ip);
				ret = send_options(req);
                return ret;
			}
		}
		else
		{
            DEBUG("version not equel server: "VERSIONFORMAT" client: "VERSIONFORMAT"", server_major,
                    server_minor, client_major, client_minor);
		}		
	}
  	return ERROR; 
}


int process_server_msg(rfb_request *req)
{
    int ret = 0;
	DEBUG("process_server_msg req->status %d READY %d", req->status, READY);
    switch(req->status)
    {
		case NORMAL:
        case LOGIN:
            ret = recv_login(req);
            break;
        case OPTIONS:					//被动请求开始显示
            ret = recv_options(req);
            break;
		case READY:
			ret = recv_ready(req);
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
	DEBUG("process_server_msg ret %d", ret);
    return ret;
}

void start_display(rfb_display *cli)
{

}

int stop_display()
{
	char buf[128] = {0};
	/* 通知正在监控客户端停止发送 */
	int i;
	for(i = 0; i < display_size; i++)
	{
        if(displays[i].play_flag == 1)
        {
            if(displays[i].req)
			{
				displays[i].req->status = DONE;
                process_server_msg(displays[i].req);

				/* 异步释放所有解码线程 */
				*(int *)&buf[HEAD_LEN] = i;
				send_pipe(buf, SER_DONE_MSG, sizeof(int));
			}
            displays[i].play_flag = 0;
        }
	}
	clear_texture();
}

void revert_display()
{

}

int start_control(int id)
{
	if(!displays[id].req)
		return ERROR;
   	control_display = &displays[id];
    control_display->fmt.play_flag = control_display->play_flag = 2;    //控制状态
    displays[id].req->status = CONTROL;
    //process_server_msg(displays[id].req);
	send_options(displays[id].req);
}

void stop_control()
{

}
void init_server()
{
	int ret = -1;
	int server_s = -1;
	
	if(window_size < 0 || window_size > 5)
	{
		window_size = default_size;
	}

	display_size = window_size * window_size;

	server_s = create_tcp();
	if(server_s == -1)
	{
		DEBUG("pthread create display ret: %d error:%s", ret, strerror(ret));
        goto run_out;
	}	

	if(bind_socket(server_s, client_port) == -1)
    {
        DEBUG("bind port %d err", client_port);
        goto run_out;
    }

	ret = create_display();
	if(ret != SUCCESS)
	{
	    DEBUG("pthread create display ret: %d error:%s", ret, strerror(ret));
        goto run_out;
	}
	
	ret = init_display();
	if(ret != SUCCESS)
	{
        DEBUG("pthread create display ret: %d error:%s", ret, strerror(ret));
        goto run_out;
	}

#if 0
	ret = pthread_create(&pthread_display, NULL, thread_display, NULL);
    if(0 != ret)
    {
        DEBUG("pthread create display ret: %d error:%s", ret, strerror(ret));
        goto run_out;
    }
#endif

    ret = pthread_create(&pthread_tcp, NULL, thread_server_tcp, &server_s);
    if(0 != ret)
    {
        DEBUG("pthread create server tcp ret: %d error:%s", ret, strerror(ret));
        goto run_out;

    }

    ret = pthread_create(&pthread_udp, NULL, thread_server_udp, NULL);
    if(0 != ret)
    {
        DEBUG("pthread create server udp ret: %d error:%s", ret, strerror(ret));
        goto run_out;
    }

	sdl_loop();
	do_exit();
run_out:
	close_fd(server_s);
	return;
}


