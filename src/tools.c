#include <limits.h>
#include <ctype.h>
#ifndef _WIN32
	#include <sys/un.h>
	#include <linux/rtc.h>
	#include <linux/types.h>
	#include <linux/netlink.h>

#define TIMEZONE_OFFSET(foo) foo->tm_gmtoff 
#endif
#include <assert.h>

#include "base.h"


const char month_tab[48] =
    "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec ";
const char day_tab[] = "Sun,Mon,Tue,Wed,Thu,Fri,Sat,";

int use_localtime = 1;   //夏时令

char network_ip[10][128] = {0};
int interface_num = 0;

/*
 * Name: get_commonlog_time
 *
 * Description: Returns the current time in common log format in a static
 * char buffer.
 *
 * commonlog time is exactly 25 characters long
 * because this is only used in logging, we add " [" before and "] " after
 * making 29 characters
 * "[27/Feb/1998:20:20:04 +0000] "
 *
 * Constrast with rfc822 time:
 * "Sun, 06 Nov 1994 08:49:37 GMT"
 *
 * Altered 10 Jan 2000 by Jon Nelson ala Drew Streib for non UTC logging
 *
 */
char *get_commonlog_time(void)
{
    struct tm *t; 
    char *p; 
    unsigned int a;
    static char buf[30];
    int time_offset;

    (void) time(&current_time);

    if (use_localtime) {
        t = localtime(&current_time);
#ifndef _WIN32
        time_offset = TIMEZONE_OFFSET(t);
#endif
    } else {
        t = gmtime(&current_time);
        time_offset = 0;
    }   

    p = buf + 29; 
    *p-- = '\0';
    *p-- = ' ';
    *p-- = ']';
    a = abs(time_offset / 60);
    *p-- = '0' + a % 10; 
    a /= 10; 
    *p-- = '0' + a % 6;
    a /= 6;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p-- = (time_offset >= 0) ? '+' : '-';
    *p-- = ' ';

    a = t->tm_sec;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p-- = ':';
    a = t->tm_min;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p-- = ':';
    a = t->tm_hour;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p-- = ':';
    a = 1900 + t->tm_year;
    while (a) {
        *p-- = '0' + a % 10;
        a /= 10;
    }
    /* p points to an unused spot */
    *p-- = '/';
    p -= 2;
    memcpy(p--, month_tab + 4 * (t->tm_mon), 3);
    *p-- = '/';
    a = t->tm_mday;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p = '[';
    return p;                   /* should be same as returning buf */
}

/* Return the UNIX time in microsends */
long long ustime(void )
{
	struct timeval tv;
	long long ust;

	gettimeofday(&tv, NULL);
	ust = ((long long)tv.tv_sec)*1000000;
	ust += tv.tv_usec;
	return ust;
}

long long mstime(void)
{
	return ustime()/1000;
}



/* 删除两边的空格*/
char *trim(char *output, const char *input)
{
	char *p = NULL;
	//assert(output != NULL);
	//assert(input != NULL);

	while((*input != '\0' && isspace(*input)))
	{
		input++;
	}
	
	while((*input != '\0' && (!isspace(*input))))
	{
		*output = *input;
		output++;
		input++;
	}
	output = '\0';
}

/* usb 热插拔事件 fd */
int get_hotplug_sock(void)
{
	int fd;
#ifndef _WIN32
	struct sockaddr_nl snl = {0};
	int sock_opt , ret ;
	
	snl.nl_family = AF_NETLINK;
	snl.nl_pid = getpid();
	snl.nl_groups = 1;		// receive broadcast message
	
	fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if(fd < 0)
	{
		DEBUG("socket err %s", strerror(errno));
		return -1;
	}	

	sock_opt = 512 * 1024;//设置为512K
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&sock_opt, sizeof (sock_opt)) == -1) 
    {   
        DEBUG(" set SO_RCVBUF size %d fail %s!", sock_opt, strerror(errno));
    }  

	ret = bind(fd, (struct sockaddr *)&snl, sizeof(struct sockaddr_nl));
	if(ret < 0)
	{
        DEBUG(" bind fd %d errno %s!", fd, strerror(errno));
		close(fd);
		return -1;
	}
#endif
	return fd;
}

