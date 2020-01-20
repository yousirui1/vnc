#include "base.h"

/* config.ini */
int server_flag = 0;
int client_port = -1, control_port = -1, h264_port = -1,  window_flag = 0, window_size = 0;
int default_fps = 0;
int max_connections = -1; 
int server_port = -1;
char *server_ip = NULL;
int run_flag = 0;
int status = NORMAL;

time_t last_time;
time_t current_time;
const char program_name[] = "remote monitor";

/* pipe */
int pipe_ser[2] = {0};
int pipe_cli[2] = {0};
int pipe_event[2] = {0};

/* pthread */
static pthread_t pthread_event;

int init_config()
{
    int ret = 0;
    char buf[126] = {0};
	
    ret = read_profile_string(BASE_SECTION, BASE_TYPE_KEY, buf, sizeof(buf), DEFAULT_TYPE, CONFIG_FILE);
    if(ret && !strncmp(buf, "server", 6))
    {   
        server_flag = 1;
    }
	else
	{
		server_flag = 0;
	}
    /* 读取对应所需要的配置信息 */
    if(server_flag)     //服务端程序
    {   
        client_port = read_profile_int(SERVER_SECTION, SERVER_CLIENT_PORT_KEY, DEFAULT_CLIENT_PORT_VALUE, CONFIG_FILE);
        control_port = read_profile_int(SERVER_SECTION, SERVER_CONTROL_PORT_KEY, DEFAULT_CONTROL_PORT_VALUE, CONFIG_FILE);
        h264_port = read_profile_int(SERVER_SECTION, SERVER_H264_PORT_KEY, DEFAULT_H264_PORT_VALUE, CONFIG_FILE);
        window_flag = read_profile_int(SERVER_SECTION, SERVER_WINDOW_FLAG_KEY, DEFAULT_WINDOW_FLAG_VALUE, CONFIG_FILE);
        window_size = read_profile_int(SERVER_SECTION, SERVER_WINDOW_SIZE_KEY, DEFAULT_WINDOW_SIZE_VALUE, CONFIG_FILE);
        DEBUG("\nprograme server: \n client_port %d, control_port %d, h264_port %d, window_flag %d, window_size %d,",
                 client_port, control_port, h264_port, window_flag, window_size);
    }
    else                //客户端程序
    {   

        ret = read_profile_string(CLIENT_SECTION, CLIENT_IP_KEY, buf, sizeof(buf), DEFAULT_IP_VALUE, CONFIG_FILE);
		server_ip = strdup(buf);	
        server_port = read_profile_int(CLIENT_SECTION, CLIENT_PORT_KEY, DEFAULT_PORT_VALUE, CONFIG_FILE);
        default_fps = read_profile_int(CLIENT_SECTION, CLIENT_FPS_KEY, DEFAULT_FPS_VALUE, CONFIG_FILE);
        DEBUG("\nprograme client: \n server_ip %s, server_port %d, window_flag %d, default_fps %d",
                server_ip, server_port, window_flag, default_fps);
    }

	if(max_connections < 1)
	{
#ifndef _WIN32
        struct rlimit rl;
        /* has not been set explicity */
        ret = getrlimit(RLIMIT_NOFILE, &rl);
        if(ret < 0)
        {
            DEBUG("getrlimit %s", strerror(errno));
            return ERROR;
        }
        max_connections = rl.rlim_cur;
#else
        max_connections = 1024;
#endif
	}
	
	return SUCCESS;	
}


void parse_options(int argc, char *argv[])
{
    int ret = 0;
    char buf[126] = {0};
    switch(argc)
    {   
        case 3:
            server_port = atoi(argv[2]);
        case 2:
            server_ip = strdup(argv[1]); 
        default:
            break;
    }   

    if(!server_ip || server_port >= 65535 || server_port <= 0)
    {   
   		ret = read_profile_string(CLIENT_SECTION, CLIENT_IP_KEY, buf, sizeof(buf), DEFAULT_IP_VALUE, CONFIG_FILE);
		server_ip = strdup(buf);	
        server_port = read_profile_int(CLIENT_SECTION, CLIENT_PORT_KEY, DEFAULT_PORT_VALUE, CONFIG_FILE);
    }   
}

