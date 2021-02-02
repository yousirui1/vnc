#include "base.h"
#ifndef _WIN32
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>      /* BitmapOpenFailed, etc.    */
    #include <X11/cursorfont.h> /* pre-defined crusor shapes */
    #include <linux/input.h>
#endif

#ifdef _WIN32
void simulate_mouse(rfb_pointevent *point)
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


void simulate_keyboard(rfb_keyevent *key)
{

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
				break;
			case 1073742052: // "ctrl"
				key->key = 0x11;
				break;
			case 1073742049: // "shift"
				key->key = 0x10;
				break;
			case 1073742053: // "shift"
				key->key = 0x10;
				break;
			case 1073742050: // "alt"
				key->key = 0x12;
				break;
			case 1073742054: // "alt"
				key->key = 0x12;
				break;
			case 1073742051: // "win"
				key->key = 0x5B;
				break;
			case 1073742055: // "win"
				key->key = 0x5B;
				break;
			case 1073741906: // "up"
				key->key = 0x26;
				break;
			case 1073741905: // "down"
				key->key = 0x28;
				break;
			case 1073741903: // "right"
				key->key = 0x27;
				break;
			case 1073741904: // "left"
				key->key = 0x25;
				break;
			case 1073741897: // "insert"
				key->key = 0x2D;
				break;
			case 127: 		// "delete"
				key->key = 0x2E;
				break;
			case 1073741898: // "home"
				key->key = 0x24;
				break;
			case 1073741901: // "end"
				key->key = 0x23;
				break;
			case 1073741899: // "pgup"
				key->key = 0x21;
				break;
			case 1073741902: // "pgdn"
				key->key = 0x22;
				break;
			case 1073741895: // "pause break"
				key->key = 0x13;
				break;
			case 1073741894: // "scroll lock"
				key->key = 0x91;
				break;
#if 0
			case 1073741896: // "ptr scsys rq"
				key->key = 0x25;
				break;
#endif
			case 1073741907: //"Num Lock"
				key->key = VK_NUMLOCK;
				break;
			case 1073741912: // "Enter"
				key->key = 13;
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
        keybd_event(key->key, 0, 0,0);
    }
    else
        keybd_event(key->key, 0,KEYEVENTF_KEYUP,0);
}
#else

void simulate_mouse(rfb_pointevent *point)
{

	unsigned long x = point->x / (float)vids_width * screen_width;
    unsigned long y = point->y / (float)vids_height * screen_height;

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

void simulate_keyboard(rfb_keyevent *key)
{
	DEBUG("key->key %d", key->key);
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
