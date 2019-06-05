#include "base.h"

#if 0
time_t current_time;
static int pipeid[2];
#endif

int server_flag = 0;
int client_port, control_port, h264_port, window_flag, window_size, server_port;
char server_ip[126] = {0};


int main(int argc, char *argv[])
{
	int ret;
	char *opt_input_file;

	parse_options(argc, argv, opt_input_file);

	create_display(window_size);

	pthread_attr_t st_attr;
    struct sched_param sched;
       
    ret = pthread_attr_init(&st_attr);
    if(ret)
    {   
        DIE("ThreadMain attr init error ");
    }   
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);

    sched.sched_priority = SCHED_PRIORITY_DATAPROC;
    ret = pthread_attr_setschedparam(&st_attr, &sched);

	if(server_flag)
	{
		init_server(window_size);
	}
	else
	{
		init_client();
	}
}



void parse_options(int argc, char **argv, char *file)
{
	/* arg > file */
	int ret;	
	char buf[126];

	ret = read_profile_string(BASE_SECTION, BASE_TYPE_KEY, buf, sizeof(buf), DEFAULT_TYPE, CONFIG_FILE);
	if(!strncmp(buf, "server", 6))
	{
		server_flag = 1;
	}

	/* 读取对应所需要的配置信息 */			
	if(server_flag)		//服务端程序
	{
		client_port = read_profile_int(SERVER_SECTION, SERVER_CLIENT_PORT_KEY, DEFAULT_CLIENT_PORT_VALUE, CONFIG_FILE);
		control_port = read_profile_int(SERVER_SECTION, SERVER_CONTROL_PORT_KEY, DEFAULT_CONTROL_PORT_VALUE, CONFIG_FILE);
		h264_port = read_profile_int(SERVER_SECTION, SERVER_H264_PORT_KEY, DEFAULT_H264_PORT_VALUE, CONFIG_FILE);
		window_flag = read_profile_int(SERVER_SECTION, SERVER_WINDOW_FLAG_KEY, DEFAULT_WINDOW_FLAG_VALUE, CONFIG_FILE);
		window_size = read_profile_int(SERVER_SECTION, SERVER_WINDOW_SIZE_KEY, DEFAULT_WINDOW_SIZE_VALUE, CONFIG_FILE);
	}
	else				//客户端程序
	{
		ret = read_profile_string(CLIENT_SECTION, CLIENT_IP_KEY, server_ip, sizeof(server_ip), DEFAULT_IP_VALUE, CONFIG_FILE);
		server_port = read_profile_int(CLIENT_SECTION, CLIENT_PORT_KEY, DEFAULT_PORT_VALUE, CONFIG_FILE);
	}
	DEBUG("server_flag %d, client_port %d, control_port %d, h264_port %d, window_flag %d, window_size %d, server_ip %s, server_port %d", server_flag, client_port, control_port, h264_port, window_flag, window_size, server_ip, server_port);
}

void init_client()
{
	int ret;
	pthread_t pthread_socket, pthread_decode;
	
#if 0
	ret = pthread_create(&pthread_socket, NULL, client_socket, NULL);
    if(0 != ret)
    {   
        DIE("ThreadDisp err %d,  %s",ret , strerror(ret));
    }   

    ret = pthread_create(&pthread_decode, NULL, thread_encode, NULL);
    if(0 != ret)
    {   
        DIE("ThreadDisp err %d,  %s",ret , strerror(ret));
    }  
#endif
	int ** tret;
  	pthread_join(pthread_socket, (void**)tret);  //等待线程同步
}

void init_server()
{
	

}
