#include <X11/Xlib.h>
#include <X11/Xutil.h>      /* BitmapOpenFailed, etc.    */
#include <X11/cursorfont.h> /* pre-defined crusor shapes */

#include "base.h"

static int fd_kbd = 0;
static int fd_mouse  = 0;
static Display *dpy = NULL;
static Window root = NULL;


int init_dev()
{
	int id ,ret;
	if((dpy = XOpenDisplay(0)) == NULL)
	{
		DEBUG("");
		goto err;
	}
	id = DefaultScreen(dpy);  //DISPLAY:= id
	if((root = XRootWindow(dpy, id)) == NULL)
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
}


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
    
    /* EV_KEY 键盘 
     * EV_REL 相对坐标 鼠标
     * EV_ABS 绝对坐标 手写板
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
