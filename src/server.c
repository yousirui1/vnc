#include "base.h"
#include "msg.h"

int display_size = 0;

void *thread_tcp(void *param);
void *thread_udp(void *param);

void init_server(int window_size)
{
    int ret;
    pthread_t pthread_tcp, pthread_udp;

    display_size = window_size * window_size;

    ret = pthread_create(&pthread_tcp, NULL, thread_tcp, NULL);
    if(0 != ret)
    {
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    }

 	ret = pthread_create(&pthread_udp, NULL, thread_udp, NULL);
    if(0 != ret)
    {
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    }

    sleep(10000);
}

void *thread_tcp(void *param)
{
    int ret, c;
    /* 设置线程优先级 */
    pthread_attr_t attr;
    struct sched_param sched;

    ret = pthread_attr_init(&attr);
    if(ret)
        DEBUG("thread_tcp attr init error");
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if(ret)
        DEBUG("thread tcp set SCHED_FIFO error");
    sched.sched_priority = SCHED_PRIORITY_TCP;
    ret = pthread_attr_setschedparam(&attr, &sched);
    if(ret)
        DEBUG("Set sched_priority 2 error");

    int server_s = create_server_socket();
	max_connections = 100;
#if 0
    if(max_connections < 1)
    {
        struct rlimit rl;
        /* has not been set explicitly */
        c = getrlimit(RLIMIT_NOFILE, &rl);
        if(c < 0)
        {
            DIE("getrlimit");
        }
        max_connections = rl.rlim_cur;
    }
#endif
    DEBUG("max_connections %d tcp_loop %d", max_connections, server_s);
    tcp_loop(server_s);
}

void *thread_udp(void *param)
{
    int ret, i;
    /* 设置线程优先级 */
    pthread_attr_t attr;
    struct sched_param sched;

    ret = pthread_attr_init(&attr);
    if(ret)
        DEBUG("thread_tcp attr init error");
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if(ret)
        DEBUG("thread tcp set SCHED_FIFO error");
    sched.sched_priority = SCHED_PRIORITY_UDP;
    ret = pthread_attr_setschedparam(&attr, &sched);
    if(ret)
        DEBUG("Set sched_priority 2 error");
 
	create_h264_socket(display_size, displays);
}





int process_msg(rfb_request *req)
{

    int ret = 0;

    switch(req->status)
    {
        case READ_HEADER:
        case LOGIN:
            ret = read_login(req);
            break;
        case OPTIONS:
            ret = read_options(req);
            break;
        case DONE:

        case DEAD:
        default:
            break;
    }
	//return 1;

	return 0;
}


int read_login(rfb_request *req)
{
    if(read_msg_order(req->head_buf) == 0x01)
    {
        int server_major, server_minor;

        if (sscanf(req->data_buf, VERSIONFORMAT,
                    &server_major, &server_minor) != 2)
                ;
        sprintf(req->data_buf, VERSIONFORMAT, 3, server_minor + 1);
        send_request(req);
        req->status = OPTIONS;
		return 0;
    }      
    else
    {
        DEBUG("read_login %d", read_msg_order(req->head_buf));
		return 1;
    }
}

int read_options(rfb_request *req)
{
   	if(read_msg_order(req->head_buf) == 0x02)
   	{
       	int i ;
       	rfb_format *fmt = (rfb_format *)&req->data_buf[0];
       	DEBUG("fmt->width %d, fmt->height %d fmt->code %d fmt->data_port %d", fmt->width, fmt->height, fmt->code, fmt->data_port);

       	fmt->play_flag = 0;
       	fmt->data_port = 0;
       	for(i = 0; i < display_size; i++)   //ready play
       	{
           	if(!displays[i].play_flag)      //存在空闲的分屏, 发送play命令
           	{
               	displays[i].req = req;
               	displays[i].play_flag = 1;
               	fmt->width = 480;
               	fmt->height = 320;
               	fmt->play_flag = 0x01;
               	fmt->vnc_flag = 0x00;       //副码流
               	fmt->data_port = i + h264_port + 1;
               	req->display_id = i;
               	DEBUG("req->display_id %d", i);
               	break;
           	}
       	}
       	send_request(req);
       	req->status = DONE;
		return 0;
    }
	else
	{
        DEBUG("read_login %d", read_msg_order(req->head_buf));
		return 1;
	}
}





