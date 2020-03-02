#include "base.h"
#include "external.h"

#ifdef _WIN32
HWND hwnd = NULL;
static stop_callback call_back;

/*
启动启动监控服务 （教师端调用）
@param clientPort 
@param controlPort j接收端地址,组播地址
@param dataPort 接收端端口
@param winStyleFlag 图像质量0-100
@param pageSize 图像质量0-100
成功返回0, 否则返回对应错误号
*/
CAPTUREANDCAST_API int StartMonitorServer(HDC dc, const int clientPort, const int controlPort, const int dataPort, const int winStyleFlag, const int pageSize, stop_callback call)
{
	init_logs();
	server_flag = 1;
	status = NORMAL;
	client_port = clientPort;
	control_port = controlPort;
	h264_port = dataPort;
	window_flag = winStyleFlag;
	window_size = pageSize;
	
	DEBUG("window_flag %d", window_flag);
	DEBUG("window_size %d", window_size);
    if(!dc || window_size <= 0 || window_size >5)
    {
        DEBUG("HDC or callback function is NULL!!");
        return ERROR;
    }

    hwnd = WindowFromDC(dc);

    DEBUG("\nprograme server: \n client_port %d, control_port %d, h264_port %d, window_flag %d, window_size %d,",
                 client_port, control_port, h264_port, window_flag, window_size);
    //call_back = call;
	
	run_flag = 1;
	return init_server();
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
	DEBUG("StopMonitorServer end");
	char s = 'S';
	send_msg(pipe_event[1], &s, 1);
	run_flag = 0;
    DEBUG("recv sig stop programe !!");
	exit_server();
	DEBUG("exit server ok");
	call_back = NULL;
	hwnd = NULL;
	//DEBUG("close logs ok");
	close_logs();
	return SUCCESS;
}

CAPTUREANDCAST_API int DisconnectAllClient()
{
	DEBUG("DisconnectAllClient");
	return SUCCESS;		
}

CAPTUREANDCAST_API int ExitControl()
{
	switch_mode(0);
	DEBUG("ExitControl");
	sleep(1);
	return SUCCESS;
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
	init_logs();

	server_ip = strdup(serverIp);	
	server_port = serverPort;

	DEBUG("\nprograme client: \n server_ip %s, server_port %d, window_flag %d, default_fps %d",
               server_ip, server_port, window_flag, default_fps);
	return init_client();
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
CAPTUREANDCAST_API int SetPageAttribute(const int pageIndex, const int pageSize, struct StudentInfo *info, const int length)
{
	DEBUG("SetPageAttribute pageIndex: %d pageSize: %d length %d", pageIndex, pageSize, length);
	stop_display();
	int i = 0;
#if 0
	for(i = 0; i < length; i++)
	{
		DEBUG("i: %d no: %d ip: %s name: %s", i, info[i].No, info[i].Name, info[i].Ip);
	}
#endif
    return SUCCESS;
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
	return SUCCESS;
}


void stop_server()
{
#if 0
	if(call_back)
		call_back();
	call_back = NULL;
#endif
}
#endif	//_WIN32
