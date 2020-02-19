#define KEY_CAPS_LOCK 1<<13

#define KEY_LEFT_SHITF 1<<1
#define KEY_RIGHT_SHITF 1<<2

#define KEY_LEFT_CTRL 1<<9
#define KEY_RIGHT_CTRL 1<<10

#define KEY_LEFT_ALT 1<<9
#define KEY_RIGHT_ALT 1<<10

#define KEY_LEFT_WIN 1<<7
#define KEY_RIGHT_WIN 1<<8


/*
 *    // close caps lock  13bit  left 1bit right 1bit
 *  |-1bit-| -2bit-|-2bit-|-2bit-|-2bit-|-2bit-|-2bit-|
 *    flag    win     alt   ctrl                 shift
 *
 *  // open caps lock  14bit  left 1bit right 1bit
 *  |-1bit-|-1bit-| -2bit-|-2bit-|-2bit-|-2bit-|-2bit-|-2bit-|
 *    flag   caps    win     alt    ctrl                 shift
 *
 */


    if(key->mod > 0x1000)
    {
        if(key->mod > 0x3000)   //open caps lock  14bit
        {
            keybd_event(VK_CAPITAL, 0, key->down, 0);
        }
        if(key->mod & KEY_LEFT_SHITF || key->mod & KEY_RIGHT_SHITF)
        {
            keybd_event(VK_SHIFT, 0, key->down, 0);
            //DEBUG("ctrl key");
        }
        if(key->mod & KEY_LEFT_CTRL || key->mod & KEY_RIGHT_CTRL)
        {
            if(first_time)
            {
                first_time = 0;
                DEBUG("ctrl key");
                keybd_event(VK_CONTROL, 0, key->down, 0);
            }
        }

        if(key->mod & KEY_LEFT_WIN )
        {
            if(first_time)
            {
                keybd_event(VK_LWIN, 0, key->down, 0);
            }
            //DEBUG("ctrl key");
        }
        if(key->mod & KEY_RIGHT_WIN)
        {
            keybd_event(VK_RWIN, 0, key->down, 0);
            //DEBUG("ctrl key");
        }
        if(key->mod & KEY_LEFT_ALT || key->mod & KEY_RIGHT_ALT)
        {
            keybd_event(VK_MENU, 0, key->down, 0);
            //DEBUG("ctrl key");
        }
    }
#if 0
void pause_vids(rfb_display *cli)
{
	cli->fmt.play_flag = cli->play_flag = -1;    //暂停状态 
	send_options(cli->req, &(cli->fmt));
}

void pause_all_vids()
{
	int i = 0;
	for(i = 0; i < display_size; i++)
	{
		if(displays[i].play_flag == 1)
		{
			pause_vids(displays[i]);
		}
	}
}

void start_control(rfb_display *cli)
{
	cli->fmt.width = screen_width;
	cli->fmt.height = screen_height;
	cli->fmt.data_port = h264_port + 1;
	cli->fmt.play_flag = 0x02;
	cli->fmt.bps = 4000000;
	cli->fmt.fps = 25;

	cli->fmt.play_flag = cli->play_flag = 2;    //控制状态
	send_options(cli->req, &(cli->fmt));
	clear_texture();
}


void play_vids(rfb_display *cli)
{
	cli->fmt.play_flag = cli->play_flag = -1;    //暂停状态 
	send_options(cli->req, &(cli->fmt));
}


void revert_all_vids()
{
	int i = 0;
	for(i = 0; i < display_size; i++)
	{
		if(displays[i].play_flag == -1)
		{
			play_vids(displays[i]);
		}
	}
}
#endif


#if 0
void stop_control(rfb_request *req)
{
	pause_vids(req);
}
#endif


#if 0
void start_control(rfb_request *req)
{
	rfb_format fmt = {0};
	fmt.width = screen_width;
	fmt.height = screen_height;
	fmt.data_port = h264_port + 1;
	fmt.play_flag = 0x02;
	fmt.bps = 4000000;
	fmt.fps = 25;

	pause_all_vids();
	send_options(req, &fmt);
	clear_texture();
}
#endif
//#include <X11/Xlib.h>
//#include <X11/Xutil.h>      /* BitmapOpenFailed, etc.    */
//#include <X11/cursorfont.h> /* pre-defined crusor shapes */

#include "base.h"

static int fd_kbd = 0;
static int fd_mouse  = 0;
//static Display *dpy = NULL;
//static Window root;


int init_dev()
{
#if 0
	int id;
	if((dpy = XOpenDisplay(0)) == NULL)
	{
		DEBUG("");
		goto err;
	}
	id = DefaultScreen(dpy);  //DISPLAY:= id
	if(!(root = XRootWindow(dpy, id)))
	{
		DEBUG("");
		goto err;
	}
	screen_width = DisplayWidth(dpy, id);
	screen_height = DisplayHeight(dpy, id);

	fd_kbd = open("/dev/input/event1", O_RDWR);
	if(fd_kbd <= 0)
	{
		DEBUG("");
		goto err;
	}

	fd_mouse = open("/dev/input/event2", O_RDWR);
	if(fd_mouse <= 0)
	{
		DEBUG("");
		goto err;
	}	
	return 0;
err:
	if(fd_kbd)
		close(fd_kbd);
	if(fd_mouse)
		close(fd_mouse);
	if(dpy)
		XCloseDisplay(dpy);
	return -1;
#endif
}

#if 0
void get_position(int *x, int *y)
{
	int tmp;
	unsigned int tmp2;
	Window fromroot, tmpwin;
	XQueryPointer(dpy, root, &fromroot, &tmpwin, x, y, &tmp, &tmp, &tmp2);	
}


