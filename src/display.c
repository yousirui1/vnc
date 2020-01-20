#include <libavutil/frame.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#ifndef _WIN32
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>      /* BitmapOpenFailed, etc.    */
    #include <X11/cursorfont.h> /* pre-defined crusor shapes */
    #include <linux/input.h>
#endif

#include "base.h"
#include "frame.h"

int screen_width  = 0;
int screen_height = 0;
int vids_width = 0;
int vids_height = 0;

static int default_width  = 1600;
static int default_height = 900;
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
static SDL_Rect full_rect;
static SDL_Rect rect;
static SDL_Texture *ttf_texture = NULL;
static TTF_Font *font = NULL;

static int loss_flag = 0;
static int loss_count = 10;
static int area_id = -1;

rfb_display *displays = NULL;
rfb_display *control_display = NULL;
pthread_t *pthread_decodes = NULL;
pthread_mutex_t renderer_mutex;

/* 获取一个可写的Frame写入 */
static Frame *frame_queue_peek_writable(FrameQueue *f)
{
    SDL_LockMutex(f->mutex);
    while(f->size >= f->max_size)               //没有可写入的
    {
        SDL_CondWait(f->cond, f->mutex);        //信号阻塞等待
    }

    SDL_UnlockMutex(f->mutex);

    return &f->queue[f->windex];
}


