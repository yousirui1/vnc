#include <libavutil/frame.h>
#include <SDL2/SDL.h>

#include "base.h"

const char program_name[] = "ffplay";

int screen_width  = 0;
int screen_height = 0;
int vids_width = 0;
int vids_height = 0;



static int default_width  = 720;
static int default_height = 640;
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

rfb_display *displays = NULL;
rfb_display *control_display = NULL;
pthread_t *pthread_decodes = NULL;
pthread_mutex_t renderer_mutex;

static int loss_flag = 0;
static int loss_count = 10;

static void do_exit()
{
	if(renderer)
		SDL_DestroyRenderer(renderer);

	if(window)
        SDL_DestroyWindow(window);	
	SDL_Quit();
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
	if(loss_flag)
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
	if(!rect)
	{
	 	SDL_Rect full_rect;
 
		full_rect.x = 0;
		full_rect.y = 0;
		full_rect.w = screen_width;
		full_rect.h = screen_height;

        SDL_UpdateYUVTexture(full_texture, NULL,
        frame_yuv->data[0], frame_yuv->linesize[0],
        frame_yuv->data[1], frame_yuv->linesize[1],
        frame_yuv->data[2], frame_yuv->linesize[2]);

        SDL_RenderCopy(renderer, full_texture, NULL, &full_rect);
	}
	else
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


void init_display()
{
	int i,j, ret, id;
	
	pthread_mutex_init(&renderer_mutex, NULL);
	clear_texture();	
	
	vids_width = screen_width / window_size;	
	vids_height = screen_height / window_size;

	/* 部分区域更新 */
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, vids_width, vids_height);
    if(!texture)
    {
        DIE("create texture err");
    }

	/* 整个窗口更新 */
    full_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);
    if(!full_texture)
    {
        DIE("create texture err");
    }

	displays = (rfb_display *)malloc(sizeof(rfb_display) * window_size * window_size);
    memset(displays, 0, sizeof(rfb_display) * window_size * window_size);
    pthread_decodes = (pthread_t *)malloc(sizeof(pthread_t) * window_size * window_size);	

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

/* 非键盘鼠标事件 */
static void refresh_loop_wait_event(SDL_Event *event)
{
    //double remaining_time = 0.0;
    SDL_PumpEvents();
    while (!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT))
    {
        SDL_PumpEvents();
    }
}


void send_control(char *buf, int data_len, int cmd)
{
	if(!control_display || control_display->play_flag != 2)	
		return;

	set_request_head(control_display->req, cmd, data_len);
	control_display->req->data_buf = buf;
	
	send_request(control_display->req);
	control_display->req->data_buf = NULL;
}

void event_loop()
{
	SDL_Event event;
    rfb_pointevent point;
    rfb_keyevent key;
	for(;;)
	{
		refresh_loop_wait_event(&event);


#if 0

		switch(event.type)
		{
            case SDL_KEYDOWN:   //键盘按下
                if(event.key.keysym.sym == SDLK_ESCAPE)
                {
					switch_mode(0);
                    break;
                }
				else
				{
					DEBUG("SDL_KEYDOWN");
					key.key = *(SDL_GetKeyName(event.key.keysym.sym));
					key.down = 1;
					send_control((char *)&key, sizeof(rfb_keyevent), 0x04);
				}
                break;
     		case SDL_KEYUP:   //键盘抬起
				DEBUG("SDL_KEYUP");
				key.key = *(SDL_GetKeyName(event.key.keysym.sym));
                key.down = 0;
                send_control((char *)&key, sizeof(rfb_keyevent), 0x04);
                break;
            case SDL_QUIT:          //关闭窗口
                run_flag = 0;
                goto run_end;
            default:
                break;
		}
#endif

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
            send_control((char *)&point, sizeof(rfb_pointevent), 0x03);
            point.mask = 0;
        }
		if (SDL_KEYDOWN == event.type) // SDL_KEYUP
        {
            key.key = *(SDL_GetKeyName(event.key.keysym.sym));
			DEBUG("event.key.keysym.sym %s", event.key.keysym.sym);
            key.down = 1;
            send_control((char *)&key, sizeof(rfb_keyevent), 0x04);
			if(event.key.keysym.sym == SDLK_ESCAPE) 
				switch_mode(0);
        }

        if(SDL_KEYUP == event.type)
        {
            key.key = *(SDL_GetKeyName(event.key.keysym.sym));
            key.down = 0;
            send_control((char *)&key, sizeof(rfb_keyevent), 0x04);
        }

		if(SDL_QUIT == event.type)
		{
			run_flag = 0;
            goto run_end;
		}

	}
run_end:
    do_exit();
	
}


void control_msg(rfb_request *req)
{
	switch(read_msg_order(req->head_buf))
	{
		case 0x03:
		{
			rfb_pointevent *point = (rfb_pointevent *)&(req->data_buf[0]);

            DWORD flags = MOUSEEVENTF_ABSOLUTE;
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
            /* 计算相对位置 */
            unsigned long x = (point->x * 65535) / (vids_width )  * (screen_width / screen_width);
            unsigned long y = (point->y * 65535) / (vids_height ) * (screen_height /screen_height);

            mouse_event(flags, (DWORD)x, (DWORD)y, NULL, 0);
            break;
		}
		case 0x04:
		{
			rfb_keyevent *keyboard = (rfb_keyevent *)&(req->data_buf[0]);	
			if(keyboard->down)
            {
                keybd_event(keyboard->key, 0, 0,0);
            }
            else
                keybd_event(keyboard->key, 0,KEYEVENTF_KEYUP,0);
			break;
		}
		case 0x05:
			
		
		case 0x06:
			break;

	}
}


void create_control(int id)
{
	control_display = &displays[id];	
	start_control(control_display);
	
}

void destroy_control()
{
	if(!control_display)
		return;
	stop_control(control_display);	
	control_display = NULL;
}

void switch_mode(int id)
{
	if(displays[id].play_flag == 1)
	{
		DEBUG("switch control");
		pause_all_vids();
		create_control(id);
		loss_flag = 1;
	}
	else if(displays[id].play_flag == 2)
	{
		DEBUG("switch video");
		destroy_control();
		revert_all_vids();
	}
}

void hide_window()
{
	if(!window)
		return
	
	SDL_HideWindow(window);
	SDL_HideWindow(window);
}

void show_window()
{
	if(!window)
		return
	DEBUG("show_window");	
	SDL_ShowWindow(window);
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
        if(borderless)              	//无边框
            flags |= SDL_WINDOW_BORDERLESS;
        else
            ;//flags |= SDL_WINDOW_RESIZABLE;  //可以缩放暂时不支持

        window = SDL_CreateWindow(program_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, default_width, default_height, flags);
        if(!window_flag)
        {
			SDL_Rect full_rect;
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

	//if(server_flag)
		//hide_window();

	if(server_flag)
	{
		show_window();
    	init_display();
    	event_loop();
	}
	else
	{
    	atexit(do_exit);
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
