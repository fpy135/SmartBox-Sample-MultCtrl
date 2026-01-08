#include "lwip/debug.h"
#include "httpd.h"
#include "lwip/tcp.h"
#include "fs.h"
//#include "lwip_comm.h"
//#include "led.h"
//#include "pcf8574.h"
//#include "adc.h"
//#include "rtc.h"
//#include "lcd.h"
#include <string.h>
#include <stdlib.h>
#include "Led_App.h"
#include "usart.h"
#include "MyFlash.h"
#include "mystring.h"
#include "StorageConfig.h"
#include "socketserver.h"
#include "lwip.h"
#include "rtc.h"
#include "Unixtimer.h"
#include "Environment_data.h"
#include "IDConfig.h"
#include "SysConfig.h"
#include "BL0942.h"

extern void GetRTC(rtc_time_t *rtc_time);
extern void SetRTC(rtc_time_t *rtc_time);
extern void Get_ElectricData(uint8_t channle, CollectionData *electricdata);

#define NUM_CONFIG_CGI_URIS 11  //CGI的URI数量

#if LWIP_HTTPD_SSI_MULTIPART == 1
#define NUM_CONFIG_SSI_TAGS	11  //SSI的TAG数量
#endif

//控制LED和BEEP的CGI handler
const char* LOGIN_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* PASSWORDSET_CGI_Handler(int iIndex,int iNumParams,char *pcParam[],char *pcValue[]);
const char* WEBSET_CGI_Handler(int iIndex,int iNumParams,char *pcParam[],char *pcValue[]);
const char* RTCTIME_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* NETPARA_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* Env_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* TimeCtrl_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* TimeCtrlSet_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* LedCtrl_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* LedSta_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* SensorCtrl_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

#if LWIP_HTTPD_SSI_MULTIPART == 1
static const char *ppcTAGs[]=  //SSI的Tag
{
	"b",	//设备ID
	"c",	//设备类型
	"s",  	//静态ip
	"m",	//子网掩码
	"g",	//网关
	"i",	//server ip
	"p",	//server port
	"d",	//dhcp
	"t",	//rtc time
	"n",	//ntp服务器
	"a"		//环境数据
};
#endif

static const tCGI ppcURLs[]= //cgi程序
{
	{"/login.cgi",LOGIN_CGI_Handler},
	{"/passwordset.cgi",PASSWORDSET_CGI_Handler},
	{"/webset.cgi",WEBSET_CGI_Handler},
	{"/rtctime.cgi",RTCTIME_CGI_Handler},
	{"/netpara.cgi",NETPARA_CGI_Handler},
	{"/env.cgi",Env_CGI_Handler},
	{"/timectrl.cgi",TimeCtrl_CGI_Handler},
	{"/timectrlset.cgi",TimeCtrlSet_CGI_Handler},
	{"/ledctrl.cgi",LedCtrl_CGI_Handler},
	{"/ledsta.cgi",LedSta_CGI_Handler},
	{"/sensorctrl.cgi",SensorCtrl_CGI_Handler},
};

uint8_t data_response_buf[512]={0};


//当web客户端请求浏览器的时候,使用此函数被CGI handler调用
static int FindCGIParameter(const char *pcToFind,char *pcParam[],int iNumParams)
{
	int iLoop;
	for(iLoop = 0;iLoop < iNumParams;iLoop ++ )
	{
		if(strcmp(pcToFind,pcParam[iLoop]) == 0)
		{
			return (iLoop); //返回iLOOP
		}
	}
	return (-1);
}

#if LWIP_HTTPD_SSI_MULTIPART == 1

void DeviceID_Handler(char *pcInsert)
{
	sprintf(pcInsert,"%d",Device_ID);
}

void DeviceType_Handler(char *pcInsert)
{
	sprintf(pcInsert,"%x",Device_TYPE);
}
void DeviceIP_Handler(char *pcInsert)
{
	uint32_t deviceip;
	deviceip = gnetif.ip_addr.addr;
	sprintf(pcInsert,"%d.%d.%d.%d",(uint8_t)(deviceip&0xff),(uint8_t)((deviceip>>8)&0xff),(uint8_t)((deviceip>>16)&0xff),(uint8_t)((deviceip>>24)&0xff));
}

void DeviceNetmask_Handler(char *pcInsert)
{
	uint32_t deviceip;
	deviceip = gnetif.netmask.addr;
	sprintf(pcInsert,"%d.%d.%d.%d",(uint8_t)(deviceip&0xff),(uint8_t)((deviceip>>8)&0xff),(uint8_t)((deviceip>>16)&0xff),(uint8_t)((deviceip>>24)&0xff));
}

void DeviceGw_Handler(char *pcInsert)
{
	uint32_t deviceip;
	deviceip = gnetif.gw.addr;
	sprintf(pcInsert,"%d.%d.%d.%d",(uint8_t)(deviceip&0xff),(uint8_t)((deviceip>>8)&0xff),(uint8_t)((deviceip>>16)&0xff),(uint8_t)((deviceip>>24)&0xff));
}

void ServerIP_Handler(char *pcInsert)
{
	sprintf(pcInsert,"%d.%d.%d.%d",(uint8_t)(TCPServerIP&0xff),(uint8_t)((TCPServerIP>>8)&0xff),(uint8_t)((TCPServerIP>>16)&0xff),(uint8_t)((TCPServerIP>>24)&0xff));
}

void ServerPort_Handler(char *pcInsert)
{
	uint16_t port;
	port = HALFWORD_Reverse(TCPServerPort);
	sprintf(pcInsert,"%d",port);
}

void ServerDHCP_Handler(char *pcInsert)
{
	if(DHCP_Enable_Flag == 0x01)
		sprintf(pcInsert,"ON");
	else
		sprintf(pcInsert,"OFF");
}

