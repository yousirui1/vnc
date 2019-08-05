#ifndef __EXTERNAL_H__
#define __EXTERNAL_H__

#define CAPTUREANDCAST_EXPORTS

#ifdef CAPTUREANDCAST_EXPORTS
#define CAPTUREANDCAST_API  __declspec(dllexport)
#else
#define CAPTUREANDCAST_API  __declspec(dllimport)
#endif

/*
启动启动监控服务 （教师端调用）
@param clientPort 启动成功后返回的实例句柄
@param controlPort j接收端地址,组播地址
@param dataPort 接收端端口
@param winStyleFlag 图像质量0-100
@param pageSize 图像质量0-100
成功返回0, 否则返回对应错误号
*/
CAPTUREANDCAST_API int StartMonitorServer(const int clientPort, const int controlPort, const int dataPort, const int winStyleFlag, const int pageSize);


/*
停止监控服务（教师端调用）
@param clientPort 启动成功后返回的实例句柄
@param controlPort j接收端地址,组播地址
@param dataPort 接收端端口
@param winStyleFlag 图像质量0-100
@param pageSize 图像质量0-100
成功返回0, 否则返回对应错误号
*/
CAPTUREANDCAST_API int StopMonitorServer();


//启动被监控端连接（学生端用）


/*
启动启动监控服务 （教师端调用）
@param clientPort 启动成功后返回的实例句柄
@param controlPort j接收端地址,组播地址
@param dataPort 接收端端口
@param winStyleFlag 图像质量0-100
@param pageSize 图像质量0-100
成功返回0, 否则返回对应错误号
*/
CAPTUREANDCAST_API int StartMonitorClient(const char* serverIp, const int serverPort, const char* clientFlag);
/*
设置分页相关属性
@param clientPort 启动成功后返回的实例句柄
@param controlPort j接收端地址,组播地址
@param dataPort 接收端端口
@param winStyleFlag 图像质量0-100
@param pageSize 图像质量0-100
成功返回0, 否则返回对应错误号
*/

CAPTUREANDCAST_API int SetPageAttribute(const int pageIndex, const int pageSize);

/*
获取分页属性
启动启动监控服务 （教师端调用）
@param clientPort 启动成功后返回的实例句柄
@param controlPort j接收端地址,组播地址
@param dataPort 接收端端口
@param winStyleFlag 图像质量0-100
@param pageSize 图像质量0-100
成功返回0, 否则返回对应错误号
*/
CAPTUREANDCAST_API int GetPageCount();



/*
启动启动监控服务 （教师端调用）
@param clientPort 启动成功后返回的实例句柄
@param controlPort j接收端地址,组播地址
@param dataPort 接收端端口
@param winStyleFlag 图像质量0-100
@param pageSize 图像质量0-100
成功返回0, 否则返回对应错误号
*/


#endif
