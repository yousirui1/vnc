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
#define DEBUG(format,...) printf("File: "__FILE__", Line: %05d: "format"\n", __LINE__, ##__VA_ARGS__);\
                        log_msg("File: "__FILE__", Line: %05d: "format"\n", __LINE__, ##__VA_ARGS__);

#define DIE(format,...) printf("File: "__FILE__", Line: %05d: "format"\n", __LINE__, ##__VA_ARGS__);\
                        err_msg("File: "__FILE__", Line: %05d: "format"\n", __LINE__, ##__VA_ARGS__);\
                        exit(1)
#else
#define DEBUG(format,...)
#define DIE(format,...) DEBUG(format);exit(1)
#endif

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
#define VERSIONFORMAT "RFB %03d.%03d\n"

#define REQUEST_TIMEOUT             60

/* req->status */
#define READ_HEADER             0
#define LOGIN                   1
#define OPTIONS                 2
#define DONE                    3
#define DEAD                    4



/* pthread level */
#define SCHED_PRIORITY_TCP 1
#define SCHED_PRIORITY_UDP 2
#define SCHED_PRIORITY_DISPLAY 3


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

#ifndef __cplusplus
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#endif