void get_usb_info(int usb_number)
{
	int ret;
	char path[50] = {0};
	char buf[1024] = {0};	
	sprintf(path, "/proc/scsi/usb-storage/%d", usb_number);
	
	DEBUG("path %s", path);
	FILE *fp = fopen(path, "r");
	if(fp)
	{
		ret = fread(buf, sizeof(buf),1,fp);
		fclose(fp);
		DEBUG("usb info\n%s", buf);
	}	
}


int remove_usb(char *buf)
{
	char path[50] = {0};
	sprintf(path, "eject /dev%s", buf);
	
	//执行两次  保证移动设备成功弹出
    system(path);
    system(path);
	return 0;
}



int get_network_info(void)
{
#ifndef _WIN32
	int fd = -1;
    int interface_num = 0;
    struct ifreq buf[16] = {0};
    struct ifreq ifrcopy;
    struct ifconf ifc;
    
    char mac[16] = {0};
    char ip[32] = {0};
    char boardcast_addr[32] = {0};
    char netmask[32] = {0};

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        DEBUG("socket %s err", strerror(errno));
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
    {
		interface_num = ifc.ifc_len / sizeof(struct ifreq);
		DEBUG("interface num = %d", interface_num);
		while(interface_num-- > 0)
		{

			//struct net_info *p_info = (struct net_info *)malloc(sizeof(struct net_info));
			DEBUG("device name %s", buf[interface_num].ifr_name);
			
			ifrcopy = buf[interface_num];
			if(ioctl(fd, SIOCGIFFLAGS, &ifrcopy))
			{
				DEBUG("ioctl %s err", strerror(errno));
				close(fd);
				return -1;
			}
			/* get mac of this interface */
			if(!ioctl(fd, SIOCGIFHWADDR, (char *)(&buf[interface_num])))	
			{
				memset(mac, 0, sizeof(mac));		
				snprintf(mac, sizeof(mac), "%02x%02x%02x%02x%02x%02x",
                    (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[0],
                    (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[1],
                    (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[2],

                    (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[3],
                    (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[4],
                    (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[5]);

				DEBUG("device mac %s", mac);
				//memcpy(p_info->mac, mac, sizeof(p_info->mac));
			}
			else	
			{

				DEBUG("ioctl %s err", strerror(errno));
				close(fd);
				return -1;
			}
			/* get the ip of this interface */
			if(!ioctl(fd, SIOCGIFADDR, (char *)&buf[interface_num]))
			{
				//memset(ip, 0, sizeof(ip);
				snprintf(ip, sizeof(ip), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interface_num].ifr_addr))->sin_addr));
				DEBUG("device ip %s", ip);
				memcpy(network_ip[interface_num], ip, sizeof(ip));
				interface_num++;
				//memcpy(p_info->ip, ip, sizeof(p_info->ip));
			}
			else
			{
				DEBUG("ioctl %s err", strerror(errno));
				close(fd);
				return -1;
			}
			/* get the boardcast_addr of the interface */
			if(!ioctl(fd, SIOCGIFBRDADDR, &buf[interface_num]))
			{
				memset(boardcast_addr, 0, sizeof(boardcast_addr));
				snprintf(boardcast_addr, sizeof(boardcast_addr), "%s",
					(char *)inet_ntoa(((struct sockaddr_in *)&(buf[interface_num].ifr_broadaddr))->sin_addr));
				DEBUG("device boardcast_addr %s", boardcast_addr);
				//memcpy(p_info->boardcast_addr, boardcast_addr, sizeof(p_info->boardcast_addr));
			}
			else
			{
				DEBUG("ioctl %s err", strerror(errno));
				close(fd);
				return -1;
			}

			/* get the netmask of the interface */
			if(!ioctl(fd, SIOCGIFNETMASK, &buf[interface_num]))
			{
				memset(netmask, 0, sizeof(netmask));
				snprintf(netmask, sizeof(netmask), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interface_num].ifr_netmask))->sin_addr));
				DEBUG("device netmask %s", netmask);
				//memcpy(p_info->netmask, netmask, sizeof(p_info->netmask));
			}
			else
			{
				DEBUG("ioctl %s err", strerror(errno));
				close(fd);
				return -1;
			}
			//network_list.push_back(*p_info);
			//p_info = NULL;
		}
	}
	else
	{
		DEBUG("ioctl %s", strerror(errno));
		close(fd);
		return -1;
	}
	close(fd);
#endif
	return 0;
}


