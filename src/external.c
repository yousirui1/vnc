#include "base.h"
#include "external.h"

#ifdef _WIN32
/*
启动启动监控服务 （教师端调用）
@param clientPort 
@param controlPort j接收端地址,组播地址
@param dataPort 接收端端口
@param winStyleFlag 图像质量0-100
@param pageSize 图像质量0-100
成功返回0, 否则返回对应错误号
*/

static stop_callback call_back;


CAPTUREANDCAST_API int StartMonitorServer(const int clientPort, const int controlPort, const int dataPort, const int winStyleFlag, const int pageSize, stop_callback call)
{
	server_flag = 1;
	run_flag = 1;

	status = NORMAL;
#ifdef __DEBUG__
	init_logs();
#endif
	client_port = clientPort;
	control_port = controlPort;
	h264_port = dataPort;
	window_flag = winStyleFlag;
	window_size = pageSize;

	DEBUG("client_port %d, control_port %d , h264_port %d  window_flag %d window_size %d",
			client_port, control_port, h264_port, window_flag, window_size);

	call_back = call;
	init_server();
	return 0;
}

/*
停止监控服务（教师端调用）
@param clientPort 启动成功后返回的实例句柄
@param controlPort j接收端地址,组播地址
@param dataPort 接收端端口
@param winStyleFlag 图像质量0-100
@param pageSize 图像质量0-100
成功返回0, 否则返回对应错误号
*/
CAPTUREANDCAST_API int StopMonitorServer()
{
	run_flag = 0;
   	do_exit();
#ifdef __DEBUG__
	close_logs();
#endif
	DEBUG("StopMonitorServer end");
	return 0;
}



CAPTUREANDCAST_API int DisconnectAllClient()
{
	DEBUG("DisconnectAllClient");
	close_display();	
	return 0;		
}


CAPTUREANDCAST_API int ExitControl()
{
	switch_mode(0);
	return 0;
}


/*
启动被监控端连接（学生端用）
@param clientPort 启动成功后返回的实例句柄
@param controlPort j接收端地址,组播地址
@param dataPort 接收端端口
@param winStyleFlag 图像质量0-100
@param pageSize 图像质量0-100
成功返回0, 否则返回对应错误号
*/

CAPTUREANDCAST_API int StartMonitorClient(const char* serverIp, const int serverPort, const char* clientFlag)
{
	server_ip = strdup(serverIp);	
	server_port = serverPort;

	run_flag = 1;

	DEBUG("server_ip %s, server_port %d",
			server_ip, server_port);
#ifdef __DEBUG__
	init_logs();
#endif
	init_client();
	
	return 0;
}


/*
设置分页相关属性
@param clientPort 启动成功后返回的实例句柄
@param controlPort j接收端地址,组播地址
@param dataPort 接收端端口
@param winStyleFlag 图像质量0-100
@param pageSize 图像质量0-100
成功返回0, 否则返回对应错误号
*/
CAPTUREANDCAST_API int SetPageAttribute(const int pageIndex, const int pageSize)
{


}

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
CAPTUREANDCAST_API int GetPageCount()
{
	return 1;
}


void stop_server()
{
	if(call_back)
		call_back();
	call_back = NULL;
}


#endif