void RTCTime_Handler(char *pcInsert)
{
	rtc_time_t rtc_time;
	GetRTC(&rtc_time);

	sprintf(pcInsert,"%02d-%02d-%02d  %02d:%02d:%02d",rtc_time.ui8Year, rtc_time.ui8Month,\
	rtc_time.ui8DayOfMonth, rtc_time.ui8Hour, rtc_time.ui8Minute, rtc_time.ui8Second);
}

void NTP_Handler(char *pcInsert)
{
	sprintf(pcInsert,"%d.%d.%d.%d",(uint8_t)(NTPServerIP&0xff),(uint8_t)((NTPServerIP>>8)&0xff),(uint8_t)((NTPServerIP>>16)&0xff),(uint8_t)((NTPServerIP>>24)&0xff));
}

void Environmental_Handler(char *pcInsert)
{
	Environment_Type environment_data;
		
	Environment_Data_RW(0,&environment_data);
	sprintf(pcInsert,"%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f",environment_data.temperature/100.0,environment_data.humidity/100.0,environment_data.pm2_5/1.0,\
			environment_data.pm10/1.0,environment_data.air_pressure/10.0,environment_data.wind_speed/100.0,environment_data.wind_direction/100.0,environment_data.noise/10.0);
}
//SSI的Handler句柄
static u16_t SSIHandler(int iIndex,char *pcInsert,int iInsertLen)
{
	switch(iIndex)
	{
		case 0: 
				DeviceID_Handler(pcInsert);
				break;
		case 1:
				DeviceType_Handler(pcInsert);
				break;
		case 2: 
				DeviceIP_Handler(pcInsert);
				break;
		case 3:
				DeviceNetmask_Handler(pcInsert);
				break;
		case 4:
				DeviceGw_Handler(pcInsert);
				break;
		case 5:
				ServerIP_Handler(pcInsert);
				break;
		case 6:
				ServerPort_Handler(pcInsert);
				break;
		case 7:
				ServerDHCP_Handler(pcInsert);
				break;
		case 8:
				RTCTime_Handler(pcInsert);
				break;
		case 9:
				NTP_Handler(pcInsert);
				break;
		case 10:
				Environmental_Handler(pcInsert);
				break;
	}
	return strlen(pcInsert);
}
#endif

//rtctime的CGI控制句柄
const char* NETPARA_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	uint32_t deviceip;
	uint32_t netmask;
	uint32_t gateway;
	uint32_t len = 0;
	uint16_t port;

	deviceip = gnetif.ip_addr.addr;
	port = HALFWORD_Reverse(TCPServerPort);
	netmask = gnetif.netmask.addr;
	gateway = gnetif.gw.addr;

	memset(data_response_buf,0,strlen((char *)data_response_buf));
	len += sprintf((char *)&data_response_buf[0],"%d.%d.%d",(SOFTWARE_VERSION&0xf000)>>12,\
		(SOFTWARE_VERSION&0x0f00)>>8, (SOFTWARE_VERSION&0x00ff));
	data_response_buf[len] = ';';
	len++;
	len += sprintf((char *)&data_response_buf[len],"%d.%d.%d",(HARDWARE_VERSION&0xf000)>>12,\
		(HARDWARE_VERSION&0x0f00)>>8, (HARDWARE_VERSION&0x00ff));
	data_response_buf[len] = ';';
	len++;
	len += sprintf((char *)&data_response_buf[len],"%08X:%d",Device_ID, Device_ID);
	data_response_buf[len] = ';';
	len++;
	len += sprintf((char *)&data_response_buf[len],"%X",Device_TYPE);
	data_response_buf[len] = ';';
	len++;
	len += sprintf((char *)&data_response_buf[len],"%d.%d.%d.%d",(uint8_t)(deviceip&0xff),\
		(uint8_t)((deviceip>>8)&0xff),(uint8_t)((deviceip>>16)&0xff),(uint8_t)((deviceip>>24)&0xff));
	data_response_buf[len] = ';';
	len++;
	len += sprintf((char *)&data_response_buf[len],"%d.%d.%d.%d",(uint8_t)(netmask&0xff),\
		(uint8_t)((netmask>>8)&0xff),(uint8_t)((netmask>>16)&0xff),(uint8_t)((netmask>>24)&0xff));
	data_response_buf[len] = ';';
	len++;
	len += sprintf((char *)&data_response_buf[len],"%d.%d.%d.%d",(uint8_t)(gateway&0xff),\
		(uint8_t)((gateway>>8)&0xff),(uint8_t)((gateway>>16)&0xff),(uint8_t)((gateway>>24)&0xff));
	data_response_buf[len] = ';';
	len++;
	len += sprintf((char *)&data_response_buf[len],"%d.%d.%d.%d",(uint8_t)(TCPServerIP&0xff),\
		(uint8_t)((TCPServerIP>>8)&0xff),(uint8_t)((TCPServerIP>>16)&0xff),(uint8_t)((TCPServerIP>>24)&0xff));
	data_response_buf[len] = ';';
	len++;
	len += sprintf((char *)&data_response_buf[len],"%d",port);
	data_response_buf[len] = ';';
	len++;
	if(DHCP_Enable_Flag == 0x01)
		len += sprintf((char *)&data_response_buf[len],"ON");
	else
		len += sprintf((char *)&data_response_buf[len],"OFF");
	data_response_buf[len] = ';';
	len++;
	len += sprintf((char *)&data_response_buf[len],"%d.%d.%d.%d",(uint8_t)(NTPServerIP&0xff),\
		(uint8_t)((NTPServerIP>>8)&0xff),(uint8_t)((NTPServerIP>>16)&0xff),(uint8_t)((NTPServerIP>>24)&0xff));
	data_response_buf[len] = ';';
	len++;
}