/* Convert a string representing an amount of memory into the number of
 * bytes, so for instance memtoll("1Gi") will return 1073741824 that is
 * (1024*1024*1024).
 *
 * On parsing error, if *err is not NULL, it's set to 1, otherwise it's
 * set to 0 */
long long memtoll(const char *p, int *err)
{
	const char *u;
	char buf[128];
	long mul;		//unit multiplier
	long long val;
	unsigned int digits;

	if(err) *err = 0;
	/* Search the first non digit charachter */
	u = p;
	if(*u == '-') u++;
	while(*u && isdigit(*u)) u++;		//检测isdigit 十进制
	if(*u == '\0' || !strcasecmp(u,"b")) 
	{
		mul = 1;
	}		
	else if(!strcasecmp(u,"k"))
	{
		mul = 1000;
	}
	else if(!strcasecmp(u,"kb"))
	{
		mul = 1024;
	}
	else if(!strcasecmp(u,"m"))
	{
		mul = 1000 * 1000;
	}
	else if(!strcasecmp(u,"mb"))
	{
		mul = 1024 * 1024;
	}
	else if(!strcasecmp(u,"g"))
	{
		mul = 1000L * 1000 * 1000;
	}
	else if(!strcasecmp(u,"gb"))
	{
		mul = 1024L * 1024 * 1024;
	}
	else
	{
		if(err) *err = 1;
		mul = 1;
	}
	digits = u - p;
	if(digits >= sizeof(buf))
	{
		if(err) *err = 1;
		return LLONG_MAX;
	}
	memcpy(buf, p, digits);
	buf[digits] = '\0';
	val = strtoll(buf, NULL, 10);
	DEBUG("val %d", val);
	return val*mul;
}

/* 随机一个32位 */
unsigned long ulrand(void) {
	srand((unsigned)time(NULL)^getpid());
    return (
     (((unsigned long)rand()<<24)&0xFF000000ul)
    |(((unsigned long)rand()<<12)&0x00FFF000ul)
    |(((unsigned long)rand()    )&0x00000FFFul));
}
/* 随机一个64位 */
unsigned long long ullrand(void) {
    return (
     (((unsigned long long)ulrand())<<32)
    | ((unsigned long long)ulrand()));
}


int enc_get_utf8_size(const unsigned char pInput)
{
   unsigned char c = pInput;
   // 0xxxxxxx 返回0
   // 10xxxxxx 不存在
   // 110xxxxx 返回2
   // 1110xxxx 返回3
   // 11110xxx 返回4
   // 111110xx 返回5
    // 1111110x 返回6
    if(c< 0x80) return 0;
    if(c>=0x80 && c<0xC0) return -1; 
    if(c>=0xC0 && c<0xE0) return 2;
    if(c>=0xE0 && c<0xF0) return 3;
    if(c>=0xF0 && c<0xF8) return 4;
    if(c>=0xF8 && c<0xFC) return 5;
    if(c>=0xFC) return 6;
}


/***************************************************************************** 
 * 将一个字符的UTF8编码转换成Unicode(UCS-2和UCS-4)编码. 
 * 
 * 参数: 
 *    pInput      指向输入缓冲区, 以UTF-8编码 
 *    Unic        指向输出缓冲区, 其保存的数据即是Unicode编码值, 
 *                类型为unsigned long . 
 * 
 * 返回值: 
 *    成功则返回该字符的UTF8编码所占用的字节数; 失败则返回0. 
 * 
 * 注意: 
 *     1. UTF8没有字节序问题, 但是Unicode有字节序要求; 
 *        字节序分为大端(Big Endian)和小端(Little Endian)两种; 
 *        在Intel处理器中采用小端法表示, 在此采用小端法表示. (低地址存低位) 
 ****************************************************************************/  