void simulate_mouse(int x, int y)
{
	int ret;
	ret = XWarpPointer(dpy, None, root, 0, 0, 0, 0, x, y);
	XFlush(dpy);
	DEBUG("XWarpPointer ret %d, %s", ret, strerror(ret));
	//mouseClick(Button3);
}
#if 0
typedef struct {
    int type;       /* of event */
    unsigned long serial;   /* # of last request processed by server */
    Bool send_event;    /* true if this came from a SendEvent request */
    Display *display;   /* Display the event was read from */
    Window window;          /* "event" window reported relative to */
    Window root;            /* root window that the event occured on */
    Window subwindow;   /* child window */
    Time time;      /* milliseconds */
    int x, y;       /* pointer x, y coordinates in event window */
    int x_root, y_root; /* coordinates relative to root */
    unsigned int state; /* key or button mask */
    char is_hint;       /* detail */
    Bool same_screen;   /* same screen flag */
} XMotionEvent;
typedef XMotionEvent XPointerMovedEvent;
#endif
void mouseClick(int button)
{
	XEvent event;
	memset(&event, 0x00, sizeof(event));

	event.type = ButtonPress;
	event.xbutton.button = button;
	event.xbutton.same_screen = True;
	
	XQueryPointer(dpy, RootWindow(dpy, DefaultScreen(dpy)), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);

	event.xbutton.subwindow = event.xbutton.window;
	while(event.xbutton.subwindow)
	{
    	event.xbutton.window = event.xbutton.subwindow;
    	XQueryPointer(dpy, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
	}

   	if(XSendEvent(dpy, PointerWindow, True, 0xfff, &event) == 0) printf("Errore nell'invio dell'evento !!!\n");
    XFlush(dpy);

    sleep(1);

    event.type = ButtonRelease;
    event.xbutton.state = 0x100;

    if(XSendEvent(dpy, PointerWindow, True, 0xfff, &event) == 0) printf("Errore nell'invio dell'evento !!!\n");

    XFlush(dpy);
}

#if 0
void simulate_mouse(int x, int y)
{
    
    /* EV_KEY ¼üÅÌ 
     * EV_REL Ïà¶Ô×ø±ê Êó±ê
     * EV_ABS ¾ø¶Ô×ø±ê ÊÖÐ´°å
     */
    //unsigned long x = (rel_x * (1920 / 960.0));
    //unsigned long y = (rel_y *  (1080 / 720.0));

    struct input_event event;
	int current_x, current_y;

	unsigned long rel_x = 0;
	unsigned long rel_y = 0;

	get_position(&current_x, &current_y);
	DEBUG("current_x %d current_y %d", current_x, current_y);

	rel_x = x - current_x;			
	rel_y = y - current_y;			


    memset(&event, 0, sizeof(event));
    gettimeofday(&event.time, NULL);
    event.type = EV_REL;
    event.code = REL_X;
    event.value = rel_x;
    write(fd_mouse, &event, sizeof(event));
    event.type = EV_REL;
    event.code = REL_Y;
    event.value = rel_y;
    write(fd_mouse, &event, sizeof(event));
    event.type = EV_SYN;
    event.code = SYN_REPORT;
    event.value = 0;
    write(fd_mouse, &event, sizeof(event));

	get_position(&current_x, &current_y);
	DEBUG("current_x %d current_y %d", current_x, current_y);
}
#endif
#endif


#include "base.h"
#include "msg.h"
#include "queue.h"

static int sockfd = 0;
static struct sockaddr_in send_addr,recv_addr;
static int total_connections = 0;

/* h264 data queue */
unsigned char **vids_buf = NULL;
QUEUE *vids_queue = NULL;

rfb_request *request_ready = NULL;   //ready list head


void *thread_client_udp(void *param)
{
    int ret;
    pthread_attr_t st_attr;
    struct sched_param sched;
    
    rfb_format *fmt = (rfb_format *)param;

    ret = pthread_attr_init(&st_attr);
    if(ret)
    {   
        DEBUG("ThreadMain attr init error ");
    }   
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {   
        DEBUG("ThreadMain set SCHED_FIFO error");
    }   
    sched.sched_priority = SCHED_PRIORITY_UDP;
    ret = pthread_attr_setschedparam(&st_attr, &sched);

    create_udp(server_ip, fmt->data_port, 0x0);
    client_udp_loop();
}


void *thread_client_tcp(void *param)
{
    int ret;
    pthread_attr_t st_attr;
    struct sched_param sched;
    
    ret = pthread_attr_init(&st_attr);
    if(ret)
    {   
        DEBUG("ThreadMain attr init error ");
    }   
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {   
        DEBUG("ThreadMain set SCHED_FIFO error");
    }   
    sched.sched_priority = SCHED_PRIORITY_UDP;
    ret = pthread_attr_setschedparam(&st_attr, &sched);

    client_tcp_loop();
}











unsigned char read_msg_syn(unsigned char* buf)
{
    return *(unsigned char*)&buf[DATA_SYN_OFFSET];
}

unsigned short read_msg_order(unsigned char * buf)
{
    return *(unsigned short *)&buf[DATA_ORDER_OFFSET];
}

int read_msg_size(unsigned char * buf)
{
    return *(int*)&buf[DATA_LEN_OFFSET];
}

int recv_msg(const int fd,char* buf, const int len)
{   
    char * tmp = buf;
    int cnt = 0; 
    int read_cnt = 0; 
    while(read_cnt != len)
    {   
        cnt = recv(fd,tmp + read_cnt ,len - read_cnt,0);
        if(cnt == 0)
        {   
            return 1;
        }
        else
        {   
            if(cnt < 0)
            {   
                if(errno == EINTR || errno == EAGAIN)
                {   
                    continue;
                }
                return 1;
            }
        }
        read_cnt += cnt;
    }
    return 0;
}

    
    
int send_msg(const int fd, const char *buf, const int len)
{   
    const char* tmp = buf;
    int cnt = 0;
    int send_cnt = 0;
    while(send_cnt != len)
    {
        cnt =  send(fd, tmp + send_cnt ,len - send_cnt,0);
        if(cnt < 0)
        {
            if(errno == EINTR || errno == EAGAIN)
                continue;
            return 1;
        }
        send_cnt += cnt;
    }
    return 0;
} 



rfb_request *new_request()
{
	rfb_request* req = (rfb_request *) malloc(sizeof (rfb_request));
    if(!req)
    {
        DEBUG("malloc for new request");
        return NULL;
    }
    memset(req, 0, sizeof (rfb_request));

	req->display_id = -1;
	req->data_buf = NULL;
	
    return req;
}


rfb_request* remove_request(rfb_request *req)
{
    rfb_request *next = req->next;
	
    if(req->data_buf)
        free(req->data_buf);
    req->data_buf = NULL;

    if(req->display_id != -1)
    {
        displays[req->display_id].req = NULL;
        displays[req->display_id].play_flag = 0;
    }  

    dequeue(&request_ready, req);

    req->next = NULL;
    req->prev = NULL;
    free(req);
    total_connections--;
    return next;
}

int send_request(rfb_request *req)
{
    if(!req || !req->fd)
        return 0;

    char *tmp = malloc(HEAD_LEN + req->data_size + 1);
    if(!tmp)
    {
        return 0;
    }
    memcpy(tmp, req->head_buf, HEAD_LEN);
    memcpy(tmp + HEAD_LEN, req->data_buf, req->data_size);
    send_msg(req->fd, tmp, HEAD_LEN + req->data_size);
	DEBUG("send_request %d fd", req->status, req->fd);

    free(tmp);
}



int create_server_socket(void)
{   
    int server_s, sock_opt = -1;
    struct sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof server_sockaddr);
    
#ifdef _WIN32
 	WSADATA wsData = {0};
    if(0 != WSAStartup(0x202, &wsData))
    {
        DEBUG("WSAStartup  fail");
        WSACleanup();
        return -1;
    }

 	server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//DWORD retBytes;
	//tcp_keepalive inkeepAlive;
	//tcp_keepalive rekeepAlive;
	
	//inkeepAlive.onoff = 1;					 //探测次数
	//inkeepAlive.keepalivetime = 5500;        // 首次探测开始前的tcp无数据收发空闲时间	
	//inkeepAlive.keepaliveinterval = 3000;    // 每次探测的间隔时间
	

#if 0
	if(WSAIoctl(server_s, SIO_KEEPALIVE_VALS,
			&inkeepAlive, sizeof(inkeepAlive),
			&rekeepAlive, sizeof(rekeepAlive),
			&retBytes, NULL, NULL) != 0)
	{
		DIE("WSAIoctl Error: %d", WSAGetLastError());
	}
#endif
	
#else
 	server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_s == -1) {
        DIE("unable to create socket");
    }
    if (fcntl(server_s, F_SETFD, 1) == -1) {
        DIE("can't set close-on-exec on server socket!");
    }
    
    if ((sock_opt = fcntl(server_s, F_GETFL, 0)) < -1) {
        DIE("can't set close-on-exec on server socket!");
    }
     if (fcntl(server_s, F_SETFL, sock_opt | O_NONBLOCK) == -1) {
        DIE("fcntl: unable to set server socket to nonblocking");
    }

	int on = 1 ;
    int keepAlive = 1;      //heart echo open
    int keepIdle = 15;      //if no data come in or out in 15 seconds,send tcp probe, not send ping
    int keepInterval = 3;   //3seconds inteval
    int keepCount = 5;      //retry count
        
    if( setsockopt(server_s, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive)) != 0) goto end_out;
    if( setsockopt(server_s, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle)) != 0) goto end_out;
    if( setsockopt(server_s, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)) != 0) goto end_out;
    if( setsockopt(server_s, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)) != 0) goto end_out;
    
    if( setsockopt(server_s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) !=0) goto end_out;

