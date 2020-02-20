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

    DEBUG("exit_server");
}

static int send_pipe(char *buf, short cmd, int size)
{
    int ret;
    set_request_head(buf, 0x0, cmd, size);
    ret = send_msg(pipe_ser[0], buf, size + HEAD_LEN);
    return ret;
}

static int recv_done(rfb_request *req)
{
	if(read_msg_order(req->head_buf) == DONE_MSG)
    {
        int ret, code;
        code = *(int *)&req->data_buf[0];
		DEBUG("done ok!!");
        if(code == 200)
        {
			req->status = OPTIONS;
            return SUCCESS;
        }
    }
    return ERROR;
}

static int send_done(rfb_request *req)
{
	int ret;
	set_request_head(req->head_buf, 0, DONE_MSG, 0);
    req->data_size = 0;
	ret = send_request(req);
	if(ret == SUCCESS)
	{
		DEBUG("send done ok !");
		req->status = OPTIONS;
		return SUCCESS;
	}	
	else
	{	
		req->status = DEAD;
		return ERROR;
	}
}


static int recv_ready(rfb_request *req)
{
    if(read_msg_order(req->head_buf) == READY_MSG)
    {
        int ret, code;
		char buf[128] = {0};
		char *tmp = &buf[HEAD_LEN];
        code = *(int *)&req->data_buf[0];
        if(code == 200)
        {
			DEBUG("ready is OK!!");
            *(int *)&tmp[0] = req->display_id;
			if(status == PLAY)
            	*(int *)&tmp[0] = req->display_id;
			else if(status == CONTROL)
            	*(int *)&tmp[0] = display_size;
            send_pipe(buf, SER_PLAY_MSG, sizeof(int));
			req->status = DONE;
            return SUCCESS;
        }
    }
    return ERROR;
}

static int send_ready(rfb_request *req)
{
	int ret;
	set_request_head(req->head_buf, 0, READY_MSG_RET, 0);
    req->data_size = 0;
	ret = send_request(req);
	if(ret == SUCCESS)
	{
		DEBUG("send ready ok !!");
		req->status = READY;
		return SUCCESS;
	}	
	else
	{	
		req->status = DEAD;
		return ERROR;
	}
}

static int recv_options(rfb_request *req)
{
    int ret = ERROR, code;
    if(read_msg_order(req->head_buf) == OPTIONS_MSG_RET)
    {
        code = *(int *)&req->data_buf[0];
        if(code == 200)
        {
			DEBUG("options OK !!");
            ret = send_ready(req);
        }
    }
	DEBUG("options error code %d", code);
    return ret;
}

static int send_options(rfb_request *req)
{
	int i, ret;
	req->data_buf = malloc(sizeof(rfb_format) + 1);
	if(!req->data_buf)
		return ERROR;
	rfb_format *fmt = (rfb_format *)&req->data_buf[0];
    req->data_size = sizeof(rfb_format);
    memset(fmt, 0, sizeof(rfb_format));
	
	if(status == PLAY)
	{
        for(i = 0; i < display_size; i++)   //ready play
        {
			//DEBUG("displays[i].play_flag %d", displays[i].play_flag);
            if(!displays[i].play_flag)      //存在空闲的分屏, 发送play命令
            {
                displays[i].req = req;
                displays[i].fmt.play_flag =  displays[i].play_flag = 1;
                fmt->width = vids_width;
                fmt->height = vids_height;
                fmt->play_flag = 0x01;
                fmt->data_port = i + h264_port;
                req->display_id = i;
                memcpy(&(displays[i].fmt), fmt, sizeof(rfb_format));
				req->status = OPTIONS;
                break;
            }
        }
	}
	if(status == CONTROL)
	{
        fmt->width = screen_width;
        fmt->height = screen_height;
        fmt->play_flag = 0x02;
        fmt->bps = 4000000;
        fmt->fps = 22;
        fmt->data_port = display_size + h264_port;
		fmt->control_port = control_port;
        req->status = OPTIONS;
	}	
	DEBUG("send options ok ");
	set_request_head(req->head_buf, 0, OPTIONS_MSG_RET, sizeof(rfb_format));
	ret = send_request(req);
	
	if(req->data_buf)
		free(req->data_buf);
	req->data_buf = NULL;
	return ret;	
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
	
	if(req->data_buf)	
		free(req->data_buf);
	req->data_buf = NULL;
	
	if(ret == SUCCESS)
		req->status = OPTIONS;
	else
		req->status = DEAD;
	return ret;	
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
			if(SUCCESS == ret)
			{
				DEBUG("client ip: %s login ok!!", req->ip);
                ret = send_options(req);
                return ret;
			}
		}
		DEBUG("version not equel server: "VERSIONFORMAT" client: "VERSIONFORMAT"", server_major,
                    server_minor, client_major, client_minor);
	}
	return ERROR;
}

int start_display()
{
	int i;
    /* 通知正在监控客户端停止发送 */
    for(i = 0; i < display_size; i++)
    {
        if(displays[i].req)
        {
        	displays[i].play_flag = 0;
			send_options(displays[i].req);
        }
    }
}

int stop_display()
{
    int i;
    char buf[128] = {0};
    /* 通知正在监控客户端停止发送 */
    for(i = 0; i < display_size; i++)
    {
        if(displays[i].play_flag == 1)
        {
            if(displays[i].req)
            {
				send_done(displays[i].req);
                /* 异步释放所有解码线程 */
                *(int *)&buf[HEAD_LEN] = i;
                send_pipe(buf, SER_DONE_MSG, sizeof(int));
            }
            displays[i].play_flag = 0;
        }
    }
    clear_texture();
}


int start_control(int id)
{
	if(!displays[id].req)
		return ERROR;

	control_display->req = displays[id].req;
	control_display->fmt.play_flag = control_display->play_flag = 2;    //控制状态
	control_display->fmt.data_port = h264_port + display_size;
	control_display->req->control_udp = create_udp(control_display->req->ip, control_port);	

	send_options(control_display->req);	
}

int stop_control()
{
    char buf[128] = {0};
	if(!control_display || !control_display->req)
	{
		return ERROR;	
	}
	control_display->fmt.play_flag = control_display->play_flag = 0; 
   	*(int *)&buf[HEAD_LEN] = display_size;
    send_pipe(buf, SER_DONE_MSG, sizeof(int));
	send_done(control_display->req);
	clear_texture();
}

int process_server_msg(rfb_request *req)
{
	int ret = ERROR;
	//DEBUG("process_server_msg req->status %d ", req->status);
	switch(req->status)
	{
		case NORMAL:
		case LOGIN:
			ret = recv_login(req);
			break;
		case OPTIONS:
			ret = recv_options(req);
			break;
		case READY:
			ret = recv_ready(req);
			break;
		case DONE:
			ret = recv_done(req);
			break;
		case DEAD:
			break;
		default:
			break;
	}	
	return ret;
}


void init_server()
{
	int ret = ERROR;
	int server_s = -1;
	
	if(window_size < 0 || window_size > 5)
		window_size = default_size;
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
