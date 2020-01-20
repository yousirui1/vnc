#include "base.h"
#include "msg.h"


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
	}
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