//rtctime的CGI控制句柄
const char* RTCTIME_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	rtc_time_t rtc_time;
	GetRTC(&rtc_time);

	memset(data_response_buf,0,strlen((const char *)data_response_buf));
	sprintf((char *)data_response_buf,"%02d-%02d-%02d  %02d:%02d:%02d",rtc_time.ui8Year, rtc_time.ui8Month,\
	rtc_time.ui8DayOfMonth, rtc_time.ui8Hour, rtc_time.ui8Minute, rtc_time.ui8Second);

	
}
//login的CGI控制句柄
const char* LOGIN_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	iIndex = FindCGIParameter("username",pcParam,iNumParams);  //找到name的索引号
	if(iIndex != -1) //name
	{
		if(strcmp(pcValue[iIndex],login_info.username) == 0)  //用户名
		{
			iIndex = FindCGIParameter("password",pcParam,iNumParams);  //找到password的索引号
			if(iIndex != -1) //name
			{
				if(strcmp(pcValue[iIndex],login_info.password) == 0)  //用户名
				{
					return "/home.html";
				}
			}
		}
	}
	return "/index.html";
}
//password的CGI控制句柄
const char* PASSWORDSET_CGI_Handler(int iIndex,int iNumParams,char *pcParam[],char *pcValue[])
{
	iIndex = FindCGIParameter("date",pcParam,iNumParams);  //设置日期时间···
	if(iIndex != -1) //date
	{
		uint16_t p,q,r;
		uint16_t str_len;
		uint32_t num;
		uint32_t ip = 0;
		rtc_time_t rtc_time;
		if(strlen(pcValue[iIndex])==0 || strlen(pcValue[iIndex]) != 10)
			return "/set.html";
		str_len = strlen(pcValue[iIndex]);
		p = StrFindString((uint8_t *)pcValue[iIndex],str_len, (uint8_t *)"-", 1);
		if(p == 0xffff)
			return "/set.html";
		Str2Num((uint8_t *)pcValue[iIndex],p,(uint32_t *)&num);
		rtc_time.ui8Year = num;
		
		q = StrFindString((uint8_t *)(pcValue[iIndex]+p+1),str_len-p-1, (uint8_t *)"-", 1);
		if(q == 0xffff)
			return "/set.html";
		Str2Num((uint8_t *)(pcValue[iIndex]+p+1),q,&num);
		rtc_time.ui8Month = num;
		
		if(str_len-p-q-2 <= 0)
			return "/set.html";
		Str2Num((uint8_t *)(pcValue[iIndex]+p+q+2),str_len-p-q-2,&num);
		rtc_time.ui8DayOfMonth = num;
		
		iIndex = FindCGIParameter("time",pcParam,iNumParams);  //时间
		if(iIndex != -1) //time
		{
			if(strlen(pcValue[iIndex])==0 || strlen(pcValue[iIndex]) != 12)
				return "/set.html";
			str_len = strlen(pcValue[iIndex]);
			p = StrFindString((uint8_t *)pcValue[iIndex],str_len, (uint8_t *)"%3A", 3);
			if(p == 0xffff)
				return "/set.html";
			Str2Num((uint8_t *)pcValue[iIndex],p,(uint32_t *)&num);
			rtc_time.ui8Hour = num;
			
			q = StrFindString((uint8_t *)(pcValue[iIndex]+p+3),str_len-p-3, (uint8_t *)"%3A", 3);
			if(q == 0xffff)
				return "/set.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+3),q,&num);
			rtc_time.ui8Minute = num;
			
			if(str_len-p-q-6 <= 0)
				return "/set.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+q+6),str_len-p-q-6,&num);
			rtc_time.ui8Second = num;
			SetRTC(&rtc_time);
			return "/home.html";
		}
	}
	iIndex = FindCGIParameter("username",pcParam,iNumParams);  //设置用户名密码
	if(iIndex != -1) //username
	{
		if(strlen(pcValue[iIndex])>sizeof(login_info.username)-1 || strlen(pcValue[iIndex])==0)
			return "/set.html";
		strcpy(login_info.username ,pcValue[iIndex]);
		login_info.username[strlen(pcValue[iIndex])] = '\0';
		iIndex = FindCGIParameter("password",pcParam,iNumParams);  //找到password的索引号
		if(iIndex != -1) //password
		{
			if(strlen(pcValue[iIndex])>sizeof(login_info.password)-1 || strlen(pcValue[iIndex])==0)
				return "/set.html";
			strcpy(login_info.password ,pcValue[iIndex]);
			login_info.password[strlen(pcValue[iIndex])] = '\0';
			Write_UserMem_Flash (WebUsername_ADDR, (uint8_t *)login_info.username, sizeof(login_info.username));
			Write_UserMem_Flash (WebPassword_ADDR, (uint8_t *)login_info.password, sizeof(login_info.password));
			return "/index.html";
		}
	}
	
	return "/set.html";
}
//webset的CGI控制句柄
const char *WEBSET_CGI_Handler(int iIndex,int iNumParams,char *pcParam[],char *pcValue[])
{
	iIndex = FindCGIParameter("btn_set",pcParam,iNumParams);  //设置网络参数按钮
	if(iIndex != -1)
	{
		iIndex = FindCGIParameter("setdevip",pcParam,iNumParams);  //设备IP
		if(iIndex != -1)
		{
			uint16_t p,q,r;
			uint16_t str_len;
			uint32_t num;
			uint32_t ip = 0;
			
			str_len = strlen(pcValue[iIndex]);
			p = StrFindString((uint8_t *)pcValue[iIndex],str_len, (uint8_t *)".", 1);
			if(p == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)pcValue[iIndex],p,&num);
			ip = num;
			q = StrFindString((uint8_t *)(pcValue[iIndex]+p+1),str_len-p-1, (uint8_t *)".", 1);
			if(q == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+1),q,&num);
			ip |= num<<8;
			
			r = StrFindString((uint8_t *)(pcValue[iIndex]+p+q+2),str_len-p-q-2, (uint8_t *)".", 1);
			if(r == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+q+2),r,&num);
			ip |= num<<16;
			
			if(str_len-p-q-r-3 <= 0)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+q+r+3),str_len-p-q-r-3,&num);
			ip |= num<<24;
			
			DeviceIP = ip;
		}
		iIndex = FindCGIParameter("setnetmask",pcParam,iNumParams);  //子网掩码
		if(iIndex != -1)
		{
			uint16_t p,q,r;
			uint16_t str_len;
			uint32_t num;
			uint32_t netmask = 0;
			
			str_len = strlen(pcValue[iIndex]);
			p = StrFindString((uint8_t *)pcValue[iIndex],str_len, (uint8_t *)".", 1);
			if(p == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)pcValue[iIndex],p,&num);
			netmask = num;
			q = StrFindString((uint8_t *)(pcValue[iIndex]+p+1),str_len-p-1, (uint8_t *)".", 1);
			if(q == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+1),q,&num);
			netmask |= num<<8;
			
			r = StrFindString((uint8_t *)(pcValue[iIndex]+p+q+2),str_len-p-q-2, (uint8_t *)".", 1);
			if(r == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+q+2),r,&num);
			netmask |= num<<16;
			
			Str2Num((uint8_t *)(pcValue[iIndex]+p+q+r+3),str_len-p-q-r-3,&num);
			netmask |= num<<24;
			
			DeviceNetmask = netmask;
		}
		iIndex = FindCGIParameter("setgateway",pcParam,iNumParams);  //网关
		if(iIndex != -1)
		{
			uint16_t p,q,r;
			uint16_t str_len;
			uint32_t num;
			uint32_t gateway = 0;
			
			str_len = strlen(pcValue[iIndex]);
			p = StrFindString((uint8_t *)pcValue[iIndex],str_len, (uint8_t *)".", 1);
			if(p == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)pcValue[iIndex],p,&num);
			gateway = num;
			q = StrFindString((uint8_t *)(pcValue[iIndex]+p+1),str_len-p-1, (uint8_t *)".", 1);
			if(q == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+1),q,&num);
			gateway |= num<<8;
			
			r = StrFindString((uint8_t *)(pcValue[iIndex]+p+q+2),str_len-p-q-2, (uint8_t *)".", 1);
			if(r == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+q+2),r,&num);
			gateway |= num<<16;
			
			if(str_len-p-q-r-3 <= 0)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+q+r+3),str_len-p-q-r-3,&num);
			gateway |= num<<24;
			
			DeviceGw = gateway;
		}
			
		iIndex = FindCGIParameter("setserverport",pcParam,iNumParams);  //服务器端口
		if(iIndex != -1)
		{
			uint16_t port;
			port = atoi(pcValue[iIndex]);
			if(port == 0 || strlen(pcValue[iIndex]) == 0)
				return "/home.html";
			TCPServerPort = HALFWORD_Reverse(port);
		}
		iIndex = FindCGIParameter("setdhcp",pcParam,iNumParams);  //dhcp
		if(iIndex != -1)
		{
			if(strcmp(pcValue[iIndex],"OFF") == 0)
				DHCP_Enable_Flag = 0;
			else if(strcmp(pcValue[iIndex],"ON") == 0)
				DHCP_Enable_Flag = 1;
		}
		iIndex = FindCGIParameter("setntp",pcParam,iNumParams);  //ntp服务器
		if(iIndex != -1)
		{
			uint16_t p,q,r;
			uint16_t str_len;
			uint32_t num;
			uint32_t ip = 0;
			
			str_len = strlen(pcValue[iIndex]);
			p = StrFindString((uint8_t *)pcValue[iIndex],str_len, (uint8_t *)".", 1);
			if(p == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)pcValue[iIndex],p,&num);
			ip = num;
			q = StrFindString((uint8_t *)(pcValue[iIndex]+p+1),str_len-p-1, (uint8_t *)".", 1);
			if(q == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+1),q,&num);
			ip |= num<<8;
			
			r = StrFindString((uint8_t *)(pcValue[iIndex]+p+q+2),str_len-p-q-2, (uint8_t *)".", 1);
			if(r == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+q+2),r,&num);
			ip |= num<<16;
			
			if(str_len-p-q-r-3 <= 0)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+q+r+3),str_len-p-q-r-3,&num);
			ip |= num<<24;
			
			NTPServerIP = ip;
		}
		iIndex = FindCGIParameter("setserverip",pcParam,iNumParams);  //服务器ip
		if(iIndex != -1)
		{
			uint16_t p,q,r;
			uint16_t str_len;
			uint32_t num;
			uint32_t ip = 0;
			
			str_len = strlen(pcValue[iIndex]);
			p = StrFindString((uint8_t *)pcValue[iIndex],str_len, (uint8_t *)".", 1);
			if(p == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)pcValue[iIndex],p,&num);
			ip = num;
			q = StrFindString((uint8_t *)(pcValue[iIndex]+p+1),str_len-p-1, (uint8_t *)".", 1);
			if(q == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+1),q,&num);
			ip |= num<<8;
			
			r = StrFindString((uint8_t *)(pcValue[iIndex]+p+q+2),str_len-p-q-2, (uint8_t *)".", 1);
			if(r == 0xffff)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+q+2),r,&num);
			ip |= num<<16;
			
			if(str_len-p-q-r-3 <= 0)
				return "/home.html";
			Str2Num((uint8_t *)(pcValue[iIndex]+p+q+r+3),str_len-p-q-r-3,&num);
			ip |= num<<24;
			
			TCPServerIP = ip;
		}
		Write_UserMem_Flash(DeviceIP_ADDR,(uint8_t *)&DeviceIP,4);
		Write_UserMem_Flash(DeviceNetmask_ADDR,(uint8_t *)&DeviceNetmask,4);
		Write_UserMem_Flash(DeviceGw_ADDR,(uint8_t *)&DeviceGw,4);
		Write_UserMem_Flash(ServerPort_ADDR,(uint8_t *)&TCPServerPort,2);
		Write_UserMem_Flash(DHCP_Enable_ADDR,(uint8_t *)&DHCP_Enable_Flag,1);
		Write_UserMem_Flash(NTPServerIP_ADDR,(uint8_t *)&NTPServerIP,4);
		Write_UserMem_Flash(ServerIP_ADDR,(uint8_t *)&TCPServerIP,4);
	}
	iIndex = FindCGIParameter("btn_rst",pcParam,iNumParams);  //参数复位
	if(iIndex != -1) //btn_rst
	{
		if(strcmp(pcValue[iIndex],"reset") == 0)  //reset
		{
			DeviceIP = 192 | 168<<8 | 1<<16 | 105<<24;
			Write_UserMem_Flash(DeviceIP_ADDR,(uint8_t *)&DeviceIP,4);
			DeviceNetmask = 255 | 255<<8 | 255<<16 | 0<<24;
			Write_UserMem_Flash(DeviceNetmask_ADDR,(uint8_t *)&DeviceNetmask,4);
			DeviceGw = 192 | 168<<8 | 1<<16 | 1<<24;
			Write_UserMem_Flash(DeviceGw_ADDR,(uint8_t *)&DeviceGw,4);
			TCPServerIP = 121 | 199<<8 | 69<<16 | 67<<24;
			Write_UserMem_Flash(ServerIP_ADDR,(uint8_t *)&TCPServerIP,4);
			TCPServerPort = HALFWORD_Reverse(9001);
			Write_UserMem_Flash(ServerPort_ADDR,(uint8_t *)&TCPServerPort,2);
			DHCP_Enable_Flag = 0;
			Write_UserMem_Flash(DHCP_Enable_ADDR,(uint8_t *)&DHCP_Enable_Flag,1);
			NTPServerIP = 120 | 25<<8 | 115<<16 | 20<<24;
			Write_UserMem_Flash(NTPServerIP_ADDR,(uint8_t *)&NTPServerIP,4);
		}
	}
	NVIC_SystemReset();
	return "/handleip.html";
}


