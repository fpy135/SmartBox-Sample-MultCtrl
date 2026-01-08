#include "UdpDevSearch.h"
#include <lwip/sockets.h>
#include "lwip/udp.h"

#include <lwip/err.h>
#include <lwip/sys.h>
#include "lwip.h"

#include "usart.h"
#include "IDConfig.h"
#include "socketserver.h"
#include "Protocol.h"
#include "mystring.h"

int devsearch_socket;
struct sockaddr_in udpDevSearchForm_addr;

uint8_t UdpDevSearch_Data[10+19+2] = {0};

int UdpDevSearch_Server_Init(void)
{
	int err = -1;
	struct sockaddr_in udpDevSearch_addr;
	devsearch_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (devsearch_socket < 0)
	{
		_myprintf("\r\ndevsearch_socket err");
//				return -1;
	}
	
	const int opt = 1;
	//设置该套接字为广播类型，
	int nb = 0;
	nb = setsockopt(devsearch_socket, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));
	if(nb == -1)
	{
		_myprintf("\r\ndevsearch udp setsockopt");
		return -1;
	}
	
	memset(&udpDevSearch_addr, 0, sizeof(udpDevSearch_addr));
	udpDevSearch_addr.sin_family = AF_INET;
	udpDevSearch_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	udpDevSearch_addr.sin_port = htons(DEVSEARCH_PORT);		//可不填
	err = bind(devsearch_socket, (struct sockaddr*)&udpDevSearch_addr, sizeof(udpDevSearch_addr));
	if(err<0)
	{
		myprintf("bind error\r\n");
		return -1;
	}
	// 广播地址
	memset(&udpDevSearchForm_addr, 0, sizeof(udpDevSearchForm_addr));
	udpDevSearchForm_addr.sin_family = AF_INET;
	udpDevSearchForm_addr.sin_addr.s_addr = htonl(IPADDR_BROADCAST);
	udpDevSearchForm_addr.sin_port = htons(DEVSEARCH_PORT);
}

int UdpDevSearch_Clint_Init(void)
{
	int err = -1;
	struct sockaddr_in udpDevSearch_addr;

	devsearch_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (devsearch_socket < 0)
	{
		_myprintf("\r\ndevsearch_socket err");
//				return -1;
	}
	
	memset(&udpDevSearch_addr, 0, sizeof(udpDevSearch_addr));
	udpDevSearch_addr.sin_family = AF_INET;
	udpDevSearch_addr.sin_addr.s_addr = INADDR_ANY;
	udpDevSearch_addr.sin_port = htons(30301);		//可不填
	err = bind(devsearch_socket, (struct sockaddr*)&udpDevSearch_addr, sizeof(udpDevSearch_addr));
	if(err<0)
	{
		myprintf("bind error\r\n");
		return -1;
	}
	// 广播地址
	memset(&udpDevSearchForm_addr, 0, sizeof(udpDevSearchForm_addr));
	udpDevSearchForm_addr.sin_family = AF_INET;
	udpDevSearchForm_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	udpDevSearchForm_addr.sin_port = htons(DEVSEARCH_PORT);
}

void UdpDevSearch_Send(void)
{
	uint8_t test[] = "\r\nhello world";
	struct sockaddr_in udpDevSearch_addr;
	
	udpDevSearch_addr.sin_family = AF_INET;
	udpDevSearch_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	udpDevSearch_addr.sin_port = htons(DEVSEARCH_PORT);		//可不填
	sendto(devsearch_socket, (char *)test, sizeof(test), 0,\
		(struct sockaddr*)&udpDevSearch_addr, sizeof(udpDevSearch_addr));
}

