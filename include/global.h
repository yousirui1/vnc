#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "queue.h"
#include "msg.h"


/* main.c */
extern int server_flag;
extern int client_port, control_port, h264_port, window_flag, window_size, server_port;
extern int max_connections;
extern char* server_ip;
extern int default_quality, default_fps;
extern int default_size;
extern int play_flag;
extern int control_flag;
extern int status;
extern time_t last_time;
extern time_t current_time;
extern const char program_name[];

extern int pipe_tcp[2];
extern int pipe_udp[2];
extern int pipe_event[2];

/* log.c */
void init_logs();
void close_logs();
void log_msg(const char *fmt, ...);
void err_msg(const char *fmt, ...);

extern int run_flag;

/* event.c */
void *thread_event(void *param);

/* server.c */
extern struct client **clients;
extern pthread_mutex_t clients_mutex;

/* client.c */
extern struct client m_client;

/* external.c */
#ifdef _WIN32
	extern HWND hwnd;
#endif
//int init_server();
//int init_client();


/* socket.c */
extern unsigned char **vids_buf;
extern QUEUE *vids_queue;
void *thread_server_tcp(void *param);
void *thread_server_udp(void *param);
void *thread_client_tcp(void *param);
void *thread_client_udp(void *param);
struct sock_udp create_udp(char *ip, int port);

/* display.c */
extern int screen_width;
extern int screen_height;
extern int vids_width;
extern int vids_height;
extern int display_size;
extern rfb_display *displays;
extern rfb_display cli_display;
extern rfb_display *control_display;
extern pthread_mutex_t renderer_mutex;
extern pthread_mutex_t display_mutex;
extern pthread_mutex_t display_cond;
void *thread_sdl(void *param);

extern int run_flag;

/* ffmpeg.c */
void *thread_decode(void *param);
void *thread_encode(void *param);


#endif  //__GLOBAL_H__
