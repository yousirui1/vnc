#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "msg.h"
#include "queue.h"

#define MAX_VIDSBUFSIZE 1024 * 1024

/* main.c */
extern int server_flag;
extern int client_port, control_port, h264_port, window_flag, window_size, server_port;
extern int max_connections;
extern char server_ip[126];


/* display.c */
extern int width,height, vids_width, vids_height;
//extern QUEUE *vids_queue;

/* inirw.h */
int read_profile_string( const char *section, const char *key,char *value, int size,const char *default_value, const char *file);
int read_profile_int( const char *section, const char *key,int default_value, const char *file);
int write_profile_string( const char *section, const char *key,const char *value, const char *file);


/* server.c */
//extern int pending_requests;

/* client.c */
void *create_socket(void *param);

/* socket.c */
extern int total_connections;

extern rfb_display *displays;


extern unsigned char **vids_buf;
extern QUEUE *vids_queue;
#endif