static void frame_queue_push(FrameQueue *f)
{
    if(++f->windex == f->max_size)
        f->windex = 0;
    SDL_LockMutex(f->mutex);
    f->size++;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

static void frame_queue_unref_item(Frame *vp)
{
    /* 重置AVFrame */
    av_frame_unref(vp->frame);
    avsubtitle_free(&vp->sub);
}

int frame_queue_init(FrameQueue *f, int max_size)
{
    int i = 0;;
    memset(f, 0, sizeof(FrameQueue));
    if (!(f->mutex = SDL_CreateMutex()))
    {
        DEBUG("SDL_CreateMutex(): %s", SDL_GetError());
        return -1;
    }

    if(!(f->cond = SDL_CreateCond()))
    {
        DEBUG("SDL_CreateMutex(): %s", SDL_GetError());
        return -1;
    }
    f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
    for(i = 0; i < f->max_size; i++)
    {
        if (!(f->queue[i].frame = av_frame_alloc()))
        {
            return -1;
        }
    }
    return 0;
}

void frame_queue_destroy(FrameQueue *f)
{
    int i = 0;
    for(i = 0; i < f->max_size; i++)
    {
        Frame *vp = &f->queue[i];
        frame_queue_unref_item(vp);
        av_frame_free(&vp->frame);
    }
    SDL_DestroyMutex(f->mutex);
    SDL_DestroyCond(f->cond);
}

static void frame_queue_signal(FrameQueue *f)
{


}

/* 出当前的。因为f->rindex + f->rindex_shown可能会超过max_size,所以用了取余 */
static Frame *frame_queue_peek(FrameQueue *f)
{
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

/* 用于在显示之前。判断队列中，是否还存在下一个，队列阻塞会导致*/
static Frame *frame_queue_peek_next(FrameQueue *f)
{
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

/* 不阻塞读取数据 */
static Frame *frame_queue_peek_last(FrameQueue *f)
{
    return &f->queue[f->rindex];
}

/* 获取一个可读的Frame读入 */
static Frame *frame_queue_peek_readable(FrameQueue *f)
{
    SDL_LockMutex(f->mutex);
    while(f->size - f->rindex_shown <= 0)       //没有可读入的
    {
        SDL_CondWait(f->cond, f->mutex);        //信号阻塞等待
    }

    SDL_UnlockMutex(f->mutex);

    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static void frame_queue_pop(FrameQueue *f)
{
    if(!f->rindex_shown)
    {
        f->rindex_shown = 1;
        return;
    }
    frame_queue_unref_item(&f->queue[f->rindex]);
    if(++f->rindex == f->max_size)
        f->rindex = 0;
    SDL_LockMutex(f->mutex);
    f->size--;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);

}

/* 返回还未显示的数量 */
static int frame_queue_nb_remaining(FrameQueue *f)
{
    return f->size - f->rindex_shown;
}



static void send_control(char *buf, int data_len, int cmd)
{
	if(!control_display || status != CONTROL)
		return;
	set_request_head(control_display->req, cmd, data_len);
    control_display->req->data_buf = buf;

    send_request(control_display->req);
    control_display->req->data_buf = NULL;
}

static void do_exit()
{


}

static int get_area(int x, int y)
{
	return ((x/vids_width) + (y/vids_height) * window_size);
}

void hide_window()
{
	if(!window)
        return
    SDL_HideWindow(window);
}

void show_window()
{
	if(!window)
		return;

	SDL_ShowWindow(window);		
}

void clear_texture()
{
    pthread_mutex_lock(&renderer_mutex);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    pthread_mutex_unlock(&renderer_mutex);
}


void update_frame(AVFrame *frame_yuv)
{
	Frame *vp = NULL;

   //vp = frame_queue_peek_writable(&frame_queue);
   vp->width = frame_yuv->width;
   vp->height = frame_yuv->height;
   vp->format = frame_yuv->format;

   //av_frame_clone
   //av_frame_copy(vp->frame, frame_yuv);
   //vp->frame = av_frame_clone(frame_yuv);
   av_frame_move_ref(vp->frame, frame_yuv);
   //frame_queue_push(&frame_queue);
   vp = NULL;
}


void update_texture(AVFrame *frame_yuv, SDL_Rect *rect)
{
    if(!texture || !renderer || !full_texture || !frame_yuv)
        return;
    if(loss_flag)						//丢弃切换后花屏的几帧
    {
        if(loss_count-- > 0)
            return;
        else
        {
            loss_count = 10;
            loss_flag = 0;
        }
    }

    pthread_mutex_lock(&renderer_mutex);
    if(!rect)								//全屏更新
    {
        SDL_UpdateYUVTexture(full_texture, NULL,
        frame_yuv->data[0], frame_yuv->linesize[0],
        frame_yuv->data[1], frame_yuv->linesize[1],
        frame_yuv->data[2], frame_yuv->linesize[2]);

        SDL_RenderCopy(renderer, full_texture, NULL, &full_rect);
    }
    else									//局部更新
    {
        SDL_UpdateYUVTexture(texture, NULL,
            frame_yuv->data[0], frame_yuv->linesize[0],
            frame_yuv->data[1], frame_yuv->linesize[1],
            frame_yuv->data[2], frame_yuv->linesize[2]);

        SDL_RenderCopy(renderer, texture, NULL, rect);
    }
    SDL_RenderPresent(renderer);
    pthread_mutex_unlock(&renderer_mutex);
}


static void refresh_loop_wait_event(SDL_Event *event)
{
    //double remaining_time = 0.0;
    SDL_PumpEvents();
    while (!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) && run_flag)
    {
        SDL_PumpEvents();
    }
}
void start_display(rfb_display *cli)
{
	status = PLAY;
	cli->req->status = PLAY;
	DEBUG("cli->req->status %d", cli->req->status);
	cli->req->data_buf = malloc(sizeof(rfb_format) + 1);	
	cli->fmt.play_flag = cli->play_flag = 1;
	memset(cli->req->data_buf, 0, sizeof(rfb_format));
	rfb_format *fmt = (rfb_format *)&(cli->req->data_buf[0]);	
	memcpy(fmt, &(cli->fmt), sizeof(rfb_format));
	DEBUG("cli->req->status %d", cli->req->status);
	process_server_msg(cli->req);
}

void stop_display()
{
	int i;
	for(i = 0; i < display_size; i++)
	{
		if(displays[i].play_flag == 1)
		{
			if(displays[i].req)
				process_server_msg(displays[i].req);
			displays[i].play_flag = 0;
		}
	}
	usleep(200000);   //等待200ms 客户端结束 
	status = DONE;	//在检测到udp 有数据就断开连接保证下一页数据不被混杂
	usleep(200000);   //等待200ms 客户端断开
	clear_texture();
}

void revert_display()
{
	int i;
	for(i = 0; i < display_size; i++)
	{
		if(displays[i].play_flag == 0)
		{
			if(displays[i].req)
			{
				start_display(&displays[i]);
				DEBUG(" process_server_msg %d", i);
			}
			displays[i].play_flag = 1;
		}
	}
}

void start_control(int id)
{
	if(!displays[id].req )
		return;	
	DEBUG("start_control");
	status = CONTROL;
	control_display = &displays[id];
	control_display->fmt.play_flag = control_display->play_flag = 2;    //控制状态
	displays[id].req->status = CONTROL;
	process_server_msg(displays[id].req);
    clear_texture();
}

void stop_control()
{
	if(!control_display || !control_display->req)
	{
		status = PLAY;
		return;	
	}

	control_display->fmt.play_flag = control_display->play_flag = 0;    //控制状态
	control_display->req->status = DONE;
	process_server_msg(control_display->req);
    clear_texture();
	usleep(200000);
}


void switch_mode(int id)
{
    if(displays[id].play_flag == 1)
    {
        DEBUG("switch control");
		stop_display();	
		start_control(id);
        loss_flag = 1;
    }
    else if(status == CONTROL)
    {
        DEBUG("switch display");
		stop_control();
		revert_display();		
    }
}

void close_display()
{
	int i;
	for(i = 0; i < display_size; i++)
	{
		if(!displays)
			return;	

		if(displays[i].play_flag == 1)
		{
			if(displays[i].req)
			{
				process_server_msg(displays[i].req);
				displays[i].req->status = DEAD;
			}
			displays[i].play_flag = 0;
			displays[i].req = NULL;
		}
	}
	DEBUG("close_display OK");
	clear_texture();
}

void control_msg(rfb_request *req)
{
	switch(read_msg_order(req->head_buf))
	{
		case MOUSE:
			//simulate_mouse((rfb_pointevent *)&(req->data_buf[0]));
			break;
		case KEYBOARD:
			//simulate_keyboard((rfb_keyevent *)&(req->data_buf[0]));
			break;
		case COPY_TEXT:
			break;
		case COPY_FILE:
			break;
	}
}

void sdl_loop()
{
	SDL_Event event;
	rfb_pointevent point = {0};
	rfb_keyevent key = {0};
	short old_x = 0;
	short old_y = 0;	
		
	while(run_flag)
	{
		refresh_loop_wait_event(&event);
		/* 鼠标滚轮 */
		if(event.type == SDL_MOUSEWHEEL)
		{
			point.wheel = event.wheel.y;
			point.x = old_x;
			point.y = old_y;
			send_control((char *)&point, sizeof(rfb_pointevent), MOUSE);
			point.wheel = 0;	
		}
		/* 鼠标按下 */
		if(event.type == SDL_MOUSEBUTTONDOWN)
		{
			if(status == PLAY)
			{
				(void) time(&current_time);	
				if((current_time - last_time) > 1)
				{
					area_id = -1;
					last_time = current_time;
				}
	
				if(area_id == get_area(event.motion.x, event.motion.y))
				{
					switch_mode(area_id);
				}
				area_id = get_area(event.motion.x, event.motion.y);
				DEBUG("area_id %d", area_id);
			}
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
            old_x = point.x;
        	old_y = point.y;
            send_control((char *)&point, sizeof(rfb_pointevent), MOUSE);
			point.mask = 0;
		}
		if(SDL_KEYDOWN == event.type)
		{
			key.key = event.key.keysym.sym; //*(SDL_GetKeyName(event.key.keysym.sym));
            key.mod = event.key.keysym.mod; //*(SDL_GetKeyName(event.key.keysym.sym));
            key.down = 1;
            send_control((char *)&key, sizeof(rfb_keyevent), KEYBOARD);
		}	
		if(SDL_KEYUP == event.type)
		{
            key.key = event.key.keysym.sym;
            key.mod = event.key.keysym.mod;
            key.down = 0;
            send_control((char *)&key, sizeof(rfb_keyevent), KEYBOARD);
		}
		if(SDL_QUIT == event.type)
		{
			run_flag = 0;
#ifdef _WIN32
			stop_server();		//通过callback 返回给dll
#endif
		}
	}	
	DEBUG("event_loop end");
}


/* 等待线程结束后销毁公共变量 */
void destroy_display()
{
	int i = 0;
	void *tret = NULL;

	for(i = 0; i < window_size * window_size ; i++ )
	{
		pthread_join(pthread_decodes[i], &tret);
	}

    if(renderer)
        SDL_DestroyRenderer(renderer);

    if(texture)
        SDL_DestroyTexture(texture);

    if(full_texture)
        SDL_DestroyTexture(full_texture);

    if(window)
        SDL_DestroyWindow(window);

    if(displays)
        free(displays);

    if(pthread_decodes)
        free(pthread_decodes);

    control_display = NULL;
    displays = NULL;
    pthread_decodes = NULL;
    window = NULL;
    renderer = NULL;
    full_texture = NULL;
    texture = NULL;

    SDL_Quit();

    DEBUG("destroy_display end");
}


void init_display()
{
	int i, j ,ret, id;
	
	pthread_mutex_init(&renderer_mutex, NULL);	
	
	vids_width = screen_width / window_size;
    vids_height = screen_height / window_size;

	vids_queue = (QUEUE *)malloc(sizeof(QUEUE) * display_size);
    vids_buf = (unsigned char **)malloc(display_size * sizeof(unsigned char *));

    displays = (rfb_display *)malloc(sizeof(rfb_display) * window_size * window_size);
    pthread_decodes = (pthread_t *)malloc(sizeof(pthread_t) * window_size * window_size);
	
	if(!vids_queue || !vids_buf || !displays || !pthread_decodes)
	{
		DIE("param malloc err");
	}
    memset(displays, 0, sizeof(rfb_display) * window_size * window_size);

	/* 局部画板 */
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, vids_width, vids_height);
    if(!texture)
    {
        DIE("create texture err");
    }

	/* 全局画板 */
	full_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);
    if(!full_texture)
    {
        DIE("create full texture err");
    }

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
	        displays[id].mtx = PTHREAD_MUTEX_INITIALIZER;
            displays[id].cond = PTHREAD_COND_INITIALIZER;



            vids_buf[id] = (unsigned char *)malloc(MAX_VIDSBUFSIZE * sizeof(unsigned char));
            memset(vids_buf[id], 0, MAX_VIDSBUFSIZE);
            /* 创建窗口的对应队列 */
            init_queue(&(vids_queue[id]), vids_buf[id], MAX_VIDSBUFSIZE);

            /* 创建对应窗口的显示线程 */
            ret = pthread_create(&(pthread_decodes[id]), NULL, thread_decode, &(displays[id]));
            if(0 != ret)
            {
                DIE("ThreadDisp err %d,  %s",ret , strerror(ret));
            }
        }
    }
	clear_texture();
	DEBUG("init display end");
}

