#include <libavutil/frame.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#ifndef _WIN32
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>      /* BitmapOpenFailed, etc.    */
    #include <X11/cursorfont.h> /* pre-defined crusor shapes */
    #include <linux/input.h>

    Display *dpy = NULL;
#endif

#include "base.h"
//#include "frame.h"

int screen_width  = 0;
int screen_height = 0;
int vids_width = 0;
int vids_height = 0;

static int default_width  = 1600;
static int default_height = 900;
static int audio_disable = 1;
static int video_disable = 0;
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

static int area_id = -1;

rfb_display *displays = NULL;
rfb_display *control_display = NULL;
pthread_mutex_t renderer_mutex;// = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t display_mutex;// = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t display_cond;// = PTHREAD_COND_INITIALIZER;

unsigned char **vids_buf;
QUEUE *vids_queue;

#if 0
int sfp_refresh_thread(void *opaque)
{
    while (thread_exit==0)
    {
        SDL_Event event;
        event.type = SFM_REFRESH_EVENT;
        SDL_PushEvent(&event);
    }
    //Quit
    SDL_Event event;
    event.type = SFM_BREAK_EVENT;
    SDL_PushEvent(&event);
    return 0;
}
#endif

static int do_exit()
{
	int i ;
	DEBUG("1111111111111");
	if(texture)
		SDL_DestroyTexture(texture);

	DEBUG("1111111111111");
	if(full_texture)
		SDL_DestroyTexture(full_texture);	

	DEBUG("1111111111111");
	if(ttf_texture)
		SDL_DestroyTexture(ttf_texture);	
		
	DEBUG("1111111111111");
	if(font)
		TTF_CloseFont(font);
	
	DEBUG("1111111111111");
	if(renderer)
		SDL_DestroyRenderer(renderer);

	DEBUG("1111111111111");
#if 0
#ifdef _WIN32
        if(!hwnd && window)
			SDL_DestroyWindow(window);	
#endif
#endif
	//if(window)
		//SDL_DestroyWindow(window);
	DEBUG("ssssssssssssss111111111111");

	memset(&rect, 0, sizeof(SDL_Rect));
	memset(&full_rect, 0, sizeof(SDL_Rect));

#ifndef _WIN32
	if(dpy)
		XCloseDisplay(dpy);
	dpy = NULL;
#endif
	
	area_id = -1;
	screen_width = 0;
	screen_height = 0;
	vids_width = 0;
	vids_height = 0;
	
	texture = NULL;
	ttf_texture = NULL;
	full_texture = NULL;
	font = NULL;
	renderer = NULL;
	window = NULL;

	/* 等待使用结束 */
	pthread_mutex_destroy(&renderer_mutex);

	DEBUG("pthread renderer destory");
	TTF_Quit();

#if 0
    if(SDL_WasInit(SDL_INIT_EVERYTHING) != 0)
    {
        DEBUG("SDL_WasInit");
        SDL_Quit();
    }
#endif

	DEBUG("sdl exit ok !!!!!!!!!!!!!");
}

