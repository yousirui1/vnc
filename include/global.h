#ifndef __GLOBAL_H__
#define __GLOBAL_H__

/* main.c */
extern int server_flag;
extern int client_port, control_port, h264_port, window_flag, window_size, server_port;
extern char server_ip[126];


/* display.c */
extern int width,height, vids_width, vids_height;

/* inirw.h */
int read_profile_string( const char *section, const char *key,char *value, int size,const char *default_value, const char *file); 
int read_profile_int( const char *section, const char *key,int default_value, const char *file); 
int write_profile_string( const char *section, const char *key,const char *value, const char *file); 



/* client.c */
void *create_socket(void *param);

#endif