int create_display()
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
        DEBUG("Could not initialize SDL - %s", SDL_GetError());
        goto run_out;
    }

    if(TTF_Init() == -1)
    {
        DEBUG("SDL_ttf Init error: %s", SDL_GetError());
        goto run_out;
    }

    font = TTF_OpenFont("../font/msyh.ttf", 16);
    if(!font)
    {
        DEBUG("SDL_ttf open ttf dir: %s error", "../font/msyh.ttf");
        goto run_out;
    }

    SDL_Color color = {255, 255, 255};
    /*
     * Solid  渲染的最快，但效果最差，文字不平滑，是单色文字，不带边框。
  　 * Shaded 比Solid渲染的慢，但显示效果好于Solid，带阴影。
　　 * Blend 渲染最慢，但显示效果最好。
     */
    SDL_Surface *surface = TTF_RenderUTF8_Solid(font, "你好!!", color);

    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

    if(!display_disable)
    {
        DEBUG("create window ");
        int flags = SDL_WINDOW_HIDDEN;  //SDL_WINDOW_SHOWN SDL_WINDOW_HIDDEN
        if(borderless)                  //无边框
            flags |= SDL_WINDOW_BORDERLESS;
        else
            flags |= SDL_WINDOW_RESIZABLE;  //可以缩放暂时不支持

        window = SDL_CreateWindow(program_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
									default_width, default_height, flags);

        SDL_GetDisplayBounds(0, &full_rect);
        DEBUG("full_width %d full_height %d", full_rect.w, full_rect.h);

        if(!window_flag)
        {
            screen_width = full_rect.w;
            screen_height = full_rect.h;
            full_rect.x = 0;
            full_rect.y = 0;
        	SDL_SetWindowPosition(window, 0, 0);
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        }
        else
        {
        	SDL_SetWindowPosition(window, full_rect.w / 6, full_rect.h /6);
            screen_width = full_rect.w / 3 *2;
            screen_height = full_rect.h / 3 * 2;
        	SDL_SetWindowSize(window, screen_width, screen_height);
        }

        DEBUG("screen_width %d screen_height %d", screen_width, screen_height);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        if(window)
        {
     //       renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if(!renderer)
            {
                DEBUG("Failed to initialize a hardware accelerated renderer: %s", SDL_GetError());
                renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE); // SDL_RENDERER_SOFTWARE
            }
            if(renderer)
            {
                if (!SDL_GetRendererInfo(renderer, &renderer_info))
                    DEBUG("Initialized %s renderer.", renderer_info.name);
				ttf_texture = SDL_CreateTextureFromSurface(renderer, surface);
            }
        }
        if(!window || !renderer || !renderer_info.num_texture_formats)
        {
            DEBUG("Failed to create window or renderer: %s", SDL_GetError());
			goto run_out;
        }
    }
    if(server_flag)
    {
		show_window();
        init_display();
        sdl_loop();
    }
	else
	{
#ifndef _WIN32
		int id;
    	if((dpy = XOpenDisplay(0)) == NULL)
    	{
			DIE("XOpenDisplay err");
    	}
    	id = DefaultScreen(dpy);  //DISPLAY:= id
    	if(!(root = XRootWindow(dpy, id)))
    	{
			DIE("XRootWindow err");
    	}
#endif

	}
	DEBUG("display exit");

run_out:
	do_exit();	
	return ERROR;
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