#endif
    
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sockaddr.sin_port = htons(client_port);

    
    /* internet family-specific code encapsulated in bind_server()  */
    if (bind(server_s, (struct sockaddr *) &server_sockaddr,
                sizeof (server_sockaddr)) == -1) {
        DIE("unable to bind");
    }
    
    /* listen: large number just in case your kernel is nicely tweaked */
    if (listen(server_s, max_connections) == -1) {
        DIE("unable to listen");
    }
    return server_s;

end_out:
    close(server_s);
    return 0;
}  


void create_h264_socket(int display_size, rfb_display *displays)
{
	
	int i, maxfd;
	
	fd_set allset;

	FD_ZERO(&allset);
	
	vids_queue = (QUEUE *)malloc(sizeof(QUEUE) * display_size);
	vids_buf = (unsigned char **)malloc(display_size * sizeof(unsigned char *));

	DEBUG("display_size %d", display_size);
	for(i = 0; i < display_size; i++)
	{
		create_udp(NULL, i + 1 + h264_port, &displays[i]);
		FD_SET(displays[i].fd, &allset);
		maxfd = maxfd > displays[i].fd ? maxfd : displays[i].fd;	
		
		vids_buf[i] = (unsigned char *)malloc(MAX_VIDSBUFSIZE * sizeof(unsigned char));
        memset(vids_buf[i], 0, MAX_VIDSBUFSIZE);
        /* 创建窗口的对应队列 */
        init_queue(&(vids_queue[i]), vids_buf[i], MAX_VIDSBUFSIZE);
	}	
	udp_loop(display_size, maxfd, allset, displays );
}

void create_udp(char *ip, int port, rfb_display *display)
{
    int ret = -1;
    int sockfd;
    struct sockaddr_in send_addr,recv_addr;

    DEBUG("ip  %s, port %d", ip, port);

#ifdef _WIN32
    WSADATA wsData = {0};
    if(0 != WSAStartup(0x202, &wsData))
    {
        DEBUG("WSAStartup  fail");
        WSACleanup();
        return -1;
    }
    int socklen =  sizeof (struct sockaddr_in);
#else
    socklen_t socklen = sizeof (struct sockaddr_in);
#endif

    int opt = 0;
    /* 创建 socket 用于UDP通讯 */
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        DIE("socket creating err in udptalk");
    }

    if(NULL != ip)
    {
        memset (&send_addr, 0, socklen);
        send_addr.sin_family = AF_INET;
        send_addr.sin_port = htons (port);
        send_addr.sin_addr.s_addr = inet_addr(ip);
    }
    opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR , (char *)&opt, sizeof(opt)) < 0)
    {
        DIE("setsockopt SO_REUSEADDR");
    }

    opt = 32*1024;//设置为32K
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&opt, sizeof (opt)) == -1)
    {
        DEBUG("IP_MULTICAST_LOOP set fail!");
	}

	opt = 32*1024;//设置为32K
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&opt, sizeof (opt)) == -1)
    {  
        DEBUG("IP_MULTICAST_LOOP set fail!");
    }  

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    recv_addr.sin_port = htons(port);

    /* 绑定自己的端口和IP信息到socket上 */
    if (bind(sockfd, (struct sockaddr *) &recv_addr,sizeof (struct sockaddr_in)) == -1)
    {  
        DIE("bind port %d err", port);
    }  

    display->fd = sockfd;
    display->port =  port;
    display->recv_addr = recv_addr;
    display->send_addr = send_addr;

    display->frame_size = 1024 * 1024 - 1;
    display->frame_pos = 0;
}