const char* SensorCtrl_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	iIndex = FindCGIParameter("btn_sensor",pcParam,iNumParams);  //传感器挂载设置按钮
	if(iIndex != -1)
	{
		SENSOR_485_USE_TYPE sensor_use_data;
		
		SENSOR_485_USE_Data_RW(0, &sensor_use_data);
		if(FindCGIParameter("SM5386",pcParam,iNumParams) != -1)	//搜博风速
		{
			sensor_use_data.bit_use.SM5386 = 1;
			myprintf("\r\n SM5386 搜博风速-->已启用");
		}
		else
		{
			sensor_use_data.bit_use.SM5386 = 0;
			myprintf("\r\n SM5386 搜博风速-->已停用");
		}
		if(FindCGIParameter("SM5387B",pcParam,iNumParams) != -1)	//搜博风向
		{
			sensor_use_data.bit_use.SM5387B = 1;
			myprintf("\r\n SM5387B 搜博风向-->已启用");
		}
		else	
		{
			sensor_use_data.bit_use.SM5387B = 0;
			myprintf("\r\n SM5387B 搜博风向-->已停用");
		}
		if(FindCGIParameter("SM6333B",pcParam,iNumParams) != -1)	//搜博温湿度粉尘
		{
			sensor_use_data.bit_use.SM6333B = 1;
			myprintf("\r\n SM6333B 搜博温湿度粉尘-->已启用");
		}
		else
		{
			sensor_use_data.bit_use.SM6333B = 0;
			myprintf("\r\n SM6333B 搜博温湿度粉尘-->已停用");
		}
		if(FindCGIParameter("MTC70M",pcParam,iNumParams) != -1)		//默律倾角
		{
			sensor_use_data.bit_use.MTC70M = 1;
			myprintf("\r\n MTC70M 默律倾角-->已启用");
		}
		else
		{
			sensor_use_data.bit_use.MTC70M = 0;
			myprintf("\r\n MTC70M 默律倾角-->已停用");
		}
		if(FindCGIParameter("IOTB2_16",pcParam,iNumParams) != -1)	//透明智能空开
		{
			sensor_use_data.bit_use.IOTB2_16 = 1;
			myprintf("\r\n IOTB2_16 透明智能空开-->已启用");
		}
		else
		{
			sensor_use_data.bit_use.IOTB2_16 = 0;
			myprintf("\r\n IOTB2_16 透明智能空开-->已停用");
		}
		if(FindCGIParameter("WS301",pcParam,iNumParams) != -1)		//耘农温湿度
		{
			sensor_use_data.bit_use.WS301 = 1;
			myprintf("\r\n WS301 耘农温湿度-->已启用");
		}
		else
		{
			sensor_use_data.bit_use.WS301 = 0;
			myprintf("\r\n WS301 耘农温湿度-->已停用");
		}
		if(FindCGIParameter("HCD68XX",pcParam,iNumParams) != -1)	//耘农多合一
		{
			sensor_use_data.bit_use.HCD68XX = 1;
			myprintf("\r\n HCD68XX 耘农多合一-->已启用");
		}
		else
		{
			sensor_use_data.bit_use.HCD68XX = 0;
			myprintf("\r\n HCD68XX 耘农多合一-->已停用");
		}
		if(FindCGIParameter("ManholeCover_angle",pcParam,iNumParams) != -1)	//耘农多合一
		{
			sensor_use_data.bit_use.ManholeCover_MTC70M = 1;
			myprintf("\r\n 默律倾角模拟井盖-->已启用");
		}
		else
		{
			sensor_use_data.bit_use.ManholeCover_MTC70M = 0;
			myprintf("\r\n 默律倾角模拟井盖-->已停用");
		}
		SENSOR_485_USE_Data_RW(1, &sensor_use_data);
		Write_UserMem_Flash(SENSOR_485_USE_ADDR,(uint8_t *)&sensor_use_data.byte_use,4);
	}
	return "/home.html";
}

