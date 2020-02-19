#ifndef __EXTERNAL_H__
#define __EXTERNAL_H__

#define CAPTUREANDCAST_EXPORTS

#ifdef CAPTUREANDCAST_EXPORTS
#define CAPTUREANDCAST_API  __declspec(dllexport)
#else
#define CAPTUREANDCAST_API  __declspec(dllimport)
#endif


typedef void(WINAPI *stop_callback)();

/*
启动启动监控服务 （教师端调用）
@param dc 窗口句柄
@param clientPort 客户端连接端口
@param controlPort 控制端口
@param dataPort 数据端口
@param winStyleFlag 图像质量0-100
@param pageSize 图像质量0-100
@param call 回调函数设置
成功返回0, 否则返回对应错误号
*/
CAPTUREANDCAST_API int StartMonitorServer(HDC dc, const int clientPort, const int controlPort, const int dataPort, const int winStyleFlag, const int pageSize, stop_callback call);


/*
停止监控服务（教师端调用）
成功返回0, 否则返回对应错误号
*/
CAPTUREANDCAST_API int StopMonitorServer();


/*
设置分页相关属性
@param pageIndex 第几页
@param pageSize 这一页有多少个客户端
@param info 客户端信息
成功返回0, 否则返回对应错误号
*/
struct StudentInfo
{
    unsigned int No;
    unsigned char Name[128];
    unsigned char Ip[128];   
    /* data */
};

CAPTUREANDCAST_API int SetPageAttribute(const int pageIndex, const int pageSize, struct StudentInfo *info);
/*
获取分页属性
启动启动监控服务 （教师端调用）
成功返回0, 否则返回对应错误号
*/
CAPTUREANDCAST_API int GetPageCount();

/*
断开所有客户端连接（教师端调用）
翻页使调用
成功返回0, 否则返回对应错误号
*/
CAPTUREANDCAST_API int DisconnectAllClient();
//启动被监控端连接（学生端用）

/*
启动启动监控服务 （教师端调用）
@param serverIp 服务端ip
@param serverPort 服务端端口
@param clientFlag 客户端flag
成功返回0, 否则返回对应错误号
*/
CAPTUREANDCAST_API int StartMonitorClient(const char* serverIp, const int serverPort);


/*
停止服务 （教师端和学生端调用）
成功返回0, 否则返回对应错误号
*/
CAPTUREANDCAST_API int ExitControl();


#endif
