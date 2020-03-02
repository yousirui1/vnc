#include "base.h"
int display_size = 0;
static int server_s = -1;
static pthread_t pthread_tcp, pthread_udp, pthread_sdl, pthread_event;
struct client **clients = NULL;

static int send_pipe(char *buf, short cmd, int size)
{
	int ret = ERROR;
	set_request_head(buf, 0x0, cmd, size);
    ret = send_msg(pipe_tcp[0], buf, size + HEAD_LEN);
    return ret;
}

static int recv_done(struct client *cli)
{
	if(read_msg_order(cli->head_buf) == DONE_MSG)
	{
		int code = *(int *)&cli->data_buf[0];
		if(code == 200)
		{
			DEBUG("done code 200 ok !!");
			cli->status = OPTIONS;
			return SUCCESS;
		}
	}
	return ERROR;
}

static int send_done(struct client *cli)
{
	int ret;
	set_request_head(cli->head_buf, 0, DONE_MSG, 0);
	cli->data_size = 0;
	ret = send_request(cli);
	cli->status = DONE;
	return ret;
}

static int recv_ready(struct client *cli)
{
	if(read_msg_order(cli->head_buf) == READY_MSG)
	{
		int ret, code;
		char buf[128] = {0};
		char *tmp = &buf[HEAD_LEN];
		code =  *(int *)&cli->data_buf[0];
		if(code == 200)
		{
			DEBUG("ready is ok !!");
			*(int *)&tmp[0] = cli->display_id;
			if(status == PLAY)
                *(int *)&tmp[0] = cli->display_id;
            else if(status == CONTROL)
                *(int *)&tmp[0] = display_size;
			send_pipe(buf, SER_PLAY_MSG, sizeof(int));
            cli->status = DONE;
            return SUCCESS;
		}	

	}
	return ERROR;
}

static int send_ready(struct client *cli)
{
	int ret;
	set_request_head(cli->head_buf, 0, READY_MSG_RET, 0);
	cli->data_size = 0;
	ret = send_request(cli);
    DEBUG("send ready ok !!");
    cli->status = READY;
	return ret;
}


static int recv_options(struct client *cli)
{
	int ret = ERROR;
	if(read_msg_order(cli->head_buf) == OPTIONS_MSG_RET)
	{
		int code = *(int *)&cli->data_buf[0];
		if(code == 200)
		{
			DEBUG("options ok !!");	
			ret = send_ready(cli);
		}
	}
	return ret;
}


