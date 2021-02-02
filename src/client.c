#include "base.h"
#include "msg.h"

struct client m_client = {0};
rfb_display cli_display = {0};

static pthread_t pthread_tcp, pthread_event;
static int server_s = -1;

static void do_exit()
{
    void *tret = NULL;
    pthread_join(pthread_event, (void**)tret);  //等待线程同步
    DEBUG("pthread event exit ret: %d", (int)tret);
    pthread_join(pthread_tcp, (void**)tret);  //等待线程同步
    DEBUG("pthread client tcp exit ret: %d", (int)tret);

	close_fd(server_s);
	close_pipe();
	unload_wsa();
	close_X11();
	
	if(m_client.head_buf)
		free(m_client.head_buf);
	if(m_client.data_buf)
		free(m_client.data_buf);
	close_fd(m_client.control_udp.fd);
}

static int send_pipe(char *buf, short cmd, int size)
{
    int ret;
    set_request_head(buf, 0, cmd, size);
    ret = send_msg(pipe_tcp[0], buf, size + HEAD_LEN);
    return ret;
}

static int send_done(struct client  *req)
{
    int ret = ERROR;

    if(req->data_buf)
        free(req->data_buf);

    req->data_buf = malloc(sizeof(int) + 1);
    if(!req->data_buf)
        return ERROR;

    req->data_size = sizeof(int);

    *(int *)&req->data_buf[0] = 200;
    set_request_head(req->head_buf, 0, DONE_MSG_RET, sizeof(int));
    ret = send_request(req);

    DEBUG("done ok !!");

    req->status = OPTIONS;
    return ret;
}

static int recv_done(struct client  *req)
{

    if(read_msg_order(req->head_buf) == DONE_MSG)
    {
        int ret;
        ret = send_pipe(req->head_buf, CLI_DONE_MSG, 0);
		close_fd(cli_display.h264_udp.fd);
		close_fd(req->control_udp.fd);
        req->status = OPTIONS;
        //ret= send_done(req);
        return ret;
    }
    return ERROR;
}

static int send_ready(struct client  *req)
{
    int ret;
    if(req->data_buf)
        free(req->data_buf);

    req->data_buf = malloc(sizeof(int) + 1);
    if(!req->data_buf)
        return ERROR;

    req->data_size = sizeof(int);

    *(int *)&req->data_buf[0] = 200;
    set_request_head(req->head_buf, 0, READY_MSG, sizeof(int));
    ret = send_request(req);
    DEBUG("ready ok !!");

    req->status = DONE;
    return ret;
}

static int recv_ready(struct client  *req)
{
    int ret = ERROR;
    if(read_msg_order(req->head_buf) == READY_MSG_RET)
    {
		DEBUG("recv options ok!!");
        ret = send_pipe(req->head_buf, CLI_PLAY_MSG, 0);
        ret = send_ready(req);
    }
    return ret;
}

static int send_options(struct client  *req)
{
    int ret;
    if(req->data_buf)
        free(req->data_buf);

    req->data_buf = malloc(sizeof(int) + 1);

    if(!req->data_buf)
        return ERROR;

    *(int *)&req->data_buf[0] = 200;

    set_request_head(req->head_buf, 0, OPTIONS_MSG_RET, sizeof(int));
    req->data_size = sizeof(int);
    ret = send_request(req);

    if(SUCCESS != ret)
        return ERROR;

    req->status = READY;

    DEBUG("options ok !!");
    if(req->data_buf)
        free(req->data_buf);

    req->data_buf = NULL;

    return ret;
}

static int recv_options(struct client  *req)
{
    int ret = ERROR;
	DEBUG("recv_options");
    if(read_msg_order(req->head_buf) == OPTIONS_MSG_RET)
    {
        rfb_format *fmt = (rfb_format *)&req->data_buf[0];

        cli_display.play_flag = fmt->play_flag;

        DEBUG("fmt->width %d fmt->height %d fmt->data_port %d fmt->control_port %d fmt->play_flag %d  fmt->bps %d fmt->fps%d",

            fmt->width, fmt->height, fmt->data_port, fmt->control_port, fmt->play_flag, fmt->bps, fmt->fps);
        if(fmt->play_flag == 2)
            req->control_udp = create_udp(server_ip, fmt->control_port);

		if(fmt->play_flag)
		{
        	vids_width = fmt->width;
        	vids_height = fmt->height;

        	memcpy(&(cli_display.fmt), fmt, sizeof(rfb_format));
        	cli_display.cli = req;

        	ret = send_options(req);
		}
		else
		{
			DEBUG("options error status DONE ");
			status = DEAD;
		}
    }
    return ret;
}

