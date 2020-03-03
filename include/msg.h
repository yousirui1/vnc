#ifndef __MSG_H__
#define __MSG_H__

#include <SDL2/SDL.h>
#include "rbtree.h"

#define IPADDR_LEN 128
#define SZ_VERFORMAT 20
#define HEAD_LNE 8

enum control_msg_type{
	MOUSE = 0x03,
	KEYBOARD,
	COPY_TEXT,
	COPY_FILE	
};

struct sock_udp
{
	unsigned int fd;
	unsigned int port;
	
    struct sockaddr_in recv_addr;
    struct sockaddr_in send_addr;
};

typedef struct _req_head
{
    unsigned char syn;
    unsigned char encrypt_flag;
    unsigned short cmd;
    unsigned int data_size;
}req_head;

typedef struct _rfb_format
{
	unsigned int width;
    unsigned int height;
    unsigned int code;
    unsigned int data_port;
    unsigned int control_port;
    unsigned char play_flag;            // 0 stop 1 play  2 control
    unsigned char fps;
    unsigned int bps;

}rfb_format;

struct client
{
    short index;
	short display_id;

    unsigned int status;

    /* red black node */
    struct rb_node rb_node;

    struct client *next;
    struct client *prev;

    /* listen fd */
    int listenfd;

    /* tcp sock fd */
    int fd;
    char ip[IPADDR_LEN];
    int port;

	/* udp sock */
	struct sock_udp control_udp;
	
    unsigned char *head_buf;
    /** has read msg head or not ,0 :not 1: yes**/
    int has_read_head ;

    unsigned char *data_buf;

    /** current data position **/
    int pos;
    /** curreant data size **/
    int data_size;
    /** max alloc size **/
    int max_size;

    time_t last_time;
}__attribute__((packed,aligned(8)));

typedef struct _rfb_display
{
    int id;
    int fd;
    int port;

    SDL_Rect rect;

    struct sockaddr_in recv_addr;
    struct sockaddr_in send_addr;

	struct sock_udp h264_udp;

    struct client *cli;
    rfb_format fmt;

    pthread_mutex_t mtx;
    pthread_cond_t cond;

    pthread_t pthread_decode;

    unsigned char frame_buf[1024 * 1024];
    int frame_pos;
    int frame_size;
    unsigned short current_count;

	char play_flag;

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
	int scan_code;
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
