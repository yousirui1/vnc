#include "base.h"
#include "msg.h"

static pthread_t pthread_cli_udp, pthread_cli_encode;

static void do_exit()
{
	int i;
	void *tret = NULL;
	if(!server_flag)
	{
		pthread_cancel(pthread_cli_udp);
        pthread_cancel(pthread_cli_encode);
        pthread_join(pthread_cli_udp, &tret);
        pthread_join(pthread_cli_encode, &tret);
	}
	else
	{
		for(i = 0; i <= display_size; i++)
		{
			pthread_cancel(displays[i].pthread_decode);
			pthread_join(displays[i].pthread_decode, &tret);
		}	
	}
}

static int send_pipe(char *buf, short cmd, int size, int msg_type)
{
	int ret = ERROR;
	set_request_head(buf, 0x0, cmd, size);
	switch(msg_type)
	{
		case CLIENT_MSG:
			ret = send_msg(pipe_cli[1], buf, size + HEAD_LEN);
			break;
		case SERVER_MSG:
			ret = send_msg(pipe_cli[1], buf, size + HEAD_LEN);
			break;
		default:
			break;
	}
	return ret;
}

static int process_ser(char *msg)
{
	int ret = ERROR;
	int index =  *(int *)&msg[HEAD_LEN];
	void *tret = NULL;
	
	if(index < 0 || index > display_size)
		return ERROR;
			
	switch(read_msg_order(msg))
	{
		case SER_PLAY_MSG:				//创建解码线程
			ret = pthread_create(&displays[index].pthread_decode, NULL, thread_decode, &displays[index]);	
			break;	
		case SER_DONE_MSG:				//断开连接 销毁解码线程
			DEBUG("SER_DONE_MSG !!!!!!!!!!!");
			pthread_cancel(displays[index].pthread_decode);
			pthread_join(displays[index].pthread_decode, &tret);
			break;	
		default:
			break;
	}
	return ret;
}

static int process_cli(char *msg)
{
	int ret = ERROR;
	void *tret = NULL;
	switch(read_msg_order(msg))
	{
		case CLI_PLAY_MSG:
			ret = pthread_create(&pthread_cli_udp, NULL, thread_client_udp, &cli_display.fmt.data_port);
			ret = pthread_create(&pthread_cli_encode, NULL, thread_encode, &cli_display.fmt);
			break;
		case CLI_DONE_MSG:
			pthread_cancel(pthread_cli_udp);
            pthread_cancel(pthread_cli_encode);
            pthread_join(pthread_cli_udp, &tret);
            pthread_join(pthread_cli_encode, &tret);
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
                process_ser(buf);
            }
            if(--nready <= 0)
                continue;
        }
		
		if(FD_ISSET(pipe_cli[1], &reset))
        {
            ret = recv(pipe_cli[1], (void *)buf, sizeof(buf), 0);
            if(ret >= HEAD_LEN)
            {
                process_cli(buf);
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
