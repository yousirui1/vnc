#include <libavutil/frame.h>
#include <SDL2/SDL.h>

#include "base.h"

const char program_name[] = "remote monitor";

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

static int loss_flag = 0;
static int loss_count = 10;
static int area_id = -1;

rfb_display *displays = NULL;
rfb_display *control_display = NULL;
pthread_t *pthread_decodes = NULL;
pthread_mutex_t renderer_mutex;



static void send_control(char *buf, int data_len, int cmd)
{
	if(!control_display || status != CONTROL)
		return;
	set_request_head(control_display->req, cmd, data_len);
    control_display->req->data_buf = buf;

    send_request(control_display->req);
    control_display->req->data_buf = NULL;
}

static int get_area(int x, int y)
{
	return ((x/vids_width) + (y/vids_height) * window_size);
}

static void simulate_mouse(rfb_pointevent *point)
{
    DWORD flags = MOUSEEVENTF_ABSOLUTE;
    DWORD wheel_movement = 0;
    flags |= MOUSEEVENTF_MOVE;

    if(point->mask & MOUSE_LEFT_DOWN)
    {
        flags |= MOUSEEVENTF_LEFTDOWN;
    }
    if(point->mask & MOUSE_LEFT_UP)
    {
        flags |= MOUSEEVENTF_LEFTUP;
    }
    if(point->mask & MOUSE_RIGHT_DOWN)
    {
        flags |= MOUSEEVENTF_RIGHTDOWN;
    }
    if(point->mask & MOUSE_RIGHT_UP)
    {
        flags |= MOUSEEVENTF_RIGHTUP;
    }

    if(point->wheel)
    {
        flags |= MOUSEEVENTF_WHEEL;
        if(point->wheel > 0)
            wheel_movement = (DWORD)+120;
        else
            wheel_movement = (DWORD)-120;
    }

    /* 计算相对位置 */
    unsigned long x = (point->x * 65535) / (vids_width )  * (screen_width / screen_width);
    unsigned long y = (point->y * 65535) / (vids_height ) * (screen_height /screen_height);
    //DEBUG("x %ld y %ld ", x, y);

    mouse_event(flags, (DWORD)x, (DWORD)y, wheel_movement, 0);
	
}

static void simulate_keyboard(rfb_keyevent *key)
{
	if(key->down)
    {
        keybd_event(key->key, 0, 0,0);
    }
    else
        keybd_event(key->key, 0,KEYEVENTF_KEYUP,0);
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
    while (!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT))
    {
        SDL_PumpEvents();
    }
}


void start_display(rfb_request *req)
{
	//state = DISPLAY;
	
	DEBUG("total %d", total_connections);

	req->data_buf = malloc(sizeof(rfb_format) + 1);	
	memset(req->data_buf, 0, sizeof(rfb_format));
	rfb_format *fmt = (rfb_format *)&req->data_buf[0];	
	
	displays[0].req = req;
	displays[0].play_flag = 1;
	fmt->width = vids_width;
    fmt->height = vids_height;
    fmt->play_flag = 0x01;
    fmt->data_port = 0 + h264_port + 1;
    req->display_id = 0;
	req->status = PLAY;
	process_server_msg(req);
}


void close_display()
{
	DEBUG("total %d", total_connections);
	int i;
	for(i = 0; i < display_size; i++)
	{
		if(displays[i].play_flag == 1)
		{
			//process_server_msg(displays[i].req);
			if(displays[i].req)
				close_fd(displays[i].req->fd);
			displays[i].play_flag = 0;
		}
	}
	usleep(50000);   //等待500ms 客户端结束 
	status = DONE;	//在检测到udp 有数据就断开连接保证下一页数据不被混杂
	usleep(50000);   //等待500ms 客户端结束 
}

void control_msg(rfb_request *req)
{
	switch(read_msg_order(req->head_buf))
	{
		case MOUSE:
			simulate_mouse((rfb_pointevent *)&(req->data_buf[0]));
			break;
		case KEYBOARD:
			simulate_keyboard((rfb_keyevent *)&(req->data_buf[0]));
			break;
		case COPY_TEXT:
			break;
		case COPY_FILE:
			break;
	}
}


void event_loop()
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
					//switch_mode(area_id);
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
			stop_server();		//通过callback 返回给dll
		}
	}	
}


/* 等待线程结束后销毁公共变量 */
void destroy_display()
{
    if(renderer)
        SDL_DestroyRenderer(renderer);

    if(window)
        SDL_DestroyWindow(window);

    if(texture)
        SDL_DestroyTexture(texture);

    if(full_texture)
        SDL_DestroyTexture(full_texture);

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
        if(borderless)                  //无边框
            flags |= SDL_WINDOW_BORDERLESS;
        else
            ;//flags |= SDL_WINDOW_RESIZABLE;  //可以缩放暂时不支持

        window = SDL_CreateWindow(program_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, default_width, default_height, flags);
        if(!window_flag)
        {
            SDL_GetDisplayBounds(0, &full_rect);
            DEBUG("full_width %d full_height %d", full_rect.w, full_rect.h);
            screen_width = full_rect.w;
            screen_height = full_rect.h;
            full_rect.x = 0;
            full_rect.y = 0;
            SDL_SetWindowSize(window, screen_width, screen_height);
            SDL_SetWindowPosition(window, 0, 0);
            //SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        }
        else
        {
            screen_width = default_width;
            screen_height = default_height;
			full_rect.w = screen_width;
			full_rect.h = screen_height;
            full_rect.x = 0;
            full_rect.y = 0;
            //SDL_SetWindowPosition(window, fullRect.x, fullRect.y);
        }
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
            }
        }
        if(!window || !renderer || !renderer_info.num_texture_formats)
        {
            DIE("Failed to create window or renderer: %s", SDL_GetError());
        }
    }
    if(server_flag)
    {
        show_window();
        init_display();
        event_loop();
    }
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