void tcp_loop(int listenfd)
{
	int maxfd, connfd, sockfd;
	int nready, ret;
	fd_set rset, allset;		

	struct sockaddr_in cliaddr, servaddr;
    socklen_t clilen = sizeof(cliaddr);

	maxfd = listenfd;

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

	rfb_request *conn = NULL;
    rfb_request *current = NULL;
    rfb_request *temp = NULL;

	total_connections = 0;

	for(;;)
	{
		rset = allset; // structure assignment 
		ret = select(maxfd + 1, &rset, NULL, NULL, NULL);
		if(ret <= 0)
		{
			 if(errno == EINTR)
                continue;
            else if(errno != EBADF)
                DIE("select");
		}
		nready = ret;

		if(FD_ISSET(listenfd, &rset))
        {   
            connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
             
             if(connfd < 0)
                continue;

			conn = new_request();
            conn->fd = connfd;
			conn->data_size = HEAD_LEN;
                 
#ifdef _WIN32
            //memcpy(&cliaddr.sin_addr, conn->ip, sizeof(conn->ip));
#else
            ret = fcntl(connfd,F_GETFL,0);
            if(ret < 0)
            {   
                close(connfd);
                continue;
            }   
            if(fcntl(connfd, F_SETFL, ret | O_NONBLOCK) <0)
            {   
                close(connfd);
                continue;
            }

			/* recode client ip */
            if(inet_ntop(AF_INET,&cliaddr.sin_addr, conn->ip, sizeof(conn->ip)) == NULL)
            {
                close(connfd);
                free(conn);
                continue;
            }
#endif
			DEBUG("connfd fd %d", conn->fd);
			
            enqueue(&request_ready, conn);
            FD_SET(connfd, &allset);
			total_connections++;
            if(connfd > maxfd)
                maxfd = connfd;
             if(--nready <= 0)
                continue;
        }

		current = request_ready;
		while(current)
        {
            if((sockfd = current->fd) < 0)
                continue;

            if(FD_ISSET(sockfd, &rset))
            {
				if(current->status != DEAD || current->status != DONE)
				{
					if(current->has_read_head == 0)
					{
						if((ret = recv(current->fd, current->head_buf+current->data_pos,HEAD_LEN - current ->data_pos, 0)) <= 0)
						{
							if(ret < 0)
							{
								if(errno == EINTR || errno == EAGAIN)
									continue;
							}
							DEBUG("close fd %d", sockfd);
							close(current->fd);	
							FD_CLR(current->fd, &allset);
							current = remove_request(current);
                    		continue;
						}
						current->data_pos += ret;
						if(current->data_pos != HEAD_LEN)
							continue;
						if(read_msg_syn(current->head_buf) != DATA_SYN)
                        {
                            close(current->fd); 
                            FD_CLR(current->fd, &allset);
                            current  = remove_request(current);
                            continue;
                        }

						current->has_read_head = 1;
                        current->data_size = read_msg_size(current->head_buf);
                        current->data_pos = 0;
			
						if(current->data_size < 0 || current->data_size > CLIENT_BUF)
                        {
                            close(current->fd); 
                            FD_CLR(current->fd, &allset);
                            current = remove_request(current);
                            continue;
                        }
                        else
                        {
                            current->data_buf = (unsigned char*)malloc(current->data_size + 1);
                            if(!current->data_buf)
                            {
                            	close(current->fd); 
                            	FD_CLR(current->fd, &allset);
                                current = remove_request(current);
                                continue;
                            }
                            memset(current->data_buf, 0, current->data_size + 1);
                        }
					}	
					if(current->has_read_head == 1)
					{
						if((ret = recv(current->fd, current->data_buf + current->data_pos, current->data_size - current ->data_pos,0)) <= 0)
                        {   
							if(ret < 0)
							{
								if(errno == EINTR || errno == EAGAIN)
									continue;
							}
                            DEBUG("close fd %d\n", sockfd);
                            close(current->fd); 
                            FD_CLR(current->fd, &allset);
                            current = remove_request(current);
                            continue;
                        }
                        current->data_pos += ret;
						if(current->data_pos == current->data_size)
                        {
                            if(process_msg(current))
                            {
	                         	DEBUG("close fd %d\n", sockfd);
                            	close(current->fd); 
                            	FD_CLR(current->fd, &allset);
                            	current = remove_request(current);
                            	continue;
                            }
                            memset(current->head_buf, 0, HEAD_LEN);
                            current->data_size = 0;
                            current->data_pos = 0;
                            free(current->data_buf);
                            current->data_buf = NULL;
                            current->has_read_head = 0;
                        }

                        if(current->data_pos > current->data_size)
                        {
                            /** wow , something error **/
                            DEBUG("current->pos > current->size");
                            close(current->fd); 
                            FD_CLR(current->fd, &allset);
                            current =  remove_request(current);
                            continue;
                        }
					}
				}
				else
				{
				    DEBUG("close fd %d\n", sockfd);
					close(current->fd);	
					FD_CLR(current->fd, &allset);
					current = remove_request(current);
                    continue;
				}
                if(--nready <= 0)
                    break;
            }
            current = current->next;
        }
	}
}

