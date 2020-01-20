#include "base.h"
#include "msg.h"

rfb_request *client_req = NULL;
rfb_display cli_display = {0};

static pthread_t pthread_tcp, pthread_display;

static int send_pipe(char *buf, short cmd, int size)
{
    int ret;
    set_request_head(buf, 0, cmd, size);
    ret = send_msg(pipe_cli[0], buf, size + HEAD_LEN);
    return ret;
}

static int recv_options(rfb_request *req)
{
    if(read_msg_order(req->head_buf) == OPTIONS_MSG_RET)
    {
		int ret;
        rfb_format *fmt = (rfb_format *)&req->data_buf[0];
         DEBUG("fmt->width %d fmt->height %d fmt->code %d,"
          " fmt->data_port %d fmt->play_flag %d  fmt->bps %d fmt->fps%d",
             fmt->width, fmt->height,fmt->code, fmt->data_port,fmt->play_flag, fmt->bps, fmt->fps);

		cli_display.play_flag = fmt->play_flag;
		memcpy(&(cli_display.fmt), fmt, sizeof(rfb_format));
		ret = send_pipe(req->head_buf, CLI_UDP_MSG, 0);

		if(SUCCESS == ret)
		{
			req->status = PLAY;
		}
#if 0
		sleep(5);
		ret = send_pipe(req->head_buf, CLI_DONE_MSG, 0);
		char s = 'S';
		send_msg(pipe_event[1], &s, 1);
		
#endif
		return ret;
    }
	return ERROR;
}

static int send_options(rfb_request *req)
{
	int ret;
	if(req->data_size < sizeof(rfb_format))
	{
		DEBUG("send_options realloc");
		req->data_buf = realloc(req->data_buf, sizeof(rfb_format) + 1);
	}	
    req->data_size = sizeof(rfb_format);
    rfb_format *fmt = (rfb_format *)&req->data_buf[0];
    fmt->width = screen_width;
    fmt->height = screen_height;
    fmt->code = 0;
    fmt->data_port = 0;
    fmt->play_flag = 0;
    fmt->bps = 2000000;
    fmt->fps = default_fps;

    set_request_head(req->head_buf, 0, OPTIONS_MSG, sizeof(rfb_format));
    req->status = OPTIONS;
	ret = send_request(req);
	free(req->data_buf);	
	
	if(SUCCESS != ret)
        return ERROR;

    req->status = OPTIONS;
    return SUCCESS;
}

static int recv_login(rfb_request *req)
{
    if(read_msg_order(req->head_buf) == LOGIN_MSG_RET)
    {
		int ret;
        int server_major = 0, server_minor = 0;
		int client_major = 0, client_minor = 0;
        sscanf(req->data_buf, VERSIONFORMAT, &server_major, &server_minor);
        DEBUG(VERSIONFORMAT, server_major, server_minor);
		get_version(&client_major, &client_minor);
		if(server_major == client_major || server_minor == client_minor)
        {
        	send_options(req);
            if(ret == SUCCESS)
            {
                DEBUG("login ok !!");
                return SUCCESS;
            }
        }
        else
        {
            DEBUG("version not equel server: "VERSIONFORMAT" client: "VERSIONFORMAT"", server_major,
                    server_minor, client_major, client_minor);
        }
        free(req->data_buf);
    }
	return ERROR;
}

static int send_login(rfb_request *req)
{
    int ret;
	int client_major = 0, client_minor = 0;

	get_version(&client_major, &client_minor);
	DEBUG("client version :"VERSIONFORMAT, client_major, client_minor);
	
    req->data_size = sz_verformat;
    req->data_buf = (unsigned char *)malloc(req->data_size + 1);
    sprintf(req->data_buf, VERSIONFORMAT, client_major, client_minor);

	set_request_head(req->head_buf, 0,  LOGIN_MSG, SZ_VERFORMAT);
    ret = send_request(req);
    free(client_req->data_buf);

	if(SUCCESS != ret)
		return ERROR;	

    req->status = LOGIN;
    return SUCCESS;
}


int process_client_pipe(char *msg, int len)
{
    int ret;
    DEBUG("pipe msg type %02X CLI_READY_MSG %02X", read_msg_order(msg), CLI_READY_MSG);
    switch(read_msg_order(msg))
    {
        case CLI_READY_MSG:
            //send_ready(&m_client);
            break;
        case CLI_PLAY_MSG:
            //send_play(&m_client);
            break;
    }
}

int process_client_msg(rfb_request *req)
{
    int ret = 0;
	DEBUG("process_client_msg   !!!");
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


static void do_exit()
{
    void *tret = NULL;
    pthread_join(pthread_tcp, (void**)tret);  //等待线程同步
	DEBUG("pthread_exit client tcp");
}

void init_client()
{
    int ret = -1;
	int server_s;

	get_screen_size(&screen_width, &screen_height);

    server_s = create_tcp();
    if(server_s == -1)
    {
        DEBUG("create tcp err");
		return;
    }

    ret = connect_server(server_s, server_ip, server_port);
	if(0 != ret)
	{
		DEBUG("connect server ip: %s port %d error", server_ip, server_port);
		goto run_out;
	}
	
	client_req = new_request();
	if(!client_req)
	{
		//DEBUG("malloc cli->head_buf sizeof: %d error :%s", HEAD_LEN, strerror(errno));
		goto run_out;
	}
	client_req->fd = server_s;

	ret = pthread_create(&pthread_tcp, NULL, thread_client_tcp, &server_s);
    if(0 != ret)
    {
		DEBUG("pthread create client tcp ret: %d error: %s",ret, strerror(ret));
		goto run_out;
    }
	ret = send_login(client_req);
	if(0 != ret)
	{
		DEBUG("send_login error");
		goto run_out;
	}
	do_exit();

run_out:
	close_fd(server_s);
	return;
}