int enc_utf8_to_unicode_one(const unsigned char* pInput, unsigned long *Unic)  
{  
    assert(pInput != NULL && Unic != NULL);  

    // b1 表示UTF-8编码的pInput中的高字节, b2 表示次高字节, ...  
    char b1, b2, b3, b4, b5, b6;  

    *Unic = 0x0; // 把 *Unic 初始化为全零  
    int utfbytes = enc_get_utf8_size(*pInput);  
    unsigned char *pOutput = (unsigned char *) Unic;  

    switch ( utfbytes )  
    {  
        case 0:  
            *pOutput     = *pInput;  
            utfbytes    += 1;  
            break;  
        case 2:  
            b1 = *pInput;  
            b2 = *(pInput + 1);  
            if ( (b2 & 0xE0) != 0x80 )  
                return 0;  
            *pOutput     = (b1 << 6) + (b2 & 0x3F);  
            *(pOutput+1) = (b1 >> 2) & 0x07;  
            break;  
        case 3:  
            b1 = *pInput;  
            b2 = *(pInput + 1);  
            b3 = *(pInput + 2);  
            if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) )  
                return 0;  
            *pOutput     = (b2 << 6) + (b3 & 0x3F);  
            *(pOutput+1) = (b1 << 4) + ((b2 >> 2) & 0x0F);  
            break;  
        case 4:  
            b1 = *pInput;  
            b2 = *(pInput + 1);  
            b3 = *(pInput + 2);  
            b4 = *(pInput + 3);  
            if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)  
                    || ((b4 & 0xC0) != 0x80) )  
                return 0;  
            *pOutput     = (b3 << 6) + (b4 & 0x3F);  
            *(pOutput+1) = (b2 << 4) + ((b3 >> 2) & 0x0F);  
            *(pOutput+2) = ((b1 << 2) & 0x1C)  + ((b2 >> 4) & 0x03);  
            break;  
        case 5:  
            b1 = *pInput;  
            b2 = *(pInput + 1);  
            b3 = *(pInput + 2);  
            b4 = *(pInput + 3);  
            b5 = *(pInput + 4);  
            if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)  
                    || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80) )  
                return 0;  
            *pOutput     = (b4 << 6) + (b5 & 0x3F);  
            *(pOutput+1) = (b3 << 4) + ((b4 >> 2) & 0x0F);  
            *(pOutput+2) = (b2 << 2) + ((b3 >> 4) & 0x03);  
            *(pOutput+3) = (b1 << 6);  
            break;  
        case 6:  
            b1 = *pInput;  
            b2 = *(pInput + 1);  
            b3 = *(pInput + 2);  
            b4 = *(pInput + 3);  
            b5 = *(pInput + 4);  
            b6 = *(pInput + 5);  
            if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)  
                    || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80)  
                    || ((b6 & 0xC0) != 0x80) )  
                return 0;  
            *pOutput     = (b5 << 6) + (b6 & 0x3F);  
            *(pOutput+1) = (b5 << 4) + ((b6 >> 2) & 0x0F);  
            *(pOutput+2) = (b3 << 2) + ((b4 >> 4) & 0x03);  
            *(pOutput+3) = ((b1 << 6) & 0x40) + (b2 & 0x3F);  
            break;  
        default:  
            return 0;  
            break;  
    }  

    return utfbytes;  
}  


/***************************************************************************** 
 * 将一个字符的Unicode(UCS-2和UCS-4)编码转换成UTF-8编码. 
 * 
 * 参数: 
 *    unic     字符的Unicode编码值 
 *    pOutput  指向输出的用于存储UTF8编码值的缓冲区的指针 
 *    outsize  pOutput缓冲的大小 
 * 
 * 返回值: 
 *    返回转换后的字符的UTF8编码所占的字节数, 如果出错则返回 0 . 
 * 
 * 注意: 
 *     1. UTF8没有字节序问题, 但是Unicode有字节序要求; 
 *        字节序分为大端(Big Endian)和小端(Little Endian)两种; 
 *        在Intel处理器中采用小端法表示, 在此采用小端法表示. (低地址存低位) 
 *     2. 请保证 pOutput 缓冲区有最少有 6 字节的空间大小! 
 ****************************************************************************/  
