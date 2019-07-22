#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <libavutil/frame.h>
#include "msg.h"
#include "queue.h"

/* main.c */
extern int server_flag;
extern int client_port, control_port, h264_port, window_flag, window_size, server_port;
extern int max_connections;
extern char server_ip[126];
extern int default_quality, default_fps;
extern int run_flag;
extern int play_flag;
extern int control_flag;



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
void h264_send_data(char *data, int len, int key_flag);
int create_tcp();
void connect_server(int fd, const char *ip, int port);
int bind_server(int fd, int port);
int send_msg(const int fd, const char *buf, const int len);
int recv_msg(const int fd,char* buf, const int len);
int send_request(rfb_request *req);
unsigned char read_msg_syn(unsigned char* buf);
unsigned short read_msg_order(unsigned char * buf);
int read_msg_size(unsigned char * buf);



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
extern int screen_height, screen_width;

void update_texture(AVFrame *frame_yuv, SDL_Rect *rect);
void clear_texture();
void send_control(char *buf, int data_len, int cmd);
int init_x11();


/* server.c */
void init_server();
int process_msg(rfb_request *req);


/* client.c */
void init_client();
extern rfb_request *client_req;

#endif