static int recv_login(struct client  *req)
{
    int ret = ERROR;
    if(read_msg_order(req->head_buf) == LOGIN_MSG_RET)
    {
        int server_major = 0, server_minor = 0;
        int client_major = 0, client_minor = 0;
        sscanf(req->data_buf, VERSIONFORMAT, &server_major, &server_minor);
        DEBUG(VERSIONFORMAT, server_major, server_minor);
        get_version(&client_major, &client_minor);
        if(server_major != client_major || server_minor != client_minor)
        {
            DEBUG("version not equel server: "VERSIONFORMAT" client: "VERSIONFORMAT"", server_major,
                    server_minor, client_major, client_minor);
            ret = ERROR;
        }
		DEBUG("login ok !!");
        if(req->data_buf)
            free(req->data_buf);
        req->data_buf = NULL;
        req->status = OPTIONS;
    }
    return ret;
}


static int send_login(struct client  *req)
{
    int ret;
    int client_major = 0, client_minor = 0;

    get_version(&client_major, &client_minor);
    DEBUG("client version :"VERSIONFORMAT, client_major, client_minor);
    req->data_size = SZ_VERFORMAT;
    req->data_buf = (unsigned char *)malloc(req->data_size + 1);
	memset(req->data_buf, 0, SZ_VERFORMAT);
    sprintf(req->data_buf, VERSIONFORMAT, client_major, client_minor);

    set_request_head(req->head_buf, 0,  LOGIN_MSG, SZ_VERFORMAT);
    ret = send_request(req);
    if(req->data_buf)
        free(req->data_buf);
    req->data_buf = NULL;

    req->status = LOGIN;
    return ret;
}


int process_client_pipe(char *msg, int len)
{
    int ret = ERROR;
    switch(read_msg_order(msg))
    {
        case CLI_PLAY_MSG:
            ret = send_ready(&m_client);
            break;
        case CLI_DONE_MSG:
            ret = send_done(&m_client);
            break;
        defult:
            break;
    }
    return ret;
}

int process_client_msg(struct client  *req)
{
    int ret = 0;
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

void exit_client()
{
	do_exit();
}


int init_client()
{
    int ret = ERROR;

    init_X11();

    get_screen_size(&screen_width, &screen_height);

	ret = load_wsa();
	if(SUCCESS != ret)
    {   
        DEBUG("load wsa error");
        goto run_out;
    }   
    
	DEBUG("load wsa ok !!");
    ret = init_pipe();
    if(ret != SUCCESS)
    {   
        DEBUG("init pipe error");
        goto run_out;
    }  

	DEBUG("pipe init ok");
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
	DEBUG("connect server ip %d port %d ok", server_ip, server_port);
	memset(&m_client, 0, sizeof(struct client));
	m_client.fd = server_s;
	m_client.head_buf = malloc(HEAD_LEN + 1);
	if(!m_client.head_buf)
		goto run_out;
	memset(m_client.head_buf, 0, HEAD_LEN);
		
	ret = pthread_create(&pthread_event, NULL, thread_event, NULL);
    if(0 != ret)
    {   
        DEBUG("pthread create server event ret: %d error:%s", ret, strerror(ret));
        goto run_out;
    }   
    ret = pthread_create(&pthread_tcp, NULL, thread_client_tcp, &server_s);
    if(0 != ret)
    {
        DEBUG("pthread create client tcp ret: %d error: %s",ret, strerror(ret));
        goto run_out;
    }
	run_flag = 1;
    ret = send_login(&m_client);
    if(0 != ret)
    {
        DEBUG("send_login error");
        goto run_out;
    }
	return ret;
run_out:
    close_fd(server_s);
    return ret;
}