void udp_loop(int display_size, int maxfd, fd_set  allset, rfb_display *clients)
{
	int i, ret, nready;
	struct timeval tv;
	fd_set fds;
	
	tv.tv_sec = REQUEST_TIMEOUT;
    tv.tv_usec = 0l;

    int sockfd = -1;
    socklen_t socklen = sizeof (struct sockaddr_in);

#if 0
	unsigned int total_size = 0;
    unsigned int tmp_size = 0;
    unsigned char buf[MAX_VIDSBUFSIZE] = {0};
    unsigned int offset = 0;
    //unsigned char *tmp;

	unsigned char tmp[DATA_SIZE + sizeof(rfb_head)] = {0};
    unsigned short count = 0;
    unsigned short current_count = 0;
#endif

	    unsigned char buf[MAX_VIDSBUFSIZE] = {0};
    unsigned int offset = 0;
    //unsigned char *tmp;
    //unsigned short count = 0;
    //unsigned short current_count = 0;
    unsigned short num = 0;
    unsigned short total_num = 0;
    unsigned char tmp[DATA_SIZE + sizeof(rfb_packet)] = {0};
    unsigned char *data = &tmp[0] + sizeof(rfb_packet);
    unsigned short data_len = 0;
	rfb_packet *packet = (rfb_packet *)&tmp[0];

    for(;;)
    {
        fds = allset;
        ret = select(maxfd + 1, &fds, NULL, NULL, &tv);
        if(ret <= 0)
            continue;

        nready = ret;
        for(i = 0; i < display_size; i++)
        {
            if((sockfd = clients[i].fd) < 0)
                continue;
            if(FD_ISSET(sockfd, &fds))
            {
            	//ret = recvfrom(sockfd, (char *)clients[i].frame_buf + clients[i].frame_pos, clients[i].frame_size - clients[i].frame_pos, 0,
                    //(struct sockaddr*)&(clients[i].recv_addr), &socklen);
				ret = recvfrom(sockfd, (char *)tmp, DATA_SIZE + sizeof(rfb_packet), 0, 
						(struct sockaddr*)&(clients[i].recv_addr), &socklen);
	
				if(ret < 0)
					continue;

            	//tmp = &(clients[i].frame_buf[clients[i].frame_pos]);
            	//clients[i].frame_pos += ret;
            		     
				if(packet->num == 0)
				{
					total_num = packet->total_num;
                	offset = 0;
                	num = 0;	
				}
				if(packet->num == num)
				{
					data_len = packet->length;
                	memcpy(buf + offset, data, data_len);
                	offset += data_len;
                	num++;
				}
				if(num == total_num)
				{
					en_queue(&vids_queue[i], buf, offset, 0x0);	
					offset = 0;
				}
				
			}
#if 0
             if(tmp[0] == 0xff && tmp[1] == 0xff)
             {
                count = *((unsigned short *)&tmp[2]);
                if(count != clients[i].current_count)
                {
                    if(clients[i].current_count == 0)
                    {
                        clients[i].current_count = count;
                        continue;
                    }
                    total_size = *((unsigned int *)&tmp[4]);
                   	en_queue(&vids_queue[i], clients[i].frame_buf + 8,  clients[i].frame_pos - ret - 8, 0x0);
                    memcpy(clients[i].frame_buf, tmp, ret);
                    clients[i].frame_pos = ret;
                    clients[i].current_count = count;
                }
                else
                {
                    if(clients[i].frame_pos == clients[i].frame_size + 8)
                    {
                        en_queue(&vids_queue[i], clients[i].frame_buf + 8, offset - 8, 0x0);
                        clients[i].frame_pos = 0;
                        clients[i].frame_size = 0;
                        clients[i].current_count = 0;
                    }
                }
              }
              if(--nready <= 0)
                break;
            }
#endif
        }
    }
}



#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <SDL2/SDL.h>

#include "base.h"
#include "queue.h"

const char program_name[] = "ffplay";

int screen_width  = 0;
int screen_height = 0;

static int default_width  = 1920;
static int default_height = 1080;
static int audio_disable = 1;
//static int video_disable = 0;
static int display_disable = 0;
static int borderless = 0;
//static int exit_on_keydown;
//static int exit_on_mousedown;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_RendererInfo renderer_info = {0};
//static SDL_AudioDeviceID audio_dev;
static SDL_Texture *texture = NULL;
static SDL_Texture *full_texture = NULL;

int width,height, vids_width, vids_height;
rfb_display *displays = NULL;
rfb_display *control_display = NULL;
pthread_t *pthread_decodes = NULL;
pthread_mutex_t renderer_mutex;

int control_mode = 0;


static void do_exit()
{
    if(renderer)
        SDL_DestroyRenderer(renderer);
    if(window)
        SDL_DestroyWindow(window);
    SDL_Quit();
	DIE("programe end");
}

#if 0
void event_loop()
{
    int done = 0;
    SDL_Event event;
    DEBUG("init ok loop start");
    while(!done)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
				do_exit();
            break;
        }
    }
}
#endif


void clear_texture()
{
	DEBUG("clear_texture");
    pthread_mutex_lock(&renderer_mutex);
	
	SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    pthread_mutex_unlock(&renderer_mutex);
}

int test = 0;
int loss_count = 10;
int first_time = 1;


void update_texture(AVFrame *frame_yuv, SDL_Rect rect)
{
	if(!texture || !renderer)
		return;


	if(test)
	{
		
		if(first_time)
		{
		if(loss_count >= 0)
		{
			loss_count--;
			return ;
		}
			
		else
			loss_count = 10;
			first_time = 0;
		}


		rect.w = screen_width;
		rect.h = screen_height;
		pthread_mutex_lock(&renderer_mutex);
    	SDL_UpdateYUVTexture(full_texture, NULL,
        frame_yuv->data[0], frame_yuv->linesize[0],
        frame_yuv->data[1], frame_yuv->linesize[1],
        frame_yuv->data[2], frame_yuv->linesize[2]);

    	SDL_RenderCopy(renderer, full_texture, NULL, &rect);

	}
	else
	{
    	pthread_mutex_lock(&renderer_mutex);
    	SDL_UpdateYUVTexture(texture, NULL,
        	frame_yuv->data[0], frame_yuv->linesize[0],
        	frame_yuv->data[1], frame_yuv->linesize[1],
        	frame_yuv->data[2], frame_yuv->linesize[2]);

    	SDL_RenderCopy(renderer, texture, NULL, &rect);
	}
    SDL_RenderPresent(renderer);
    pthread_mutex_unlock(&renderer_mutex);
}


void create_control()
{
	int ret;
	pthread_t pthread_decode;
 	control_display = (rfb_display *)malloc(sizeof(rfb_display));
	

	ret = pthread_create(&pthread_decodes, NULL, thread_decode, control_display);
    if(0 != ret)
    {   
        DIE("ThreadDisp err %d,  %s",ret , strerror(ret));
    }   
	control_flag = 1;
}


void destroy_control()
{
	if(control_display)
		free(control_display);
		
	control_flag = 0;
}


