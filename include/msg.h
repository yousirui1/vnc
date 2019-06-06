#ifndef __MSG_H__
#define __MSG_H__

#include <SDL2/SDL.h>

typedef struct _rfb_client
{
	int tcp_fd;
	int udp_fd;
	int port;
	
	struct sockaddr_in recv_addr;
	struct sockaddr_in send_addr;
	
	/* ip */
	char client_ip[126];
	
	/* is play state */
	int state;	
	
	unsigned char *head_buf;
	/* has read msg head or not ,0 :not 1: yes */
	int has_read_head;
		
	unsigned char *data_buf;
	/* current data position */
	int data_pos;
	/* current data size */
	int data_size;

	unsigned char frame_buf[1024 * 1024];
	int frame_pos;
	int frame_size; 
	unsigned short current_count;

}rfb_client;

typedef struct _rfb_vid
{
	int id;
	int fd;
	SDL_Rect rect;
}rfb_vid;

typedef struct _rfb_head
{
    char syn;
    char entry;
    short cmd;
    int size;
}rfb_head;

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
    char key;
}rfb_keyevent;

typedef struct _rfb_pointer_evnet
{
    short mask;
    short x;
    short y;
}rfb_pointevent;




#endif
