#include "base.h"
#include "msg.h"

static pthread_t pthread_cli_udp, pthread_cli_encode;


static void do_exit()
{
    void *tret = NULL;
    /* client pthread*/
    if(!server_flag)
    {
        pthread_join(pthread_cli_udp, (void **)tret);
        pthread_join(pthread_cli_encode, (void **)tret);
    }
	else
	{

	}
}

static int send_pipe(char *buf, short cmd, int size, int msg_type)
{
    int ret = SUCCESS;
    set_request_head(buf, 0x0, cmd, size);
    switch(msg_type)
    {
        case CLIENT_MSG:
            ret = send_msg(pipe_cli[1], buf, size + HEAD_LEN);
            break;
        case SERVER_MSG:
            ret = send_msg(pipe_ser[1], buf, size + HEAD_LEN);
            break;
    }
    return ret;
}

static int process_ser(char *msg, int len)
{
    int ret;
    int index = 0;
    switch(read_msg_order(msg))
    {
        case SER_PLAY_MSG:          //创建解码线程
        {
            //ret = pthread_create(&displays[index].pthread_decode, NULL, thread_decode, &displays[index]);
            //ret = pthread_create(display[index]->pthread_udp, NULL, thread_server_udp);
            break;
        }
        case SER_DONE_MSG:          //断开连接 销毁解码线程
        {
#if 0
            if(display[index] && display[index]->pthread_decode)
            {
                pthread_cancel(client[index]->pthread_decode);
                free(client[index]->pthread_decode);
            }
#endif
            break;
        }
    }
}

#if 0
if(cli_display.fmt.play_flag == 1)
	{
		//req->status = PLAY;
    	create_encode(&cli_display.fmt);
	}

	if(cli_display.fmt.play_flag == 2)
	{
		/* 获取服务端分辨率用于控制坐标转换 */
		vids_width = cli_display.fmt.width;   
		vids_height = cli_display.fmt.height;
		//req->status = CONTROL;
    	create_encode(&cli_display.fmt);
	}
    int ret;
    pthread_t pthread_udp, pthread_encode;

    ret = pthread_create(&pthread_udp, NULL, thread_client_udp, fmt);
    if(0 != ret)
    {
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    }
    ret = pthread_create(&pthread_encode, NULL, thread_encode, fmt);
    if(0 != ret)
    {
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    }

#endif


static int process_cli(char *msg, int len)
{
    char *tmp = &msg[HEAD_LEN];
    int ret;
    //struct client *cli = &m_client;
    switch(read_msg_order(msg))
    {
        case CLI_UDP_MSG:                   //创建udp 线程
        {
            ret = pthread_create(&pthread_cli_udp, NULL, thread_client_udp, &cli_display.fmt);
			ret = pthread_create(&pthread_cli_encode, NULL, thread_encode, &cli_display.fmt);
	        send_pipe(msg, CLI_READY_MSG, 0, CLIENT_MSG);
            break;
        }
        case CLI_PLAY_MSG:                  //创建 编码线程
        {
            DEBUG("CLI_PLAY_MSG");
            //ret = pthread_create(&pthread_cli_encode, NULL, thread_encode, (void *)&cli->fmt);
            send_pipe(msg, CLI_PLAY_MSG, 0, CLIENT_MSG);
            break;
        }
        case CLI_CONTROL_MSG:
        {
            //ret = pthread_create(&pthread_cli_encode, NULL, thread_encode, (void *)&cli->fmt);
            //send_pipe(msg, CLI_PLAY_MSG, 0, CLIENT_MSG);
            break;
        }
        case CLI_DONE_MSG:
        case CLI_DEAD_MSG:                  //销毁udp和编码 线程
            pthread_cancel(pthread_cli_udp);
            pthread_cancel(pthread_cli_encode);

            break;
        default:
            break;
    }
    return ret;
}

/* 线程的创建和检测释放 */
void event_loop()
{
    int maxfd = 0, ret, nready;
    struct timeval tv;

    fd_set reset, allset;

    FD_ZERO(&allset);
    FD_SET(pipe_ser[1], &allset);
    FD_SET(pipe_cli[1], &allset);
    FD_SET(pipe_event[0], &allset);

    maxfd = maxfd > pipe_ser[1] ? maxfd : pipe_ser[1];
    maxfd = maxfd > pipe_cli[1] ? maxfd : pipe_cli[1];
    maxfd = maxfd > pipe_event[0] ? maxfd : pipe_event[0];

    char buf[DATA_SIZE] = {0};
    char *tmp = &buf[HEAD_LEN];

    for(;;)
    {
        reset = allset;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(maxfd + 1, &reset, NULL, NULL, &tv);
		DEBUG("event loop");
        if(ret == -1)
        {
            if(errno == EINTR)
                continue;
            else if(errno != EBADF)
            {
                DEBUG("select error: %s", strerror(errno));
            }
        }
        /* pipe msg */
        if(FD_ISSET(pipe_ser[1], &reset))
        {
            ret = recv(pipe_ser[1], (void *)buf, sizeof(buf), 0);
            if(ret >= HEAD_LEN)
            {
                process_ser(buf, ret);
            }
            if(--nready <= 0)
                continue;
        }
        if(FD_ISSET(pipe_cli[1], &reset))
        {
            ret = recv(pipe_cli[1], (void *)buf, sizeof(buf), 0);
            if(ret >= HEAD_LEN)
            {
                process_cli(buf, ret);
            }
            if(--nready <= 0)
                continue;
        }

        if(FD_ISSET(pipe_event[0], &reset))
        {
            ret = recv(pipe_event[0], (void *)buf, sizeof(buf), 0);
            if(ret == 1)
            {
                if(buf[0] == 'S')
                {
					DEBUG("event thread end");
                    /* 通知其他线程关闭 */
					send_msg(pipe_cli[1], &buf[0], 1);
					send_msg(pipe_ser[1], &buf[0], 1);
                    break;
                }
            }
            if(--nready <= 0)
                continue;
        }
	}
	do_exit();
}


void *thread_event(void *param)
{
    int ret;
    pthread_attr_t st_attr;
    struct sched_param sched;

    ret = pthread_attr_init(&st_attr);
    if(ret)
    {
        DEBUG("Thread Event attr init error ");
    }
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {
        DEBUG("Thread Event set SCHED_FIFO error");
    }
    sched.sched_priority = SCHED_PRIORITY_THRIFT;
    ret = pthread_attr_setschedparam(&st_attr, &sched);

    event_loop();
    return (void *)0;
}