void partition_display()
{
	int i , j, ret, id;

    vids_width = screen_width / window_size;
    vids_height = screen_height / window_size;

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, vids_width, vids_height);
	if(!texture)
	{
		DIE("create texture err");
	}

	full_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);
	if(!full_texture)
	{
		DIE("create texture err");
	}
	pthread_mutex_init(&renderer_mutex, NULL);

    displays = (rfb_display *)malloc(sizeof(rfb_display) * window_size * window_size);
    memset(displays, 0, sizeof(rfb_display) * window_size * window_size);
    pthread_decodes = (pthread_t *)malloc(sizeof(pthread_t) * window_size * window_size);

	clear_texture();

    for(i = 0; i < window_size; i++)
    {   
        for(j = 0; j < window_size; j++)
        {   
            id = i + j * window_size;
            displays[id].id = id;
            displays[id].rect.x = i * vids_width;
            displays[id].rect.y = j * vids_height;
            displays[id].rect.w = vids_width;
            displays[id].rect.h = vids_height;

            /* 创建对应窗口的显示线程 */
            ret = pthread_create(&(pthread_decodes[id]), NULL, thread_decode, &(displays[id]));
            if(0 != ret)
            {   
                DIE("ThreadDisp err %d,  %s",ret , strerror(ret));
            }   
        }   
    }   

}


static void refresh_loop_wait_event(SDL_Event *event)
{
	//double remaining_time = 0.0;
    SDL_PumpEvents();
	while (!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT))
	{
		SDL_PumpEvents();
	}
}


void event_loop()
{
	SDL_Event event;
	for(;;)
	{
		refresh_loop_wait_event(&event);
		switch(event.type)
		{
			case SDL_KEYDOWN:   //键盘事件
				if(event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q)
				{
					test = !test;
					start_control(displays[0].req);
					break;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:  //鼠标按下
				switch(event.button.button)
				{
					case SDL_BUTTON_LEFT:
						if(control_mode)
						{
							//point.mask | = (1<<2);
						}
						else
						{
							#if 0
							x = event.button.x;
							y = event.button.x;
							id = check_area(x,y);  //判断在哪个区域
							if(id == id)			//是否连点2次
							{
								contorl_mode = 1;
								//pthread_wait();
								//tranfrom_mode();	
							}	
							#endif
						}
						break;
					case SDL_BUTTON_RIGHT:
						break;
				}
				break;

			case SDL_MOUSEBUTTONUP:  //鼠标抬起
				break;
				
			case SDL_QUIT:			//关闭窗口
                run_flag = 0;
				do_exit();
				goto run_end;
			default:
				break;
		}
	}	

run_end:
	if(pthread_decodes)
		free(pthread_decodes);

    SDL_Quit();
}



void control_loop()
{
#if 0
    SDL_Event event;
    rfb_pointevent point;
    rfb_keyevent key;
    for(;;)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_MOUSEBUTTONDOWN)
            {
                if(SDL_BUTTON_LEFT == event.button.button)
                {
                    point.mask |= (1<<1);
                }

                if(SDL_BUTTON_RIGHT == event.button.button)
                {
                    point.mask |= (1<<3);
                }
            }

            if(event.type == SDL_MOUSEBUTTONUP)
            {
                if(SDL_BUTTON_LEFT == event.button.button)
                {
                    point.mask |= (1<<2);
                }

                if(SDL_BUTTON_RIGHT == event.button.button)
                {
                    point.mask |= (1<<4);
                }
            }
            if(point.mask != 0 || event.type == SDL_MOUSEMOTION)
            {
                point.x = event.motion.x;
                point.y = event.motion.y;
                send_control((char *)&point, sizeof(rfb_pointevent), 0x01);
                point.mask = 0;
            }

            if (SDL_KEYDOWN == event.type) // SDL_KEYUP
            {
                key.key = *(SDL_GetKeyName(event.key.keysym.sym));
                key.down = 1;
                send_control((char *)&key, sizeof(rfb_keyevent), 0x02);
            }

            if(SDL_KEYUP == event.type)
            {
                key.key = *(SDL_GetKeyName(event.key.keysym.sym));
                key.down = 0;
                send_control((char *)&key, sizeof(rfb_keyevent), 0x02);
            }
        }
    }
#endif

}


void create_display()
{
    int flags;
    flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
    if(audio_disable)
        flags &= ~SDL_INIT_AUDIO;
    else
    {
        /* Try to work around an occasional ALSA buffer underflow issue when the
         * period size is NPOT due to ALSA resampling by forcing the buffer size. */
        if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE"))
            SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE","1", 1);
    }

    if(display_disable)
        flags &= ~SDL_INIT_VIDEO;

    if(SDL_Init(flags))
    {
        DIE("Could not initialize SDL - %s", SDL_GetError());
    }
    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

    if(!display_disable)
    {
        DEBUG("create window ");
        int flags = SDL_WINDOW_HIDDEN;  //SDL_WINDOW_SHOWN SDL_WINDOW_HIDDEN
        if(borderless)              //无边框
            flags |= SDL_WINDOW_BORDERLESS;
        else
            flags |= SDL_WINDOW_RESIZABLE;

        window = SDL_CreateWindow(program_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, default_width, default_height, 0);
		if(!window_flag)
		{
			SDL_Rect fullRect;
			SDL_GetDisplayBounds(0, &fullRect);
			DEBUG("full_width %d full_height %d", fullRect.w, fullRect.h);
	    	screen_width = fullRect.w;
	    	screen_height = fullRect.h;
			SDL_SetWindowSize(window, screen_width, screen_height);
			SDL_SetWindowPosition(window, 0, 0);
			//SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
		}
		else
		{
	    	screen_width = default_width;
	    	screen_height = default_height;
			//SDL_SetWindowPosition(window, fullRect.x, fullRect.y);
		}
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        if(window)
        {
     //       renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if(!renderer)
            {
                DEBUG("Failed to initialize a hardware accelerated renderer: %s", SDL_GetError());
                renderer = SDL_CreateRenderer(window, -1, 0);
            }
            if(renderer)
            {
                if (!SDL_GetRendererInfo(renderer, &renderer_info))
                    DEBUG("Initialized %s renderer.", renderer_info.name);
            }
        }
        if(!window || !renderer || !renderer_info.num_texture_formats)
        {
            DIE("Failed to create window or renderer: %s", SDL_GetError());
        }
    }
	partition_display();
    atexit(do_exit);
    event_loop();
}

void *thread_display(void *param)
{
	int ret;
    pthread_attr_t st_attr;
    struct sched_param sched;

    ret = pthread_attr_init(&st_attr);
    if(ret)
    {   
       	DEBUG("ThreadMain attr init error ");
    }   
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {   
        DEBUG("ThreadMain set SCHED_FIFO error");
    }   
    sched.sched_priority = SCHED_PRIORITY_UDP;
    ret = pthread_attr_setschedparam(&st_attr, &sched);

	create_display();
	return (void *)0;
}

