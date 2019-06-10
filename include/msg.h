#ifndef __MSG_H__
#define __MSG_H__

#define HEAD_LEN 8

#include <SDL2/SDL.h>


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


typedef struct _rfb_display
{
	int id;					//diplay[id]
	int fd;					//udp h264 data fd
	int port;

	SDL_Rect rect;

	struct sockaddr_in recv_addr;
    struct sockaddr_in send_addr;
		
    rfb_request *req;
    int play_flag;

	unsigned char frame_buf[1024 * 1024];
    int frame_pos;
    int frame_size;
	unsigned short current_count;
}rfb_display;

typedef struct _rfb_format
{
    unsigned int width;
    unsigned int height;
    unsigned int code;
    unsigned int data_port;
    unsigned char play_flag;
    unsigned char vnc_flag;     //打开flag  代表主码流,用于全屏显示控制, 否则副码流 480*320
}rfb_format;

typedef struct _rfb_vid
{
    int id;
    int fd;
    SDL_Rect rect;
}rfb_vid;


#endif