void UdpDevSearch_Recv(void)
{
	int len = sizeof(struct sockaddr_in);
	int ret=recvfrom(devsearch_socket, UdpDevSearch_Data, 100, MSG_DONTWAIT, (struct sockaddr*)&udpDevSearchForm_addr,(socklen_t*)&len);
	if(ret<=0)
	{
		
	}
	else
	{
		if(UdpDevSearch_Data[0] == 0xAA)
		{
			uint16_t crc_tmp1,crc_tmp2;
            uint8_t datalen = 10+UdpDevSearch_Data[9]+2;		//10为负载以前的固定数据的数据长度+负载数据长度+crc
            crc_tmp1 = UdpDevSearch_Data[datalen-1]<<8 | UdpDevSearch_Data[datalen-2];
            crc_tmp2 = CRC16_calc(UdpDevSearch_Data,datalen-2);
            if(crc_tmp2 == crc_tmp1 && UdpDevSearch_Data[8] == 0x00)
			{
				uint8_t * pdata;
				uint16_t CRCRes;
		//		pdata = pvPortMalloc(sizeof(uint8_t)*24);
				pdata = UdpDevSearch_Data;
				
				myprintf("\r\n设备IP:%d.%d.%d.%d", (DeviceIP&0x000000ff),((DeviceIP&0x0000ff00)>>8),((DeviceIP&0x00ff0000)>>16),(DeviceIP&0xff000000)>>24);
				myprintf("\r\n子网掩码:%d.%d.%d.%d",(DeviceNetmask&0x000000ff),((DeviceNetmask&0x0000ff00)>>8),((DeviceNetmask&0x00ff0000)>>16),(DeviceNetmask&0xff000000)>>24);
				myprintf("\r\n默认网关:%d.%d.%d.%d",(DeviceGw&0x000000ff),((DeviceGw&0x0000ff00)>>8),((DeviceGw&0x00ff0000)>>16),(DeviceGw&0xff000000)>>24);
				
				myprintf("\r\n服务器IP:%d.%d.%d.%d", (TCPServerIP&0x000000ff),((TCPServerIP&0x0000ff00)>>8),((TCPServerIP&0x00ff0000)>>16),(TCPServerIP&0xff000000)>>24);
				myprintf("\r\n服务器Port:%d",((TCPServerPort&0x00ff) << 8) | ((TCPServerPort&0xff00) >> 8));
				myprintf("\r\nDHCP使能:%d",DHCP_Enable_Flag);
				myprintf("\r\nNTP服务器IP:%d.%d.%d.%d\r\n", (NTPServerIP&0x000000ff),((NTPServerIP&0x0000ff00)>>8),((NTPServerIP&0x00ff0000)>>16),(NTPServerIP&0xff000000)>>24);

		//		*pdata = (DeviceIP&0x000000ff);
		//		*pdata++ = (DeviceIP&0x0000ff00)>>8;
		//		*pdata++ = (DeviceIP&0x00ff0000)>>16;
		//		*pdata++ = (DeviceIP&0xff000000)>>24;
				
				*pdata++ = 0xAA;
				*pdata++ = 0x00;
				
				*pdata++ = (Device_TYPE&0xff00)>>8;
				*pdata++ = (Device_TYPE&0x00ff);
				
				*pdata++ = (Device_ID&0xff000000)>>24;
				*pdata++ = (Device_ID&0x00ff0000)>>16;
				*pdata++ = (Device_ID&0x0000ff00)>>8;
				*pdata++ = (Device_ID&0x000000ff);
				
				//cmd
				*pdata++ = 0x55;
				//len
				*pdata++ = 19;
				
				*pdata++ = (DeviceNetmask&0x000000ff);
				*pdata++ = (DeviceNetmask&0x0000ff00)>>8;
				*pdata++ = (DeviceNetmask&0x00ff0000)>>16;
				*pdata++ = (DeviceNetmask&0xff000000)>>24;
				
				*pdata++ = (DeviceGw&0x000000ff);
				*pdata++ = (DeviceGw&0x0000ff00)>>8;
				*pdata++ = (DeviceGw&0x00ff0000)>>16;
				*pdata++ = (DeviceGw&0xff000000)>>24;
				
				*pdata++ = (TCPServerIP&0x000000ff);
				*pdata++ = (TCPServerIP&0x0000ff00)>>8;
				*pdata++ = (TCPServerIP&0x00ff0000)>>16;
				*pdata++ = (TCPServerIP&0xff000000)>>24;
				
				*pdata++ = (TCPServerPort&0x00ff);
				*pdata++ = (TCPServerPort&0xff00)>>8;
				
				*pdata++ = (NTPServerIP&0x000000ff);
				*pdata++ = (NTPServerIP&0x0000ff00)>>8;
				*pdata++ = (NTPServerIP&0x00ff0000)>>16;
				*pdata++ = (NTPServerIP&0xff000000)>>24;
				
				*pdata++ = DHCP_Enable_Flag;
				
				CRCRes = CRC16_calc(UdpDevSearch_Data, 10+19);
				*pdata++ = (uint8_t)(CRCRes & 0xff);
				*pdata++ = (uint8_t)((CRCRes>>8) & 0xff);
				
				sendto(devsearch_socket, UdpDevSearch_Data, 10+19+2, 0,\
				(struct sockaddr*)&udpDevSearchForm_addr, sizeof(udpDevSearchForm_addr));
			}
		}
	}
}