int init_pipe()
{
    int ret = SUCCESS;
#ifdef _WIN32
    int listenfd = 0;
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    listenfd = create_tcp();
    ret = bind_socket(listenfd, 22001);

    pipe_event[1] = create_tcp();
    ret = connect_server(pipe_event[1], "127.0.0.1", 22001);

    pipe_event[0] = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    close_fd(listenfd);

    listenfd = create_tcp();
    ret = bind_socket(listenfd, 22002);

    pipe_ser[1] = create_tcp();
    ret = connect_server(pipe_ser[1], "127.0.0.1", 22002);

    pipe_ser[0] = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    close_fd(listenfd);

    listenfd = create_tcp();
    ret = bind_socket(listenfd, 22003);

    pipe_cli[1] = create_tcp();
    ret = connect_server(pipe_cli[1], "127.0.0.1", 22003);

    pipe_cli[0] = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    close_fd(listenfd);

    DEBUG("pipe_event[0] %d pipe_event[1] %d", pipe_event[0], pipe_event[1]);
    DEBUG("pipe_ser[0] %d pipe_ser[1] %d", pipe_ser[0], pipe_ser[1]);
    DEBUG("pipe_cli[0] %d pipe_cli[1] %d", pipe_cli[0], pipe_cli[1]);

#else
    /* create pipe to give main thread infomation */
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipe_ser) < 0)
    {
        DEBUG("create server pipe err %s", strerror(errno));
        return ERROR;
    }

    fcntl(pipe_ser[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_ser[1], F_SETFL, O_NONBLOCK);

    /* create pipe to give main thread infomation */
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipe_cli) < 0)
    {
        DIE("create client pipe err %s", strerror(errno));
    }

    fcntl(pipe_cli[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_cli[1], F_SETFL, O_NONBLOCK);

    /* create pipe to give main thread infomation */
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipe_usb) < 0)
    {
        DIE("create usb pipe err %s", strerror(errno));
    }

    fcntl(pipe_usb[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_usb[1], F_SETFL, O_NONBLOCK);

    /* create pipe to give main thread infomation */
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipe_thrift) < 0)
    {
        DIE("create thrift  pipe err %s", strerror(errno));
    }

    fcntl(pipe_thrift[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_thrift[1], F_SETFL, O_NONBLOCK);

    /* create pipe to give main thread infomation */
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipe_event) < 0)
    {
        DIE("create event pipe err %s", strerror(errno));
    }

    fcntl(pipe_event[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_event[1], F_SETFL, O_NONBLOCK);
#endif
    return ret;
}


void close_pipe()
{
    close_fd(pipe_event[0]);
    close_fd(pipe_event[1]);
    close_fd(pipe_ser[0]);
    close_fd(pipe_ser[1]);
}



void sig_quit_listen(int e)
{
    char s = 'S';

    //write( pipeid[1], &s,  sizeof(s));
    DEBUG("recv stop msg");
    return;
}

void do_exit()
{
    void *tret = NULL;

    pthread_join(pthread_event, &tret);
    DEBUG("pthread event exit ret: %d", (int *)tret);

    if(server_ip)
        free(server_ip);
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR argv, int argc)
#else
int main(int argc, char *argv[])
#endif
{
	int ret;
	(void) time(&current_time);	
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
    //signal(SIGINT, SIG_IGN);

    struct sigaction act;
    act.sa_handler = sig_quit_listen;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, 0);
#endif

	run_flag = 1;

    init_logs();
    /* config */
    init_config();
    parse_options(argc, argv);
	
    ret = load_wsa();
    if(SUCCESS != ret)
    {
        DEBUG("load wsa error");
        goto run_out;
    }

    ret = init_pipe();
    if(SUCCESS != ret)
    {
        DEBUG("init pipe error");
        goto run_out;
    }
    DEBUG("init pipe ok !!!");

	ret = pthread_create(&pthread_event, NULL, thread_event, NULL);
	if(0 != ret)
	{
        DEBUG("create pthread event ret: %d error: %s", ret, strerror(ret));
        goto run_out;
	}

    if(server_flag)
    {
        init_server();
    }
    else
    {
        init_client();
    }
	
	do_exit();
run_out:
	unload_wsa();
	close_pipe();
	close_logs();
	return 0;
}