static void send_control(char *data, int data_len, int cmd)
{
	if(!control_display || !control_display->cli || status != CONTROL)
		return;
	char *buf = (char *)malloc(data_len + HEAD_LEN + 1);
	if(!buf)
		return;
		
	char *tmp = &buf[HEAD_LEN];
	set_request_head(buf, 0, cmd, data_len);	
	memcpy(tmp, data, data_len);
	sendto(control_display->cli->control_udp.fd, buf,  data_len + HEAD_LEN, 0,
                    (struct  sockaddr *)&(control_display->cli->control_udp.send_addr),
                    sizeof (control_display->cli->control_udp.send_addr));
	free(buf);
}
#ifdef _WIN32
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
	key->scan_code = 0;
	/* a-z */
	if(key->key >= 97 && key->key <= 122)
	{
		key->key -= 32;
	}
	/* F1-F12 */
	else if(key->key >= 1073741882 && key->key <=1073741893 )
	{
		key->key -= 1073741770;
	}
	/* 0- 9*/
	else if(key->key >= 1073741913 && key->key <= 1073741921)
	{
		key->key -= 1073741864;
	}
	else
	{
		if(key->key == 0x51)		//Q
		{
			key->scan_code = 0x10;
		}
		if(key->key == 0x57)			//W
		{
			key->scan_code = 0x11;
		}
		if(key->key == 0x45)			//E
		{
			key->scan_code = 0x12;
		}	
		if(key->key == 0x52)			//R
		{
			key->scan_code = 0x13;
		}
		if(key->key == 0x41)			//A
		{
			key->scan_code = 0x1E;
		}
		if(key->key == 0x53)			//S
		{
			key->scan_code = 0x1F;
		}
		if(key->key == 0x44)			//D
		{
			key->scan_code = 0x20;
		}
		if(key->key == 0x46)			//F
		{
			key->scan_code = 0x21;
		}
		if(key->key == 0x5A)			//Z
		{
			key->scan_code = 0x2C;
		}
		if(key->key == 0x58)			//X
		{
			key->scan_code = 0x2D;
		}
		if(key->key == 0x43)			//C
		{
			key->scan_code = 0x2E;
		}
		if(key->key == 0x56)			//V
		{
			key->scan_code = 0x2F;
		}

		switch(key->key)
		{
			case 39:   // "'"
				key->key = 0xDE;
				break;
			case 44:	//","
				key->key = 0xBC;
				break;
			case 45:   //"-"
				key->key = 0xBD;
				break;
			case 46:	//"."
				key->key = 0xBE;
				break;
			case 47:   //"/"
				key->key = 0xBF;
				break;
			case 59:	//":"
				key->key = 0xBA;
				break;
			case 61:	// "+"
				key->key = 0xBB;
				break;
			case 91:  // "["
				key->key = 0xDB;
				break;
			case 92:  // "|"
				key->key = 0xDC;
				break;
			case 93:  // "]"
				key->key = 0xDD;
				break;
			case 96:	//"~"
				key->key = 0xC0;
				break;
			case 1073741881: // "caps lock"
				key->key = 0x14;
				break;
			case 1073742048: // "ctrl"
				key->key = 0x11;
				key->scan_code = 0x1D;
				break;
			case 1073742052: // "ctrl"
				key->key = 0x11;
				key->scan_code = 0x1D;
				break;
			case 1073742049: // "shift"
				key->key = 0x10;
				key->scan_code = 0x2A;
				break;
			case 1073742053: // "shift"
				key->key = 0x10;
				key->scan_code = 0x2A;
				break;
			case 1073742050: // "alt"
				key->key = 0x12;
				key->scan_code = 0x38;
				break;
			case 1073742054: // "alt"
				key->key = 0x12;
				key->scan_code = 0x38;
				break;
			case 1073742051: // "win"
				key->key = 0x5B;
				key->scan_code = 0x5B;
				break;
			case 1073742055: // "win"
				key->key = 0x5B;
				key->scan_code = 0x5B;
				break;
			case 1073741906: // "up"
				key->key = 0x26;
				key->scan_code = 0x48
				break;
			case 1073741905: // "down"
				key->key = 0x28;
				key->scan_code = 0x50;
				break;
			case 1073741903: // "right"
				key->key = 0x27;
				key->scan_code = 0x4B;
				break;
			case 1073741904: // "left"
				key->key = 0x25;
				key->scan_code = 0x4D;
				break;
			case 1073741897: // "insert"
				key->key = 0x2D;
				key->scan_code = 0x52;
				break;
			case 127: 		// "delete"
				key->key = 0x2E;
				key->scan_code = 0x53;
				break;
			case 1073741898: // "home"
				key->key = 0x24;
				key->scan_code = 0x47;
				break;
			case 1073741901: // "end"
				key->key = 0x23;
				key->scan_code = 0x4F;
				break;
			case 1073741899: // "pgup"
				key->key = 0x21;
				key->scan_code = 0x49;
				break;
			case 1073741902: // "pgdn"
				key->key = 0x22;
				key->scan_code = 0x51;
				break;
			case 1073741895: // "pause break"
				key->key = 0x13;
				key->scan_code = 0x37
				break;
			case 1073741894: // "scroll lock"
				key->key = 0x91;
				key->scan_code = 0x46;
				break;
#if 0
			case 1073741896: // "ptr scsys rq"
				key->key = 0x25;
				key->scan_code = 0x45;
				break;
#endif
			case 1073741907: //"Num Lock"
				key->key = VK_NUMLOCK;
				break;
			case 1073741912: // "Enter"
				key->key = 13;
				key->scan_code = 0x1C;
				break;
			case 1073741908: //"/"
				key->key = VK_DIVIDE;
				break;
			case 1073741909: // "*"
				key->key = VK_MULTIPLY;
				break;
			case 1073741910: // "-"
				key->key = VK_SUBTRACT;
				break;				
			case 1073741911: // "+"
				key->key = VK_ADD;
				break;
			case 1073741923: // "."
				key->key = VK_DECIMAL;
				break;
			case 1073741922: //"0"
				key->key = 48;
				break;
			default:
				break;
		}
	}	

	if(key->down)
    {
        keybd_event(key->key, key->scan_code, 0,0);
    }
    else
        keybd_event(key->key, key->scan_code, KEYEVENTF_KEYUP,0);
}
#else