const char* Env_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	uint8_t i = 0;
	uint16_t len = 0;
	Environment_Type environment_data;
	SENSOR_485_USE_TYPE sensor_use_data;
	ANGLE_MTC70M_Type angle_data;
	
	memset(data_response_buf,0,strlen((const char *)data_response_buf));
	Environment_Data_RW(0,&environment_data);
	SENSOR_485_USE_Data_RW(0, &sensor_use_data);
	{
		if(environment_data.temperature & 0x8000)
		{
			environment_data.temperature = (environment_data.temperature & 0x7fff);
			len += sprintf((char *)&data_response_buf[len],"%.2f",environment_data.temperature/100.0*(-1));
		}
		else
			len += sprintf((char *)&data_response_buf[len],"%.2f",environment_data.temperature/100.0);
			
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%.2f",environment_data.humidity/100.0);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%.2f",environment_data.pm2_5/1.0);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%.2f",environment_data.pm10/1.0);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%.2f",environment_data.air_pressure/10.0);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%.2f",environment_data.wind_speed/100.0);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%.2f",environment_data.wind_direction/100.0);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%.2f",environment_data.noise/10.0);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",sensor_use_data.bit_use.SM5386);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",sensor_use_data.bit_use.SM5387B);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",sensor_use_data.bit_use.SM6333B);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",sensor_use_data.bit_use.MTC70M);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",sensor_use_data.bit_use.IOTB2_16);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",sensor_use_data.bit_use.WS301);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",sensor_use_data.bit_use.HCD68XX);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",sensor_use_data.bit_use.ManholeCover_MTC70M);
		data_response_buf[len] = ';';
		len++;
		Angle_Data_RW(0, &angle_data);
		len += sprintf((char *)&data_response_buf[len],"%.2f", (float)(angle_data.angle_x)/10);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%.2f", (float)(angle_data.angle_y)/10);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%.2f", (float)(angle_data.angle_z)/10);
		data_response_buf[len] = ';';
		len++;
		ManholeCover_angle_Data_RW(0, 0, &angle_data);
		len += sprintf((char *)&data_response_buf[len],"%.2f", (float)(angle_data.angle_x)/10);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%.2f", (float)(angle_data.angle_y)/10);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%.2f", (float)(angle_data.angle_z)/10);
		data_response_buf[len] = ';';
		len++;
		ManholeCover_angle_Data_RW(0, 0, &angle_data);
		len += sprintf((char *)&data_response_buf[len],"%d", !(HAL_GPIO_ReadPin(INPUT_GPIO_Port, INPUT_Pin)));
		data_response_buf[len] = ';';
		len++;
	}
}

