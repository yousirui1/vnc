#include "base.h"

/* config.ini */
int server_flag = 0;
int client_port = -1, control_port = -1, h264_port = -1,  window_flag = 0, window_size = 0;
int default_fps = 12;
int default_size = 3;
int default_bps = 20000000;
int max_connections = 1024;
int server_port = -1;
char *server_ip = NULL;
int status = NORMAL;

time_t current_time;
const char program_name[] = "remote_monitor";

/* pipe */
int pipe_tcp[2] = {0};
int pipe_udp[2] = {0};
int pipe_event[2] = {0};

int init_config()
{
	int ret;
	char buf[128] = {0};
		
	ret = read_profile_string(BASE_SECTION, BASE_TYPE_KEY, buf, sizeof(buf), DEFAULT_TYPE, CONFIG_FILE);
	if(ret && !strncmp(buf, "server", 6))
	{
		server_flag = 1;
	}
	else
	{
		 server_flag = 0;
	}
    if(server_flag)     //服务端程序
    {
        client_port = read_profile_int(SERVER_SECTION, SERVER_CLIENT_PORT_KEY, DEFAULT_CLIENT_PORT_VALUE, CONFIG_FILE);
        control_port = read_profile_int(SERVER_SECTION, SERVER_CONTROL_PORT_KEY, DEFAULT_CONTROL_PORT_VALUE, CONFIG_FILE);
        h264_port = read_profile_int(SERVER_SECTION, SERVER_H264_PORT_KEY, DEFAULT_H264_PORT_VALUE, CONFIG_FILE);
        window_flag = read_profile_int(SERVER_SECTION, SERVER_WINDOW_FLAG_KEY, DEFAULT_WINDOW_FLAG_VALUE, CONFIG_FILE);
        window_size = read_profile_int(SERVER_SECTION, SERVER_WINDOW_SIZE_KEY, DEFAULT_WINDOW_SIZE_VALUE, CONFIG_FILE);

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
#endif
        }
        DEBUG("\nprograme server: \n client_port %d, control_port %d, h264_port %d, window_flag %d, window_size %d,",
                 client_port, control_port, h264_port, window_flag, window_size);
    }
    else                //客户端程序
    {
        ret = read_profile_string(CLIENT_SECTION, CLIENT_IP_KEY, buf, sizeof(buf), DEFAULT_IP_VALUE, CONFIG_FILE);
        server_ip = strdup(buf);
        server_port = read_profile_int(CLIENT_SECTION, CLIENT_PORT_KEY, DEFAULT_PORT_VALUE, CONFIG_FILE);
        //default_fps = read_profile_int(CLIENT_SECTION, CLIENT_FPS_KEY, DEFAULT_FPS_VALUE, CONFIG_FILE);
        DEBUG("\nprograme client: \n server_ip %s, server_port %d, window_flag %d, default_fps %d",
                server_ip, server_port, window_flag, default_fps);
    }
	return ret;	
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

    pipe_tcp[1] = create_tcp();
    ret = connect_server(pipe_tcp[1], "127.0.0.1", 22002);

    pipe_tcp[0] = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    close_fd(listenfd);

    listenfd = create_tcp();
    ret = bind_socket(listenfd, 22003);

    pipe_udp[1] = create_tcp();
    ret = connect_server(pipe_udp[1], "127.0.0.1", 22003);

    pipe_udp[0] = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    close_fd(listenfd);

#else
    /* create pipe to give main thread infomation */
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipe_tcp) < 0)
    {
        DEBUG("create server pipe err %s", strerror(errno));
        return ERROR;
    }

    fcntl(pipe_tcp[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_tcp[1], F_SETFL, O_NONBLOCK);

    /* create pipe to give main thread infomation */
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipe_udp) < 0)
    {
        DEBUG("create client pipe err %s", strerror(errno));
        return ERROR;
    }

    fcntl(pipe_udp[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_udp[1], F_SETFL, O_NONBLOCK);

    /* create pipe to give main thread infomation */
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipe_event) < 0)
    {
        DEBUG("create client pipe err %s", strerror(errno));
        return ERROR;
    }

    fcntl(pipe_event[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_event[1], F_SETFL, O_NONBLOCK);
#endif
    return ret;
}

int close_pipe()
{
    close_fd(pipe_event[0]);
    close_fd(pipe_event[1]);
    close_fd(pipe_tcp[0]);
    close_fd(pipe_tcp[1]);
    close_fd(pipe_udp[0]);
    close_fd(pipe_udp[1]);
}

static int do_exit()
{
	if(server_flag)
	{
		exit_server();
	}
	else
	{
		exit_client();
	}
	if(server_ip)
		free(server_ip);
	
	server_ip = NULL;
}

static void sig_quit_listen(int e)
{
	char s = 'S';
	send_msg(pipe_event[1], &s, 1);
	DEBUG("recv sig stop programe !!");
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR argv, int argc)
#else
int main(int argc, char *argv[])
#endif
{
    int ret;

start:
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
	init_logs();
	init_config();
	parse_options(argc, argv);
    if(server_flag)
    {
        ret = init_server();
    }
    else
    {
        ret = init_client();
    }
	do_exit();
	close_logs();

	goto start;
	
    return ret;
}