static void simulate_mouse(rfb_pointevent *point)
{

	unsigned long x = point->x / (float)vids_width * screen_width;
    unsigned long y = point->y / (float)vids_height * screen_height;

	if(!dpy)
		return;

    XTestFakeMotionEvent(dpy, 0, x, y, 0L);
    int button = 0;
    int down = 0;

    if(point->mask & MOUSE_LEFT_DOWN)
    {   
        button = 1;
        down = 1;
    }   
    if(point->mask & MOUSE_LEFT_UP)
    {   
        button = 1;
        down = 0;
    }   
    if(point->mask & MOUSE_RIGHT_DOWN)
    {   
        button = 3;
        down = 1;
    }   
    if(point->mask & MOUSE_RIGHT_UP)
    {   
        button = 3;
        down = 0;
    }   
    if(point->mask)
        XTestFakeButtonEvent(dpy, button, down, 0L);

    XFlush(dpy);
}

static void simulate_keyboard(rfb_keyevent *key)
{
	if(!dpy)
		return;

	/* a-z */
	if(key->key >= 97 && key->key <= 122)
	{
		key->key -= 32;
	}

	/* F1-F12 */
	else if(key->key >= 1073741882 && key->key <=1073741893 )
	{
		key->key -= 1073676412;
	}
	/* 1- 9*/
	else if(key->key >= 1073741913 && key->key <= 1073741922)
	{
		key->key -= 1073741864;
	}

	else
	{
		switch(key->key)
		{
			case SDLK_ESCAPE:
				key->key = XK_Escape;
				break;
			case SDLK_BACKSPACE:
				key->key = XK_BackSpace;
				break;
			case 39:   // "'"
				key->key = 0x22;
				break;
			case 44:	//","
				key->key = 0x2C;
				break;
			case 45:   //"-"
				key->key = 0x2d;
				break;
			case 46:	//"."
				key->key = 0x2E;
				break;
			case 47:   //"/"
				key->key = 0x2F;
				break;
			case 59:	//":"
				key->key = 0x3A;
				break;
			case 61:	// "+"
				key->key = 0x2B;
				break;
			case 91:  // "["
				key->key = 0x7B;
				break;
			case 92:  // "|"
				key->key = 0x7C;
				break;
			case 93:  // "]"
				key->key = 0x7D;
				break;
			case 96:	//"~"
				key->key = 0x7E;
				break;
			case 13:		//"enter"
				key->key = XK_KP_Enter;
				break;
			case 9:		//"tab"
				key->key = XK_KP_Tab;
				break;
			case 1073741881: // "caps lock"
				key->key = XK_Caps_Lock;
				break;
			case 1073742048: // "ctrl"
				key->key = XK_Control_L;
				break;
			case 1073741925: // "ctrl"
				key->key = XK_Control_R;
				break;
			case 1073742049: // "shift"
				key->key = XK_Shift_L;
				break;
			case 1073742053: // "shift"
				key->key = XK_Shift_R;
				break;
			case 1073742050: // "alt"
				key->key = XK_Alt_L;
				break;
			case 1073742054: // "alt"
				key->key = XK_Alt_R;
				break;
			case 1073742051: // "win"
				key->key = XK_Meta_L;
				break;
			case 1073742055: // "win"
				key->key = XK_Meta_R;
				break;
			case 1073741906: // "up"
				key->key = XK_Up;
				break;
			case 1073741905: // "down"
				key->key = XK_Down;
				break;
			case 1073741903: // "right"
				key->key = XK_Right;
				break;
			case 1073741904: // "left"
				key->key = XK_Left;
				break;
			case 1073741897: // "insert"
				key->key = XK_Insert;
				break;
			case 127: 		// "delete"
				key->key = XK_Delete;
				break;
			case 1073741898: // "home"
				key->key = XK_Home;
				break;
			case 1073741901: // "end"
				key->key = XK_End;
				break;
			case 1073741899: // "pgup"
				key->key = XK_Page_Up;
				break;
			case 1073741902: // "pgdn"
				key->key = XK_Page_Down;
				break;
			case 1073741895: // "pause break"
				key->key = XK_Pause;
				break;
			case 1073741894: // "scroll lock"
				key->key = XK_Scroll_Lock;
				break;
			case 1073741896: // "ptr scsys rq"
				key->key = XK_Sys_Req;
				break;
			//key->key = XK_BackSpace
			default:
				break;
		}
	}	
	XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, key->key), key->down, 0L);
    XFlush(dpy);    
}
#endif