const char* TimeCtrl_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	uint8_t i = 0;
	uint16_t len = 0;
	TimeControl_Type timecontrol_data;
	memset(data_response_buf,0,strlen((const char *)data_response_buf));
	
	for(i=1; i<=(LED_NUM+CUSTOM_DEV_NUM); i++)
	{
		if(i<=LED_NUM)
		{
			Get_TimeControl_Data(i, &timecontrol_data);
		}
		
		#if CUSTOM_DEV_NUM
		else
		{
			Get_TimeCustomCtrl_Data(i-LED_NUM, (TimeCustomControl_Type *)&timecontrol_data);
		}
		#endif
		
		len += sprintf((char *)&data_response_buf[len],"%02d:%02d",timecontrol_data.start_hour,timecontrol_data.start_min);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",timecontrol_data.phase1_time);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",timecontrol_data.phase1_Pwm);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",timecontrol_data.phase2_time);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",timecontrol_data.phase2_Pwm);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",timecontrol_data.phase3_time);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",timecontrol_data.phase3_Pwm);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",timecontrol_data.phase4_time);
		data_response_buf[len] = ';';
		len++;
		len += sprintf((char *)&data_response_buf[len],"%d",timecontrol_data.phase4_Pwm);
		data_response_buf[len] = ';';
		len++;
	}
}