#include "base.h"
#include "msg.h"

rfb_display client_display = {0};



int login_server(int fd, rfb_format *fmt)
{
	char buf[1024] = {0};
	char *tmp = &buf[8];
	int server_major = 0, server_minor = 0;
		
	rfb_head *head = (rfb_head *)&buf[0];
	head->syn = 0xff;
	head->encrypt_flag = 0;	//加密
	head->cmd = 1;
	head->data_size = sz_verformat;
	
	/* version */
    sprintf(tmp, VERSIONFORMAT, server_major, server_minor);    

	send_msg(fd, buf, HEAD_LEN + sz_verformat);
	recv_msg(fd, buf, HEAD_LEN + sz_verformat);
		
	sscanf(buf + 8, VERSIONFORMAT, &server_major, &server_minor); 
    DEBUG(VERSIONFORMAT, server_major, server_minor);

	
	/* format */
	head->cmd = 2;
	head->data_size = sizeof(rfb_format);

    rfb_format *_fmt = (rfb_format *)&tmp[0];
    _fmt->width = 1280;
    _fmt->height = 720;
    _fmt->code = 0;
    _fmt->data_port = 0;
    _fmt->play_flag = 0;
    _fmt->vnc_flag = 0;	
    _fmt->quality = default_quality;	
    _fmt->fps = default_fps;	
	
	send_msg(fd, buf, HEAD_LEN + sizeof(rfb_format));
	recv_msg(fd, buf, HEAD_LEN + sizeof(rfb_format));

	memcpy(fmt, _fmt, sizeof(rfb_format));

	DEBUG("fmt->width %d fmt->height %d fmt->code %d,"
          " fmt->data_port %d fmt->play_flag %d  fmt->vnc_flag %d fmt->quality %d fmt->fps%d",
             fmt->width, fmt->height,fmt->code, fmt->data_port,fmt->play_flag, fmt->vnc_flag, fmt->quality, fmt->fps);
    if(fmt->width < 0 || fmt->width > 1920 || fmt->height < 0 || fmt->height > 1080 || fmt->data_port < 0 || fmt->data_port > 24000)
    {
        DEBUG("login server fail ");
		return -1;
    }
	return 0;
}

void start_encode(rfb_format *fmt)
{
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
	void *tret = NULL;
	pthread_join(pthread_udp, &tret);  //等待线程同步		
	pthread_join(pthread_encode, &tret);  //等待线程同步		
}

void init_client()
{
	int ret = -1;
	pthread_t pthread_tcp;
	rfb_format fmt = {0};

	run_flag = 1;

    int server_s = create_tcp();
    if(!server_s)
    {   
        DIE("create tcp err");
    }   
    connect_server(server_s, server_ip, server_port);
    ret = login_server(server_s, &fmt);
    if(0 != ret)
    {   
        DIE("login_server err");
    }   
    ret = init_dev();
    if(0 != ret)
    {   
        DIE("init device err %d,  %s",ret,strerror(ret));
    }   
    ret = pthread_create(&pthread_tcp, NULL, thread_client_tcp, &server_s);
    if(0 != ret)
    {   
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    } 
	DEBUG("fmt.play_flag %d", fmt.play_flag);
	if(fmt.play_flag)
	{
		start_encode(&fmt);
	}	
	void *tret = NULL;
	pthread_join(pthread_tcp, (void**)tret);  //等待线程同步		
}


#include "base.h"
#include "msg.h"

int display_size = 0;

