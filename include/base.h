#ifndef __BASE_H__
#define __BASE_H__

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>    
#else
    #include <sys/syscall.h>
    #include <sys/epoll.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/wait.h>
    #include <sys/utsname.h>
    #include <sys/resource.h>


    #include <netinet/tcp.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
#endif

#include "global.h"

#define __DEBUG__
#ifdef __DEBUG__
#define DEBUG(format,...) printf("File: "__FILE__", Line: %05d: "format"\r\n", __LINE__, ##__VA_ARGS__);\
                        log_msg("File: "__FILE__", Line: %05d: "format"\r\n", __LINE__, ##__VA_ARGS__);

#define DIE(format,...) printf("File: "__FILE__", Line: %05d: "format"\r\n", __LINE__, ##__VA_ARGS__);\
                        err_msg("File: "__FILE__", Line: %05d: "format"\r\n", __LINE__, ##__VA_ARGS__);\
                        exit(1)
#else
#define DEBUG(format,...)
#define DIE(format,...) DEBUG(format);exit(1)
#endif

#define SUCCESS 0
#define ERROR 	1


/* config */
#define CONFIG_FILE "config.ini"

#define BASE_SECTION "base"
#define BASE_TYPE_KEY "type"

#define SERVER_SECTION "server"
#define SERVER_CLIENT_PORT_KEY "client_port"
#define SERVER_CONTROL_PORT_KEY "control_port"
#define SERVER_H264_PORT_KEY "h264_port"
#define SERVER_WINDOW_FLAG_KEY "window_flag"
#define SERVER_WINDOW_SIZE_KEY "window_size"            //2 = 2 * 2 4个窗口显示

#define CLIENT_SECTION "client"
#define CLIENT_IP_KEY "server_ip"
#define CLIENT_PORT_KEY "server_port"
#define CLIENT_QUALITY_KEY "quality"
#define CLIENT_FPS_KEY "fps"

#define DEFAULT_TYPE "client"

#define DEFAULT_CLIENT_PORT_VALUE 22000
#define DEFAULT_CONTROL_PORT_VALUE 23000
#define DEFAULT_H264_PORT_VALUE 23001
#define DEFAULT_WINDOW_FLAG_VALUE 0
#define DEFAULT_WINDOW_SIZE_VALUE 1             //2 = 2 * 2 4个窗口显示

#define DEFAULT_QUALITY_VALUE 60
#define DEFAULT_FPS_VALUE 25

#define DEFAULT_IP_VALUE "192.169.27.164"
#define DEFAULT_PORT_VALUE 22000

/* sock */
#define VERSIONFORMAT "RFB %03d.%03d"

#define REQUEST_TIMEOUT             60

/* req->status */
#define NORMAL					0
#define READ_HEADER             1
#define LOGIN                   2
#define OPTIONS                 3
#define PLAY					4
#define CONTROL					5
#define DONE                    6
#define DEAD                    7



/* pthread level */
#define SCHED_PRIORITY_TCP 1
#define SCHED_PRIORITY_UDP 2
#define SCHED_PRIORITY_DISPLAY 3
#define SCHED_PRIORITY_THRIFT 4


/* packet */
#define MAX_CLIENT_NR 1024*1024
#define HEAD_LEN 8
#define CLIENT_BUF 1024*20

#define DATA_SYN 0xFF
#define DATA_SYN_OFFSET 0
#define DATA_ENCRYPT 1
#define DATA_ORDER_OFFSET 2
#define DATA_LEN_OFFSET 4

#define HEAD_LEN 8

/* queue */
#define MAX_VIDSBUFSIZE 1024 * 1024
#define DATA_SIZE 1452



/* control */
#define  MOUSE_LEFT_DOWN  1<<1
#define  MOUSE_LEFT_UP  1<<2

#define  MOUSE_RIGHT_DOWN  1<<3
#define  MOUSE_RIGHT_UP  1<<4

#define MOUSE_WHEEL_DOWN 1<<5
#define MOUSE_WHEEL_UP 	1<<6


/* msg */
#define LOGIN_MSG               0x01
#define LOGIN_MSG_RET           0x02
#define OPTIONS_MSG             0x03
#define OPTIONS_MSG_RET         0x04
#define READY_MSG               0x05
#define READY_MSG_RET           0x06
#define PLAY_MSG                0x07
#define PLAY_MSG_RET            0x08


/* pipe type */
#define CLIENT_MSG              0x00
#define SERVER_MSG              0x01
#define USB_MSG                 0x02
#define EVENT_MSG               0x03
#define THRIFT_MSG              0x04


/* pipe msg */
#define CLI_UDP_MSG             0x01
#define CLI_READY_MSG           0x02
#define CLI_PLAY_MSG            0x03
#define CLI_CONTROL_MSG         0x04
#define CLI_DONE_MSG            0x05
#define CLI_DEAD_MSG            0x06

#define SER_READY_MSG           0x01
#define SER_PLAY_MSG            0x02
#define SER_DONE_MSG            0x03




#ifndef __cplusplus
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#endif