const char* TimeCtrlSet_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	uint8_t BtnSetFlg = 0;
	TimeControl_Type timecontrol_data;
	if(FindCGIParameter("btn1",pcParam,iNumParams) != -1)//设置按钮
	{
		BtnSetFlg = 1;
	}
	else if(FindCGIParameter("btn2",pcParam,iNumParams) != -1)//设置按钮
	{
		BtnSetFlg = 2;
	}
	else if(FindCGIParameter("btn3",pcParam,iNumParams) != -1)//设置按钮
	{
		BtnSetFlg = 3;
	}
	else if(FindCGIParameter("btn4",pcParam,iNumParams) != -1)//设置按钮
	{
		BtnSetFlg = 4;
	}
	if(BtnSetFlg > 0)
	{
		uint16_t p,q;
		uint16_t str_len;
		uint32_t num;
		uint16_t temp_time;
		
		iIndex = FindCGIParameter("start_time",pcParam,iNumParams);  //开始时间
		if(iIndex != -1)
		{
			str_len = strlen(pcValue[iIndex]);
			p = StrFindString((uint8_t *)pcValue[iIndex],str_len, (uint8_t *)"%3A", 3);
			if(p == 0xffff)
				return "/set.html";
			Str2Num((uint8_t *)pcValue[iIndex],p,(uint32_t *)&num);
			timecontrol_data.start_hour = num;
			
			Str2Num((uint8_t *)(pcValue[iIndex]+p+3),str_len-p-3,&num);
			timecontrol_data.start_min = num;
		}
		if(BtnSetFlg > LED_NUM)
			iIndex = FindCGIParameter("usw_tim1",pcParam,iNumParams);
		else
			iIndex = FindCGIParameter("led_tim1",pcParam,iNumParams);
		if(iIndex != -1)
		{
			timecontrol_data.phase1_time = atoi(pcValue[iIndex]);
			//timecontrol_data.phase1_time = HALFWORD_Reverse(temp_time);
			if(timecontrol_data.phase1_time>1440)
			{
				timecontrol_data.phase1_time = timecontrol_data.phase1_time%1440;
			}
		}
		if(BtnSetFlg > LED_NUM)
			iIndex = FindCGIParameter("usw1",pcParam,iNumParams);
		else
			iIndex = FindCGIParameter("led_pwm1",pcParam,iNumParams);
		if(iIndex != -1)
		{
			timecontrol_data.phase1_Pwm = atoi(pcValue[iIndex]);
			if(timecontrol_data.phase1_Pwm>100)
			{
				timecontrol_data.phase1_Pwm = 100;
			}
		}
		if(BtnSetFlg > LED_NUM)
			iIndex = FindCGIParameter("usw_tim2",pcParam,iNumParams);
		else
			iIndex = FindCGIParameter("led_tim2",pcParam,iNumParams);
		if(iIndex != -1)
		{
			timecontrol_data.phase2_time = atoi(pcValue[iIndex]);
			//timecontrol_data.phase2_time = HALFWORD_Reverse(temp_time);
			if(timecontrol_data.phase2_time>1440)
			{
				timecontrol_data.phase2_time = timecontrol_data.phase2_time%1440;
			}
		}
		if(BtnSetFlg > LED_NUM)
			iIndex = FindCGIParameter("usw2",pcParam,iNumParams);
		else
			iIndex = FindCGIParameter("led_pwm2",pcParam,iNumParams);
		if(iIndex != -1)
		{
			timecontrol_data.phase2_Pwm = atoi(pcValue[iIndex]);
			if(timecontrol_data.phase2_Pwm>100)
			{
				timecontrol_data.phase2_Pwm = 100;
			}
		}
		if(BtnSetFlg > LED_NUM)
			iIndex = FindCGIParameter("usw_tim3",pcParam,iNumParams);
		else
			iIndex = FindCGIParameter("led_tim3",pcParam,iNumParams);
		if(iIndex != -1)
		{
			timecontrol_data.phase3_time = atoi(pcValue[iIndex]);
			if(timecontrol_data.phase3_time>1440)
			{
				timecontrol_data.phase3_time = timecontrol_data.phase3_time%1440;
			}
		}
		if(BtnSetFlg > LED_NUM)
			iIndex = FindCGIParameter("usw3",pcParam,iNumParams);
		else
			iIndex = FindCGIParameter("led_pwm3",pcParam,iNumParams);
		if(iIndex != -1)
		{
			timecontrol_data.phase3_Pwm = atoi(pcValue[iIndex]);
			if(timecontrol_data.phase3_Pwm>100)
			{
				timecontrol_data.phase3_Pwm = 100;
			}
		}
		if(BtnSetFlg > LED_NUM)
			iIndex = FindCGIParameter("usw_tim4",pcParam,iNumParams);
		else
			iIndex = FindCGIParameter("led_tim4",pcParam,iNumParams);
		if(iIndex != -1)
		{
			timecontrol_data.phase4_time = atoi(pcValue[iIndex]);
			if(timecontrol_data.phase4_time>1440)
			{
				timecontrol_data.phase4_time = timecontrol_data.phase4_time%1440;
			}
		}
		if(BtnSetFlg > LED_NUM)
			iIndex = FindCGIParameter("usw4",pcParam,iNumParams);
		else
			iIndex = FindCGIParameter("led_pwm4",pcParam,iNumParams);
		if(iIndex != -1)
		{
			timecontrol_data.phase4_Pwm = atoi(pcValue[iIndex]);
			if(timecontrol_data.phase4_Pwm>100)
			{
				timecontrol_data.phase4_Pwm = 100;
			}
		}
		
		if(BtnSetFlg <= LED_NUM)
		{
			Write_TimeControl_Data(BtnSetFlg,&timecontrol_data);
			Write_UserMem_Flash(TIMECONTROL_ADDR + (BtnSetFlg-1)*14,(uint8_t *)&timecontrol_data, sizeof(timecontrol_data));
			myprintf("\r\n灯头%d 时间控制策略:开始时间:%02d:%02d,阶段一:time:%d  pwm:%d  阶段二:time:%d  pwm:%d  阶段三:time:%d  pwm:%d  阶段四:time:%d  pwm:%d\r\n",\
			BtnSetFlg,timecontrol_data.start_hour,timecontrol_data.start_min,timecontrol_data.phase1_time,timecontrol_data.phase1_Pwm,\
			timecontrol_data.phase2_time,timecontrol_data.phase2_Pwm,timecontrol_data.phase3_time,timecontrol_data.phase3_Pwm,\
			timecontrol_data.phase4_time,timecontrol_data.phase4_Pwm);
		}
		#if CUSTOM_DEV_NUM
		else
		{
			Write_TimeCustomCtrl_Data(BtnSetFlg-LED_NUM,(TimeCustomControl_Type *)&timecontrol_data);
			Write_UserMem_Flash(TIMECTRCUSTOM_ADDR + (BtnSetFlg-1-LED_NUM)*14,(uint8_t *)&timecontrol_data, sizeof(timecontrol_data));
			myprintf("\r\n用户自定义设备%d 时间控制策略:开始时间:%02d:%02d,阶段一:time:%d  sta:%d  阶段二:time:%d  sta:%d  阶段三:time:%d  sta:%d  阶段四:time:%d  sta:%d\r\n",\
			BtnSetFlg - LED_NUM,timecontrol_data.start_hour,timecontrol_data.start_min,timecontrol_data.phase1_time,timecontrol_data.phase1_Pwm,\
			timecontrol_data.phase2_time,timecontrol_data.phase2_Pwm,timecontrol_data.phase3_time,timecontrol_data.phase3_Pwm,\
			timecontrol_data.phase4_time,timecontrol_data.phase4_Pwm);
		}
		#endif
	}
	return "/ledctrl.html";
}