static int send_options(struct client *cli)
{
	int i, ret;
	if(cli->data_buf)
		free(cli->data_buf);
	cli->data_buf = malloc(sizeof(rfb_format) + 1);
	if(!cli->data_buf)
		return ERROR;
	rfb_format *fmt = (rfb_format *)&cli->data_buf[0];
	cli->data_size = sizeof(rfb_format);
	memset(fmt, 0, sizeof(rfb_format));

	if(PLAY == status)
	{
		for(i = 0; i <= display_size; i++)   //ready play
        {
            if(!displays[i].play_flag)      //存在空闲的分屏, 发送play命令
            {
                displays[i].cli = cli;
                displays[i].fmt.play_flag =  displays[i].play_flag = 1;
                fmt->width = vids_width;
                fmt->height = vids_height;
                fmt->play_flag = 0x01;
                fmt->data_port = i + h264_port;
                cli->display_id = i;
                memcpy(&(displays[i].fmt), fmt, sizeof(rfb_format));
                cli->status = OPTIONS;
				DEBUG("fmt->width: %d fmt->height %d", fmt->width , fmt->height);
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
        cli->status = OPTIONS;
	}
	DEBUG("send options ok !!");
	set_request_head(cli->head_buf, 0, OPTIONS_MSG_RET, sizeof(rfb_format));
	ret = send_request(cli);
	
	if(cli->data_buf)
		free(cli->data_buf);	
	cli->data_buf = NULL;
	return ret;
}



static int send_login(struct client *cli)
{
	int ret;
    int server_major = 0, server_minor = 0;
    get_version(&server_major, &server_minor);
    sprintf(cli->data_buf, VERSIONFORMAT, server_major, server_minor);	
	cli->data_size = SZ_VERFORMAT;
	set_request_head(cli->head_buf, 0, LOGIN_MSG_RET, SZ_VERFORMAT);
	ret = send_request(cli);
	
	cli->status = OPTIONS;
	return ret;
}

static int recv_login(struct client *cli)
{
	if(read_msg_order(cli->head_buf) == LOGIN_MSG)
	{
		int ret;
		int server_major = 0, server_minor = 0;
        int client_major = 0, client_minor = 0;
		if(server_major == client_major && server_minor == client_minor)
        {
            ret = send_login(cli);
            if(SUCCESS == ret)
            {
                DEBUG("client ip: %s login ok!!", cli->ip);
                ret = send_options(cli);
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
        if(displays[i].cli)
        {
            displays[i].play_flag = 0;
            send_options(displays[i].cli);
        }
    }
}

int stop_display()
{
    int i;
    char buf[128] = {0};
	DEBUG("stop display all client");

	if(!displays)
	{
		DEBUG("display param is NULL!!");
		return ERROR;
	}

    /* 通知正在监控客户端停止发送 */
    for(i = 0; i < display_size; i++)
    {
        if(displays && displays[i].play_flag == 1)
        {
            if(displays[i].cli)
            {
                send_done(displays[i].cli);
                /* 异步释放所有解码线程 */
                *(int *)&buf[HEAD_LEN] = i;
                send_pipe(buf, SER_DONE_MSG, sizeof(int));
            }
            displays[i].play_flag = 0;
        }
    }
    clear_texture();
	return SUCCESS;
}

int start_control(int id)
{
    if(!displays[id].cli)
        return ERROR;

    control_display->cli = displays[id].cli;
    control_display->fmt.play_flag = control_display->play_flag = 2;    //控制状态
    control_display->fmt.data_port = h264_port + display_size;
    control_display->cli->control_udp = create_udp(control_display->cli->ip, control_port);

    send_options(control_display->cli);
}

int stop_control()
{
    char buf[128] = {0};
    if(!control_display || !control_display->cli)
    {
        return ERROR;
    }
    control_display->fmt.play_flag = control_display->play_flag = 0;
    *(int *)&buf[HEAD_LEN] = display_size;
    send_pipe(buf, SER_DONE_MSG, sizeof(int));
    send_done(control_display->cli);
    clear_texture();
}

int process_server_msg(struct client *cli)
{
    int ret = ERROR;
    switch(cli->status)
    {
        case NORMAL:
        case LOGIN:
            ret = recv_login(cli);
            break;
        case OPTIONS:
            ret = recv_options(cli);
            break;
        case READY:
            ret = recv_ready(cli);
            break;
        case DONE:
            ret = recv_done(cli);
            break;
        case DEAD:
            break;
        default:
            break;
    }
    return ret;
}

/* ------------------------------------------------ */
static void do_exit()
{
	int i;
	void *tret = NULL;

	pthread_join(pthread_tcp, &tret);  //等待线程同步
    DEBUG("pthread_exit %d tcp", (int)tret);
    pthread_join(pthread_udp, &tret);  //等待线程同步
    DEBUG("pthread_exit %d udp", (int)tret);
    pthread_join(pthread_sdl, &tret);  //等待线程同步
    DEBUG("pthread_exit %d display", (int)tret);
    pthread_join(pthread_event, &tret);  //等待线程同步
    DEBUG("pthread_exit %d event", (int)tret);

	close_fd(server_s);
	close_pipe();
	//unload_wsa();
	for(i = 0; i < max_connections; i++)
	{
		if(clients[i])
		{
			close_fd(clients[i]->fd);
			if(clients[i]->head_buf)
				free(clients[i]->head_buf);
			if(clients[i]->data_buf)
				free(clients[i]->data_buf);
			free(clients[i]);	
			clients[i] = NULL;
		}
	}
	display_size = 0;
	server_s = -1;
	if(clients)
		free(clients);
	clients = NULL;
}


void exit_server()
{
	do_exit();
}

int init_server()
{
	int ret = ERROR;

	if(window_size <= 0 || window_size > 5)
	{
		DEBUG("window size: %d error", window_size);
		return ERROR;
	}

	clients = (struct client **)malloc(sizeof(struct client *) * max_connections);
	if(!clients)
	{
		DEBUG("clients malloc error %s", strerror(errno));
		return ERROR;
	}
	memset(clients, 0, sizeof(struct client *) * max_connections);

	ret = load_wsa();
	if(SUCCESS != ret)
	{
		DEBUG("load wsa error");
		goto run_out;
	}
	
	ret = init_pipe();
	if(ret != SUCCESS)
	{
		DEBUG("init pipe error");
		goto run_out;
	}
	
	server_s = create_tcp();
	if(server_s == -1)
	{
		DEBUG("create tcp error");
		goto run_out;
	}
	
	if(bind_socket(server_s, client_port) == -1)
    {
        DEBUG("bind port %d err", client_port);
        goto run_out;
    }

	pthread_mutex_init(&display_mutex, NULL);
	pthread_cond_init(&display_cond, NULL);

	ret = pthread_create(&pthread_sdl, NULL, thread_sdl, NULL);
    if(0 != ret)
    {
		DEBUG("pthread create server sdl ret: %d error:%s", ret, strerror(ret));
        goto run_out;
    }
	ret = pthread_create(&pthread_event, NULL, thread_event, NULL);
    if(0 != ret)
    {
		DEBUG("pthread create server event ret: %d error:%s", ret, strerror(ret));
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

	return ret;
run_out:
	do_exit();
	return ret;
}
