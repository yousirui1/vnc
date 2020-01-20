#ifndef __MSG_H__
#define __MSG_H__

#include <SDL2/SDL.h>

#define HEAD_LEN 8
#define sz_verformat 20

enum control_msg_type{
	MOUSE = 0x03,
	KEYBOARD,
	COPY_TEXT,
	COPY_FILE	
};



struct rfb_request
{
    int fd;                     /* client's socket fd */
    int status;                 /* see #defines.h */
    time_t time_last;           /* time of last succ. op. */
    struct rfb_request *next;       /* next */
    struct rfb_request *prev;       /* previous */

    unsigned char head_buf[HEAD_LEN];
    /* has read msg head or not ,0 :not 1: yes */
    int has_read_head;

    unsigned char *data_buf;
    /* current data position */
    int data_pos;
    /* current data size */
    int data_size;
    char ip[16];
    int display_id;
};

typedef struct rfb_request rfb_request;


typedef struct _rfb_head
{   
    unsigned char syn;
    unsigned char encrypt_flag;
    unsigned short cmd;
    unsigned int data_size;
}rfb_head;

#pragma pack(2)
typedef struct _rfb_packet
{
    char b_keyFrame;
    char encodec;
    short width;
    short height;
    short total_num;
    short num;
    short length;
}rfb_packet;


struct rfb_format
{
    unsigned int width;
    unsigned int height;
    unsigned int code;
    unsigned int data_port;
    unsigned char play_flag;   			// 0 stop 1 play  2 control
	unsigned char fps;
	unsigned int bps;
};

typedef struct rfb_format rfb_format;

typedef struct _rfb_display
{
    int id;                 //diplay[id]
    int fd;                 //udp h264 data fd
    int port;

    SDL_Rect rect;

    struct sockaddr_in recv_addr;
    struct sockaddr_in send_addr;
            
	pthread_mutex_t mtx;
    pthread_cond_t cond;


    rfb_request *req;
	rfb_format fmt;
	
    char play_flag;

    unsigned char frame_buf[1024 * 1024];
    int frame_pos;
    int frame_size;
    unsigned short current_count;
}rfb_display;



typedef struct _rfb_filemsg
{
    char flags;
    short datasize;
    char path[128];
}rfb_filemsg;

typedef struct _rfb_textmsg
{   
    short pad1;
    short pad2;
    int length;
}rfb_textmsg;
    
typedef struct _rfb_key_event
{   
    char down;
    int key;			//SDL keycode
	unsigned short mod;			//组合键	
}rfb_keyevent;
    
typedef struct _rfb_pointer_evnet
{   
    short mask;
    short x;
    short y;
	short wheel;
}rfb_pointevent;


#endif