int init_X11()
{
#ifndef _WIN32
    if((dpy = XOpenDisplay(0)) == NULL)
    {
        DEBUG("XOpenDisplay error");
        return ERROR;
    }
#endif
	return SUCCESS;
}

void close_X11()
{
#ifndef _WIN32
	if(dpy)
    	XCloseDisplay(dpy);
	dpy = NULL;
#endif
}


int get_screen_size(int *temp_w, int *temp_h)
{
#ifdef _WIN32
    SDL_GetWindowSize(window, temp_w, temp_h);
    //*tmp_x = FFALIGN(*tmp_x, 16);
    //*tmp_y = FFALIGN(*tmp_y, 16);
    return SUCCESS;
#else
    int id;
    Window root;

    if(!dpy)
        return ERROR;

    id = DefaultScreen(dpy);
    if(!(root = XRootWindow(dpy, id)))
    {
        DEBUG("get XRootWindow id: %d error", id);
        if(dpy)
            XCloseDisplay(dpy);
        return ERROR;
    }

    *temp_w = DisplayWidth(dpy, id);
    *temp_h = DisplayHeight(dpy, id);

    if(!(*temp_w) || !(*temp_h))
        return ERROR;
    else
        return SUCCESS;
#endif
}


static int get_area(int x, int y)
{
    return ((x/vids_width) + (y/vids_height) * window_size);
}


void sdl_text_show(char *buf, SDL_Rect *rect)
{
    SDL_Color color = {255, 255, 255};
    /*
     * Solid  渲染的最快，但效果最差，文字不平滑，是单色文字，不带边框。
  　 * Shaded 比Solid渲染的慢，但显示效果好于Solid，带阴影。
　　 * Blend 渲染最慢，但显示效果最好。
     */
    SDL_Surface *surface = TTF_RenderUTF8_Solid(font, buf, color);
	if(ttf_texture)
		SDL_DestroyTexture(ttf_texture);
    ttf_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect ttf_rect;
    ttf_rect.x = rect->x + (vids_width / 2) - (surface->w / 2);
    ttf_rect.y = rect->y + vids_height - (surface->h * 1.2);
    ttf_rect.w = surface->w;
    ttf_rect.h = surface->h;

    SDL_BlitSurface(surface, NULL, window, &ttf_rect);
    SDL_RenderCopy(renderer, ttf_texture, NULL, &ttf_rect);
    SDL_RenderPresent(renderer);

	SDL_FreeSurface(surface);
}

