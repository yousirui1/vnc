#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "msg.h"
#include "queue.h"

/* main.c */
extern int client_port, control_port, h264_port, window_flag, window_size, server_port;
extern int max_connections;
extern char server_ip[126];
extern int default_quality, default_fps;

/* log.c */
void init_logs();
void close_logs();
void log_msg(const char *fmt, ...);
void err_msg(const char *fmt, ...);


/* socket.c */
void *thread_client_udp(void *param);
void *thread_client_tcp(void *param);
void *thread_server_tcp(void *param);
void *thread_server_udp(void *param);

/* ffmpeg.c */
void *thread_encode(void *param);
void *thread_decode(void *param);

/* inirw.h */
int read_profile_string( const char *section, const char *key,char *value, int size,const char *default_value, const char *file);
int read_profile_int( const char *section, const char *key,int default_value, const char *file);
int write_profile_string( const char *section, const char *key,const char *value, const char *file);


/* control.c */
int init_dev();


/* display.c */
void *thread_display(void *param);
extern rfb_display *displays;
extern int width,height, vids_width, vids_height;

extern unsigned char **vids_buf;
extern QUEUE *vids_queue;

extern int display_size;

extern rfb_display client_display ;


#endif

