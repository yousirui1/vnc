#include "base.h"

static pthread_t pthread_cli_udp, pthread_cli_encode;

static void do_exit()
{
	int i = 0;
	void *tret = NULL;
	if(server_flag)
	{
		if(displays)
		{
			pthread_cond_wait(&display_mutex, &display_cond);
			for(i = 0; i <= display_size  && display_size != 0; i++)
			{
				pthread_cancel(displays[i].pthread_decode);
            	pthread_join(displays[i].pthread_decode, &tret);
				if(vids_buf[i])
            		free(vids_buf[i]);
				if(displays[i].h264_udp.fd)
					close_fd(displays[i].h264_udp.fd);
				vids_buf[i] = NULL;
			}
        	free(displays);
		}

		if(vids_queue)
			free(vids_queue);
		if(vids_buf)
			free(vids_buf);
    	displays = NULL;
		vids_buf = NULL;
		vids_queue = NULL;
	}
	else
	{
        pthread_cancel(pthread_cli_udp);
        pthread_cancel(pthread_cli_encode);
        pthread_join(pthread_cli_udp, &tret);
        pthread_join(pthread_cli_encode, &tret);
        close_fd(cli_display.h264_udp.fd);
    }
    DEBUG("thread event end !!");
}

static int send_pipe(char *buf, short cmd, int size)
{
	int ret = ERROR;
	set_request_head(buf, 0x0, cmd, size);
	ret = send_msg(pipe_tcp[1], buf, size + HEAD_LEN);
	return ret;
}

static int process_udp(char *msg)
{

}

static int process_tcp(char *msg)
{
	int ret = ERROR;
	void *tret = NULL;
	int index;
	switch(read_msg_order(msg))
	{
		case SER_PLAY_MSG:
		{
			index = *(int *)&msg[HEAD_LEN];
			if(index < 0 || index > display_size || !displays)
				break;
			ret = pthread_create(&displays[index].pthread_decode, NULL, thread_decode, &displays[index]);
			break;
		}
		case SER_DONE_MSG:
		{
			index = *(int *)&msg[HEAD_LEN];
			if(index < 0 || index > display_size || !displays)
				break;
			DEBUG("SER_DONE_MSG %d index", index);
			pthread_cancel(displays[index].pthread_decode);
			
			pthread_join(displays[index].pthread_decode, &tret);
			DEBUG("server decode thread exit");
			break;
		}

		case CLI_UDP_MSG:
		{
			break;
		}
		case CLI_PLAY_MSG:
		{
			DEBUG("CLI_PLAY_MSG");
    		cli_display.h264_udp = create_udp(server_ip, cli_display.fmt.data_port);
			ret = pthread_create(&pthread_cli_encode, NULL, thread_encode, &cli_display.fmt);
			if(cli_display.fmt.play_flag == 2)
				ret = pthread_create(&pthread_cli_udp, NULL, thread_client_udp, &cli_display.fmt.data_port);			
			break;
		} 
		case CLI_CONTROL_MSG:
		{
			ret = pthread_create(&pthread_cli_udp, NULL, thread_client_udp, &cli_display.fmt.data_port);			
			break;
		} 
		case CLI_DONE_MSG:
		{
			DEBUG("CLI_DONE_MSG exit start ----------");
			char s = 'S';
			send_msg(pipe_udp[1], &s, 1);

				
        	//pthread_cancel(pthread_cli_udp);
        	pthread_cancel(pthread_cli_encode);
			DEBUG("CLI_DONE_MSG exit middle ===========");
        	pthread_join(pthread_cli_encode, &tret);
			DEBUG("CLI_DONE_MSG encode exit end +++++");
        	pthread_join(pthread_cli_udp, &tret);			//ysr
			DEBUG("CLI_DONE_MSG udp exit end +++++");
			close_fd(cli_display.h264_udp.fd);
			DEBUG("close cli_display.h264_udp.fd %d", cli_display.h264_udp.fd);
			break;
		} 
		default:
			break;
	}
	return ret;
}

void event_loop()
{
	int maxfd = 0, ret, nready;
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	
	fd_set reset, allset;

    FD_ZERO(&allset);
    FD_SET(pipe_tcp[1], &allset);
    FD_SET(pipe_udp[1], &allset);
    FD_SET(pipe_event[0], &allset);

    maxfd = maxfd > pipe_tcp[1] ? maxfd : pipe_tcp[1];
    maxfd = maxfd > pipe_udp[1] ? maxfd : pipe_udp[1];
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
		nready = ret;

        /* pipe msg */
        if(FD_ISSET(pipe_tcp[1], &reset))
        {
            ret = recv(pipe_tcp[1], (void *)buf, sizeof(buf), 0);
            if(ret >= HEAD_LEN)
            {
                process_tcp(buf);
            }
            if(--nready <= 0)
                continue;
        }

        if(FD_ISSET(pipe_udp[1], &reset))
        {
            ret = recv(pipe_udp[1], (void *)buf, sizeof(buf), 0);
            if(ret >= HEAD_LEN)
            {
                process_udp(buf);
            }
            if(--nready <= 0)
                continue;
        }

        if(FD_ISSET(pipe_event[0], &reset))
        {
            ret = recv(pipe_event[0], (void *)buf, sizeof(buf), 0);
			DEBUG("pipe_event msg %d ----------", ret);
            if(ret == 1)
            {
                if(buf[0] == 'S')
                {
                    /* 通知其他线程关闭 */
                    send_msg(pipe_tcp[1], &buf[0], 1);
                    send_msg(pipe_udp[1], &buf[0], 1);
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
        DEBUG("Thread Event attr init warning ");
    }
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {
        DEBUG("Thread Event set SCHED_FIFO warning");
    }
    sched.sched_priority = SCHED_PRIORITY_EVENT;
    ret = pthread_attr_setschedparam(&st_attr, &sched);

    event_loop();
    return (void *)SUCCESS;
}