void sdl_text_clear()
{

}

void update_texture(AVFrame *frame_yuv, SDL_Rect *rect)
{
    if(!texture || !renderer || !full_texture || !frame_yuv)
        return;
    pthread_mutex_lock(&renderer_mutex);
    if(!rect)                               //全屏更新
    {
        SDL_UpdateYUVTexture(full_texture, NULL,
        frame_yuv->data[0], frame_yuv->linesize[0],
        frame_yuv->data[1], frame_yuv->linesize[1],
        frame_yuv->data[2], frame_yuv->linesize[2]);

        SDL_RenderCopy(renderer, full_texture, NULL, &full_rect);
    }
    else                                    //局部更新
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

void clear_texture()
{
    pthread_mutex_lock(&renderer_mutex);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    pthread_mutex_unlock(&renderer_mutex);
}


void switch_mode(int id)
{
	int ret = ERROR;
    if(status == PLAY)
    {
        DEBUG(" switch CONTROL status");
        status = CONTROL;
        stop_display();
        ret = start_control(id);
		if(ret != SUCCESS)
			status = PLAY;
    }
    else if(status == CONTROL)
    {
        DEBUG(" switch PLAY status");
        status = PLAY;
        stop_control();
        start_display();
    }
}

void control_msg(char *buf)
{
    switch(read_msg_order(buf))
    {
        case MOUSE:
            simulate_mouse((rfb_pointevent *)&(buf[HEAD_LEN]));
            break;
        case KEYBOARD:
            simulate_keyboard((rfb_keyevent *)&(buf[HEAD_LEN]));
            break;
        case COPY_TEXT:
            break;
        case COPY_FILE:
            break;
    }
}

int run_flag = 0;

static void refresh_loop_wait_event(SDL_Event *event)
{
    //double remaining_time = 0.0;
    SDL_PumpEvents();
    while (!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) && run_flag)
    {
        //usleep(200000);
        SDL_PumpEvents();
    }
}

void sdl_loop()
{
	SDL_Event event;
	rfb_pointevent point = {0};
	rfb_keyevent key = {0};
	short old_x = 0;
	short old_y = 0;	
		
	time_t last_time = current_time;
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
			//stop_server();		//通过callback 返回给dll
#endif
		}
	}	

	DEBUG("event_loop end");
	close_window();
}
	

int init_display()
{
	int i, ret, x = 0, y = 0;
    vids_width = screen_width / window_size;
    vids_height = screen_height / window_size;
	display_size = window_size * window_size;
    vids_queue = (QUEUE *)malloc(sizeof(QUEUE) * (display_size + 1));
    vids_buf = (unsigned char **)malloc(display_size  * sizeof(unsigned char *));

    displays = (rfb_display *)malloc(sizeof(rfb_display) * (display_size + 1));

    /* 局部画板 */
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, vids_width, vids_height);

    /* 全局画板 */
    full_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);

    if(!vids_queue||!vids_buf||!displays||!texture||!full_texture)
    {
        DEBUG("display_size: %d malloc vids_queue vids_buf texture full_texture and displays one error :%s",
                display_size, strerror(errno));
        goto run_out;
    }
	memset(displays, 0, sizeof(rfb_display) * (display_size + 1));
	DEBUG("init display malloc OK!");

	pthread_mutex_init(&renderer_mutex, NULL);

	for(i = 0; i <= display_size; i++)
	{
        displays[i].id = i;
        x = i % window_size;
        y = i / window_size;

        DEBUG("x = %d y = %d ", x, y);
        displays[i].rect.x = x * vids_width;
        displays[i].rect.y = y * vids_height;
        displays[i].rect.w = vids_width;
        displays[i].rect.h = vids_height;

        pthread_mutex_init(&displays[i].mtx,NULL);
        pthread_cond_init(&displays[i].cond, NULL);

        vids_buf[i] = (unsigned char *)malloc(MAX_VIDSBUFSIZE * sizeof(unsigned char));
        if(!vids_buf[i])
        {
            DEBUG("vids_buf malloc sizeof: %d error: %s", MAX_VIDSBUFSIZE * sizeof(unsigned char),
                    strerror(errno));
            goto run_out;

        }
        memset(vids_buf[i], 0, MAX_VIDSBUFSIZE);
        /* 创建窗口的对应队列 */
        init_queue(&(vids_queue[i]), vids_buf[i], MAX_VIDSBUFSIZE);
	}	

    control_display = &displays[display_size];
    control_display->rect.x = screen_width;;
    control_display->rect.y = screen_height;
    control_display->rect.w = 0;
    control_display->rect.h = 0;


	DEBUG("init display param init OK!");
	pthread_mutex_lock(&display_mutex);
	pthread_cond_signal(&display_cond);
	pthread_mutex_unlock(&display_mutex);

	DEBUG("init display signal OK!");
    clear_texture();
    status = PLAY;
    DEBUG("init display end");

    return SUCCESS;