int read_login(rfb_request *req)
{
    if(read_msg_order(req->head_buf) == 0x01)
    {
        int server_major, server_minor;

        if (sscanf((char *)req->data_buf, VERSIONFORMAT,
                    &server_major, &server_minor) != 2)
                ;
        sprintf((char *)req->data_buf, VERSIONFORMAT, 3, server_minor + 1);
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

		while(!displays)
		{
			sleep(1);
		}

       	for(i = 0; i < display_size; i++)   //ready play
       	{
           	if(!displays[i].play_flag)      //存在空闲的分屏, 发送play命令
           	{
               	displays[i].req = req;
               	displays[i].play_flag = 1;
                fmt->width = vids_width;
                fmt->height = vids_height;
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
	return ret;
}

void init_server()
{
    int ret, server_s;
    pthread_t pthread_tcp, pthread_udp , pthread_display;

    server_s = create_tcp();
	
	run_flag = 1;

	display_size = window_size * window_size;
    if(server_s == -1) 
    {   
        DIE("create socket err");
    }   

    if(bind_server(server_s, client_port) == -1) 
    {   
        DIE("bind port %d err", client_port);
    }   
	DEBUG("server_s %d", server_s);

	ret = pthread_create(&pthread_display, NULL, thread_display, NULL);
    if(0 != ret)
    {
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    }

    ret = pthread_create(&pthread_tcp, NULL, thread_server_tcp, &server_s);
    if(0 != ret)
    {
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    }

    ret = pthread_create(&pthread_udp, NULL, thread_server_udp, NULL);
    if(0 != ret)
    {
        DIE("ThreadTcp err %d,  %s",ret,strerror(ret));
    }

    void *tret = NULL;
    pthread_join(pthread_display, &tret);  //等待线程同步
	DEBUG("pthread_join %d", (int)tret);
    pthread_join(pthread_tcp, &tret);  //等待线程同步
	DEBUG("pthread_join %d", (int)tret);
    pthread_join(pthread_udp, &tret);  //等待线程同步
	DEBUG("pthread_join %d", (int)tret);
}



//#include <X11/Xlib.h>
//#include <X11/Xutil.h>      /* BitmapOpenFailed, etc.    */
//#include <X11/cursorfont.h> /* pre-defined crusor shapes */

#include "base.h"

static int fd_kbd = 0;
static int fd_mouse  = 0;
//static Display *dpy = NULL;
//static Window root;


int init_dev()
{
#if 0
	int id;
	if((dpy = XOpenDisplay(0)) == NULL)
	{
		DEBUG("");
		goto err;
	}
	id = DefaultScreen(dpy);  //DISPLAY:= id
	if(!(root = XRootWindow(dpy, id)))
	{
		DEBUG("");
		goto err;
	}
	screen_width = DisplayWidth(dpy, id);
	screen_height = DisplayHeight(dpy, id);

	fd_kbd = open("/dev/input/event1", O_RDWR);
	if(fd_kbd <= 0)
	{
		DEBUG("");
		goto err;
	}

	fd_mouse = open("/dev/input/event2", O_RDWR);
	if(fd_mouse <= 0)
	{
		DEBUG("");
		goto err;
	}	
	return 0;
err:
	if(fd_kbd)
		close(fd_kbd);
	if(fd_mouse)
		close(fd_mouse);
	if(dpy)
		XCloseDisplay(dpy);
	return -1;
#endif
}

#if 0
void get_position(int *x, int *y)
{
	int tmp;
	unsigned int tmp2;
	Window fromroot, tmpwin;
	XQueryPointer(dpy, root, &fromroot, &tmpwin, x, y, &tmp, &tmp, &tmp2);	
}


void simulate_mouse(int x, int y)
{
	int ret;
	ret = XWarpPointer(dpy, None, root, 0, 0, 0, 0, x, y);
	XFlush(dpy);
	DEBUG("XWarpPointer ret %d, %s", ret, strerror(ret));
	//mouseClick(Button3);
}
#if 0
typedef struct {
    int type;       /* of event */
    unsigned long serial;   /* # of last request processed by server */
    Bool send_event;    /* true if this came from a SendEvent request */
    Display *display;   /* Display the event was read from */
    Window window;          /* "event" window reported relative to */
    Window root;            /* root window that the event occured on */
    Window subwindow;   /* child window */
    Time time;      /* milliseconds */
    int x, y;       /* pointer x, y coordinates in event window */
    int x_root, y_root; /* coordinates relative to root */
    unsigned int state; /* key or button mask */
    char is_hint;       /* detail */
    Bool same_screen;   /* same screen flag */
} XMotionEvent;
typedef XMotionEvent XPointerMovedEvent;
#endif
void mouseClick(int button)
{
	XEvent event;
	memset(&event, 0x00, sizeof(event));

	event.type = ButtonPress;
	event.xbutton.button = button;
	event.xbutton.same_screen = True;
	
	XQueryPointer(dpy, RootWindow(dpy, DefaultScreen(dpy)), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);

	event.xbutton.subwindow = event.xbutton.window;
	while(event.xbutton.subwindow)
	{
    	event.xbutton.window = event.xbutton.subwindow;
    	XQueryPointer(dpy, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
	}

   	if(XSendEvent(dpy, PointerWindow, True, 0xfff, &event) == 0) printf("Errore nell'invio dell'evento !!!\n");
    XFlush(dpy);

    sleep(1);

    event.type = ButtonRelease;
    event.xbutton.state = 0x100;

    if(XSendEvent(dpy, PointerWindow, True, 0xfff, &event) == 0) printf("Errore nell'invio dell'evento !!!\n");

    XFlush(dpy);
}

#if 0
void simulate_mouse(int x, int y)
{
    
    /* EV_KEY ¼üÅÌ 
     * EV_REL Ïà¶Ô×ø±ê Êó±ê
     * EV_ABS ¾ø¶Ô×ø±ê ÊÖÐ´°å
     */
    //unsigned long x = (rel_x * (1920 / 960.0));
    //unsigned long y = (rel_y *  (1080 / 720.0));

    struct input_event event;
	int current_x, current_y;

	unsigned long rel_x = 0;
	unsigned long rel_y = 0;

	get_position(&current_x, &current_y);
	DEBUG("current_x %d current_y %d", current_x, current_y);

	rel_x = x - current_x;			
	rel_y = y - current_y;			


    memset(&event, 0, sizeof(event));
    gettimeofday(&event.time, NULL);
    event.type = EV_REL;
    event.code = REL_X;
    event.value = rel_x;
    write(fd_mouse, &event, sizeof(event));
    event.type = EV_REL;
    event.code = REL_Y;
    event.value = rel_y;
    write(fd_mouse, &event, sizeof(event));
    event.type = EV_SYN;
    event.code = SYN_REPORT;
    event.value = 0;
    write(fd_mouse, &event, sizeof(event));

	get_position(&current_x, &current_y);
	DEBUG("current_x %d current_y %d", current_x, current_y);
}
#endif
#endif

#include "VNCHooks.h"
extern "C"
{
#include "base.h"
#include "msg.h"
}

#ifdef _WIN32
HHOOK hKeyboardHook = NULL;
HINSTANCE hInstance = NULL;


LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	//rfb_keyevent key = {0};
	PKBDLLHOOKSTRUCT pVirKey = (PKBDLLHOOKSTRUCT)lParam;

	//MessageBox(NULL, "WIN", "DOWN", MB_ICONEXCLAMATION | MB_OK);
	if(nCode >= 0)
	{
		switch(wParam)
		{
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				switch(pVirKey->vkCode)
				{
					case VK_LWIN:
					case VK_RWIN:
					{
						//key.key = 1073742055;
						MessageBox(NULL, "WIN", "DOWN", MB_ICONEXCLAMATION | MB_OK);
						//key.down = 1;
						//send_control((char *)&key, sizeof(rfb_keyevent), 0x04);

						//return CallNextHookEx(hInstance, nCode, wParam, lParam);
						//return CallNextHookEx(hInstance, nCode, wParam, lParam);
						//return 0;
						return TRUE;
					}
					case VK_MENU:
						//key.key = VK_MENU;
						//key.down = 1;
						return 1;
					default:
						break;
				}
				break;

			case WM_KEYUP:
			case WM_SYSKEYUP:
				switch(pVirKey->vkCode)
				{
					case VK_LWIN:
					case VK_RWIN:
					{
						//key.key = 1073742055;
						MessageBox(NULL, "WIN", "UP", MB_ICONEXCLAMATION | MB_OK);
						//key.down = 0;
						//send_control((char *)&key, sizeof(rfb_keyevent), 0x04);
						return TRUE;
					}
					case VK_MENU:
						//key.key = 1073742050;
						//key.down = 0;
						//send_control((char *)&key, sizeof(rfb_keyevent), 0x04);
						return 1;	
				}
				break;
		}

		//return CallNextHookEx(hInstance, nCode, wParam, lParam);
	}
	//return CallNextHookEx(hInstance, nCode, wParam, lParam);
}

int SetKeyboardHook(int active)
{
	if(active)
	{
		hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, hInstance ,0);
	}
	else
	{
		UnhookWindowsHookEx(hKeyboardHook);
		hKeyboardHook = NULL;	
	}
	return 0;
}

#endif