int enc_unicode_to_utf8_one(unsigned long unic, unsigned char *pOutput,  
        int outSize)  
{  
    assert(pOutput != NULL);  
    assert(outSize >= 6);  
  
    if ( unic <= 0x0000007F )  
    {  
        // * U-00000000 - U-0000007F:  0xxxxxxx  
        *pOutput     = (unic & 0x7F);  
        return 1;  
    }  
    else if ( unic >= 0x00000080 && unic <= 0x000007FF )  
    {  
        // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx  
        *(pOutput+1) = (unic & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 6) & 0x1F) | 0xC0;  
        return 2;  
    }  
    else if ( unic >= 0x00000800 && unic <= 0x0000FFFF )  
    {  
        // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx  
        *(pOutput+2) = (unic & 0x3F) | 0x80;  
        *(pOutput+1) = ((unic >>  6) & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 12) & 0x0F) | 0xE0;  
        return 3;  
    }  
    else if ( unic >= 0x00010000 && unic <= 0x001FFFFF )  
    {  
        // * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  
        *(pOutput+3) = (unic & 0x3F) | 0x80;  
        *(pOutput+2) = ((unic >>  6) & 0x3F) | 0x80;  
        *(pOutput+1) = ((unic >> 12) & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 18) & 0x07) | 0xF0;  
        return 4;  
    }  
    else if ( unic >= 0x00200000 && unic <= 0x03FFFFFF )  
    {  
        // * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
        *(pOutput+4) = (unic & 0x3F) | 0x80;  
        *(pOutput+3) = ((unic >>  6) & 0x3F) | 0x80;  
        *(pOutput+2) = ((unic >> 12) & 0x3F) | 0x80;  
        *(pOutput+1) = ((unic >> 18) & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 24) & 0x03) | 0xF8;  
        return 5;  
    }  
    else if ( unic >= 0x04000000 && unic <= 0x7FFFFFFF )  
    {  
        // * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
        *(pOutput+5) = (unic & 0x3F) | 0x80;  
        *(pOutput+4) = ((unic >>  6) & 0x3F) | 0x80;  
        *(pOutput+3) = ((unic >> 12) & 0x3F) | 0x80;  
        *(pOutput+2) = ((unic >> 18) & 0x3F) | 0x80;  
        *(pOutput+1) = ((unic >> 24) & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 30) & 0x01) | 0xFC;  
        return 6;  
    }  
  
    return 0;  
}  

char *unicode_to_utf8(unsigned int *unic, char *pOutput)                             
{ 
    while(*unic)
    {
        if ( (*unic) <= 0x0000007F ) 
        { 
            // * U-00000000 - U-0000007F:  0xxxxxxx 
            *(pOutput++)  = ((*unic) & 0x7F); 
        } 
        else if ( ((*unic) >= 0x00000080) && ((*unic) <= 0x000007FF) ) 
        { 
            // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx 
            *(pOutput++) = (((*unic) >> 6) & 0x1F) | 0xC0; 
            *(pOutput++) = ((*unic) & 0x3F) | 0x80; 
        } 
        else if ( ((*unic) >= 0x00000800) && ((*unic) <= 0x0000FFFF) ) 
        { 
            // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx 
            *(pOutput++) = (((*unic) >> 12) & 0x0F) | 0xE0; 
            *(pOutput++) = (((*unic) >>  6) & 0x3F) | 0x80;
            *(pOutput++) = ((*unic) & 0x3F) | 0x80; 
        } 
        else if ( ((*unic) >= 0x00010000) && ((*unic) <= 0x001FFFFF) ) 
        { 
            // * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx 
            *(pOutput++) = (((*unic) >> 18) & 0x07) | 0xF0;
            *(pOutput++) = (((*unic) >> 12) & 0x3F) | 0x80;
            *(pOutput++) = (((*unic) >>  6) & 0x3F) | 0x80;
            *(pOutput++) = ((*unic) & 0x3F) | 0x80; 
        } 
        else if ( ((*unic) >= 0x00200000) && ((*unic) <= 0x03FFFFFF) ) 
        { 
            // * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 
            *(pOutput++) = (((*unic) >> 24) & 0x03) | 0xF8;
            *(pOutput++) = (((*unic) >> 18) & 0x3F) | 0x80;
            *(pOutput++) = (((*unic) >> 12) & 0x3F) | 0x80; 
            *(pOutput++) = (((*unic) >>  6) & 0x3F) | 0x80;
            *(pOutput++) = ((*unic) & 0x3F) | 0x80; 
        } 
        else if ( ((*unic) >= 0x04000000) && ((*unic) <= 0x7FFFFFFF) ) 
        { 
            // * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 
            *(pOutput++) = (((*unic) >> 30) & 0x01) | 0xFC;
            *(pOutput++) = (((*unic) >> 24) & 0x3F) | 0x80;
            *(pOutput++) = (((*unic) >> 18) & 0x3F) | 0x80;
            *(pOutput++) = (((*unic) >> 12) & 0x3F) | 0x80; 
            *(pOutput++) = (((*unic) >>  6) & 0x3F) | 0x80;
            *(pOutput++) = ((*unic) & 0x3F) | 0x80; 
        }
        unic++;
    }
    *pOutput = 0x0000;
    return pOutput; 
}