run_out:
    if(vids_queue)
        free(vids_queue);
	for(i = 0; i<= display_size; i++)
	{
		if(vids_buf[i])
			free(vids_buf[i]);
		vids_buf[i] = NULL;
	}
    if(vids_buf)
        free(vids_buf);
    if(displays)
        free(displays);
    if(texture)
        SDL_DestroyTexture(texture);
    if(full_texture)
        SDL_DestroyTexture(full_texture);
	vids_queue = NULL;	
	vids_buf = NULL;
	displays = NULL;
	texture = NULL;
	full_texture = NULL;
	control_display = NULL;

	//pthread_en();  发送sig
    return ERROR;
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

void close_window()
{
	char stop = 'S';
	send_msg(pipe_event[1], &stop, 1);
	do_exit();
}

int create_window()
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
    font = TTF_OpenFont(TTF_DIR, 16);
    if(!font)
    {
        DEBUG("SDL_ttf open ttf dir: %s error", TTF_DIR);
        goto run_out;
    }

    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

    if(!display_disable)
    {
        int flags = SDL_WINDOW_SHOWN;  //SDL_WINDOW_SHOWN SDL_WINDOW_HIDDEN
        if(borderless)                  //无边框
            flags |= SDL_WINDOW_BORDERLESS;
        else
            flags |= SDL_WINDOW_RESIZABLE;  //可以缩放暂时不支持

#ifdef _WIN32
        if(hwnd)
            window = SDL_CreateWindowFrom(hwnd);
#endif
		if(!window)
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

			full_rect.w = screen_width;
			full_rect.h = screen_height;
			full_rect.x = 0;
			full_rect.y = 0;
        	SDL_SetWindowSize(window, screen_width, screen_height);
        }

        DEBUG("screen_width %d screen_height %d", screen_width, screen_height);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        if(window)
        {
     		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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
            DEBUG("Failed to create window or renderer: %s", SDL_GetError());
			goto run_out;
        }
    }
	DEBUG("create SDL window ok !");
	return SUCCESS;
run_out:
	do_exit();	
	return ERROR;
}

/* 服务端关闭从这个线程开始 */
void *thread_sdl(void *param)
{
    int ret;
    pthread_attr_t st_attr;
    struct sched_param sched;

    ret = pthread_attr_init(&st_attr);
    if(ret)
    {
        DEBUG("Thread SDL attr init error ");
    }
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {
        DEBUG("Thread SDL set SCHED_FIFO error");
    }
    sched.sched_priority = SCHED_PRIORITY_SDL;
    ret = pthread_attr_setschedparam(&st_attr, &sched);
	
	ret = create_window();
	if(SUCCESS != ret)
	{
		DEBUG("create SDL window error");
		goto run_out;
	}
	ret = init_display();
	if(SUCCESS != ret)
	{
		DEBUG("init display error");
		goto run_out;
	}
    sdl_loop();
    return (void *)ret;

run_out:
	close_window();
    return (void *)ret;
}