const char* LedCtrl_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	uint8_t ledpwm,ledsta;
	iIndex = FindCGIParameter("btn3",pcParam,iNumParams);
	if(iIndex != -1)
	{
		iIndex = FindCGIParameter("led1",pcParam,iNumParams);
		if(iIndex != -1)
		{
			ledpwm = atoi(pcValue[iIndex]);
			if(ledpwm <= 0)
			{
				ledpwm = 0;
				ledsta = 0;
			}
			else if(ledpwm >= 100)
			{
				ledpwm = 100;
				ledsta = 1;
			}
			else
			{
				ledpwm = ledpwm;
				ledsta = 1;
			}
			Write_LED_Data(1,&ledsta,&ledpwm);	//LED亮度目标百分比调整
		}
		iIndex = FindCGIParameter("led2",pcParam,iNumParams);
		if(iIndex != -1)
		{
			ledpwm = atoi(pcValue[iIndex]);
			if(ledpwm <= 0)
			{
				ledpwm = 0;
				ledsta = 0;
			}
			else if(ledpwm >= 100)
			{
				ledpwm = 100;
				ledsta = 1;
			}
			else
			{
				ledpwm = ledpwm;
				ledsta = 1;
			}
			Write_LED_Data(2,&ledsta,&ledpwm);	//LED亮度目标百分比调整
		}
		#if CUSTOM_DEV_NUM
		iIndex = FindCGIParameter("usw1",pcParam,iNumParams);
		if(iIndex != -1)
		{
			ledsta = atoi(pcValue[iIndex]);
			if(ledsta >= 1)
			{
				ledsta = 1;
			}
			else
			{
				ledsta = 0;
			}
			Write_LED_Data(3,&ledsta,&ledpwm);	//LED亮度目标百分比调整
		}
		else
		{
			ledsta = 0;
			Write_LED_Data(3,&ledsta,&ledpwm);	//LED亮度目标百分比调整
		}
		iIndex = FindCGIParameter("usw2",pcParam,iNumParams);
		if(iIndex != -1)
		{
			ledsta = atoi(pcValue[iIndex]);
			if(ledsta >= 1)
			{
				ledsta = 1;
			}
			else
			{
				ledsta = 0;
			}
			Write_LED_Data(4,&ledsta,&ledpwm);	//LED亮度目标百分比调整
		}
		else
		{
			ledsta = 0;
			Write_LED_Data(4,&ledsta,&ledpwm);	//LED亮度目标百分比调整
		}
		#endif
		xSemaphoreGive(LedShowBinary);			//演示模式
	}
	return "/ledctrl.html";
}

const char* LedSta_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	uint16_t len = 0;
	uint8_t ledstatus,ledpwm,i;
	CollectionData electricdata = {0};
	memset(data_response_buf,0,strlen((const char *)data_response_buf));
	
	for(i=1; i<=(LED_NUM+CUSTOM_DEV_NUM); i++)
	{
		Get_LED_Data(i,&ledstatus,&ledpwm);
		if(i<=LED_NUM)
		{
			len += sprintf((char *)&data_response_buf[len],"%d",ledpwm);
		}
		else
		{
			len += sprintf((char *)&data_response_buf[len],"%d",ledstatus);
		}
		data_response_buf[len] = ';';
		len++;
	}
	for(i=1; i<=(LED_NUM+CUSTOM_DEV_NUM); i++)
	{
		Get_ElectricData(i, &electricdata);
		len += sprintf((char *)&data_response_buf[len],"V%d = %dV,A = %dmA,P = %dW,E= %.3fkwh",i,\
			electricdata.Voltage/1000, electricdata.Current, electricdata.Power/1000, (float)electricdata.Energy/100);
		data_response_buf[len] = ';';
		len++;
	}
}

//SSI句柄初始化
void httpd_ssi_init(void)
{
#if LWIP_HTTPD_SSI_MULTIPART == 1
	//配置SSI句柄
	http_set_ssi_handler(SSIHandler,ppcTAGs,NUM_CONFIG_SSI_TAGS);
#endif
}

//CGI句柄初始化
void httpd_cgi_init(void)
{ 
  //配置CGI句柄
  http_set_cgi_handlers(ppcURLs, NUM_CONFIG_CGI_URIS);
}


