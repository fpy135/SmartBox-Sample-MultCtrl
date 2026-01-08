#include <lwip/sockets.h>
#include <lwip/err.h>
#include <lwip/sys.h>
#include "lwip/dns.h"
#include "lwip/udp.h"
#include "lwip.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>								//包含需要的所有头文件，（实现socket和标准输入输出）
#include "SysConfig.h"
#include "mystring.h"

#include "socketserver.h"
#include "usart.h"
#include "Protocol.h"
#include "Led_App.h"
#include "IDConfig.h"
#include "MyFlash.h"
#include "Unixtimer.h"
#include "rtc.h"
#include "Platform.h"

#include "IAP.h"

extern void GetRTC(rtc_time_t *rtc_time);
extern void SetRTC(rtc_time_t *rtc_time);

uint32_t DeviceIP = 0x6901A8C0;		//192.168.1.105
uint32_t DeviceNetmask = 0x00ffffff;//255.255.255.0
uint32_t DeviceGw = 0x0101A8C0;		//192..168.1.1
uint32_t TCPServerIP = 0x4345C779;	//121.199.69.67
uint16_t TCPServerPort = 9001;
uint8_t DHCP_Enable_Flag = 0;
uint32_t NTPServerIP = 0;
uint8_t socket_reconnet_flag = 0;

uint8_t sockfd_err_flag = 0;

QueueHandle_t TCP_SEND_Queue;
SemaphoreHandle_t TcpMutex;

int sock_fd = -1;								//服务器的 socked
int ntp_socket = -1;

uint8_t tcp_server_recvbuf[TCP_BUFF_SIZE];					//定义数据处理Buff大小为300（为100也无所谓，只要大于等于100就好）
uint8_t tcp_server_sendbuf[TCP_BUFF_SIZE];

uint8_t udp_server_recvbuf[TCP_BUFF_SIZE/2];
uint8_t getTrimingDNS_flag = 0;
uint8_t get_ntp_flag = 0;


BaseType_t Tcp_TaskID;
xTaskHandle pvCreatedTask_Tcp;


uint8_t Tcp_Connect_Flag = ConnectOffLine;

static uint32_t gettimetick = 0;
ip_addr_t tirming_addr;                   //记录IP地址
uint8_t tirming_addrtype;

void TcpConnectFlag_RW(uint8_t rw,uint8_t *flag)
{
	xSemaphoreTake (TcpMutex, portMAX_DELAY);
	if(rw == 0)
	{
		*flag = Tcp_Connect_Flag;
	}
	else
	{
		Tcp_Connect_Flag = *flag;
	}
	xSemaphoreGive (TcpMutex);
}

#if LOCAL_NTP_SERVER
//--- 同步时间，每个NTP服务器
int8_t ntpTime(void)
{
	uint8_t tmp[48];
	struct sockaddr_in udp_server_addr,remote_addr,udp_tirming_addr;
	if((HAL_GetTick() - gettimetick)>24*3600*1000 || gettimetick == 0 || get_ntp_flag == 0)	//每24小时重新获取时间
	{
		int err = -1;
		socklen_t  addrlen; 
		int recv_data_len;

		get_ntp_flag = 0;
		gettimetick = HAL_GetTick();
restart:
		myprintf("\r\n 连接授时服务中心\r\n");
/*		while(1)
		{
			ntp_socket = socket(AF_INET, SOCK_DGRAM, 0);
			if (ntp_socket < 0)
			{
				vTaskDelay(250);
			}
			else	break;
		} */
		{
			ntp_socket = socket(AF_INET, SOCK_DGRAM, 0);
			if (ntp_socket < 0)
			{
//				return -1;
			}
		}
		memset(&udp_server_addr, 0, sizeof(udp_server_addr));
		udp_server_addr.sin_family = AF_INET;
		udp_server_addr.sin_addr.s_addr = INADDR_ANY;
		udp_server_addr.sin_port = htons(30303);		//可不填
		err = bind(ntp_socket, (struct sockaddr*)&udp_server_addr, sizeof(udp_server_addr));
		if(err<0)
		{
			myprintf("bind error\r\n");
			return -1;
		}
		myprintf("connect NTP IP: %d.%d.%d.%d\r\n", (NTPServerIP&0x000000ff),((NTPServerIP&0x0000ff00)>>8),((NTPServerIP&0x00ff0000)>>16),(NTPServerIP&0xff000000)>>24);
		memset(&udp_tirming_addr, 0, sizeof(udp_tirming_addr));
		udp_tirming_addr.sin_family = AF_INET;
//		udp_tirming_addr.sin_addr.s_addr =  inet_addr(Local_ntp_ip);
		udp_tirming_addr.sin_addr.s_addr =  NTPServerIP;
		udp_tirming_addr.sin_port = htons(123);
//		udp_tirming_addr.sin_addr.s_addr = inet_addr(SERVER_IP);	//测试上位机UDP服务器
//		udp_tirming_addr.sin_port = htons(20018);
//		err = bind(ntp_socket, (struct sockaddr*)&udp_tirming_addr, sizeof(udp_tirming_addr));
		if(-1 == connect(ntp_socket, (struct sockaddr *)&udp_tirming_addr, sizeof(udp_tirming_addr)))
		{
			myprintf("UDP 连接失败\r\n");
			return -1;
		}
		/*********** ntp udp send packed*************/
		{
			uint32_t frame;
			memset(tmp,0,48);
			frame = ((LI << 30) | (VN << 27) | (MODE << 24) | (STRATUM << 16) | (POLL << 8) | (PREC & 0xff));//构造协议头部信息
			tmp[0] = frame>>24;
			tmp[1] = frame>>16;
			tmp[2] = frame>>8;
			tmp[3] = frame;
			tmp[12]  = 49;
			tmp[13]  = 0x4E;
			tmp[14]  = 49;
			tmp[15]  = 52;
		}			
		myprintf("\r\n向服务器发送：");
		myprintf_buf_unic((uint8_t *)tmp,48);
		sendto(ntp_socket, (char *)tmp, sizeof(tmp),0,(struct sockaddr*)&udp_tirming_addr,sizeof(udp_tirming_addr));
		while(1)	
		{
			static uint8_t count = 0;

			count++;
			recv_data_len = recv(ntp_socket,udp_server_recvbuf,TCP_BUFF_SIZE,MSG_DONTWAIT);

			if(recv_data_len>0)
			{
				/*显示发送端的IP地址*/
				myprintf("receive from %s\n",inet_ntoa(remote_addr.sin_addr));
				myprintf("\r\n UDP recever:");
				myprintf_buf_unic((uint8_t *)udp_server_recvbuf,recv_data_len);
				{
					unsigned long highWord;
					unsigned long lowWord;
					 unsigned long secsSince1900;
					// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
					const unsigned long seventyYears = 2208988800UL;
					// subtract seventy years:
					unsigned long epoch;
					highWord = (udp_server_recvbuf[32]<<8 | (udp_server_recvbuf[33]));
					lowWord = (udp_server_recvbuf[34]<<8 | (udp_server_recvbuf[35]));
					 secsSince1900 = highWord << 16 | lowWord;
					epoch = secsSince1900 - seventyYears;
					
					{
						rtc_time_t rtc_time;
						/*****************进入临界态保护RtcTime*************/
						covUnixTimeStp2Beijing(epoch,&rtc_time);
						
						myprintf("\r\n现在是北京时间 %d-%d-%d %d:%d:%d\r\n",\
						rtc_time.ui8Year,rtc_time.ui8Month,rtc_time.ui8DayOfMonth,\
						rtc_time.ui8Hour,rtc_time.ui8Minute,rtc_time.ui8Second); // print the second
						/*******将数据写入RTC*********/
						SetRTC(&rtc_time);
						closesocket(ntp_socket);
						get_ntp_flag = 1;
					}
					break;
				}
			}
			myprintf("\r\n----------send count %d----------",count);
			if(count>10)
			{
				static uint8_t re_cnt = 0;
				count = 0;
				closesocket(ntp_socket);
				re_cnt++;
				if(re_cnt>10)
				{
					rtc_time_t rtc_time;
					re_cnt = 0;
					GetRTC(&rtc_time);
					if(rtc_time.ui8Year >= 2020)		//未获取到时间，但本地已有一个时间
						return 0;
					else
						return -1;
//					break;
				}
				else
					goto restart;
			}
			vTaskDelay(100);
		}
	}
	return 1;
}
#else
void My_DNS_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
	myprintf("\r\n%s ip is:%s\r\n",name,ip4addr_ntoa(ipaddr));
	tirming_addr = *ipaddr;
	getTrimingDNS_flag = 1;
}
//--- 同步时间，每个NTP服务器
void ntpTime(void)
{
	uint8_t tmp[48];
	struct sockaddr_in udp_server_addr,remote_addr,udp_tirming_addr;
	
	if((HAL_GetTick() - gettimetick)>24*3600*1000 || gettimetick == 0 || get_ntp_flag == 0)
	{	
		err_t err;
		ip_addr_t dnsserver;
		
		gettimetick = HAL_GetTick();
		IP4_ADDR(&dnsserver, 8, 8,8,8);
		dns_setserver(0, &dnsserver);
		myprintf("DNS Server: %s\n", ip4addr_ntoa(dns_getserver(0)));
		err = dns_gethostbyname(TIME_TIRMING_DNS,&tirming_addr,My_DNS_callback,NULL);
		if (err == ERR_OK)
		{
			myprintf("In cache! IP: %s\n", ip4addr_ntoa(&tirming_addr));
			getTrimingDNS_flag = 1;
		}
		else
			myprintf("Not in cache! err=%d\n", err); // 缓存中没有时需要等待DNS解析完毕在dns_found回调函数中返回结果
	}
	if(getTrimingDNS_flag)
	{
		int err = -1;
		socklen_t  addrlen; 
		int recv_data_len;
		getTrimingDNS_flag = 0;
restart:
		myprintf("\r\n 连接授时服务中心\r\n");
		while(1)
		{
			ntp_socket = socket(AF_INET, SOCK_DGRAM, 0);
			if (ntp_socket < 0)
			{
				vTaskDelay(250);
			}
			else	break;
		}
		memset(&udp_server_addr, 0, sizeof(udp_server_addr));
		udp_server_addr.sin_family = AF_INET;
		udp_server_addr.sin_addr.s_addr = INADDR_ANY;
		udp_server_addr.sin_port = htons(30303);		//可不填
		err = bind(ntp_socket, (struct sockaddr*)&udp_server_addr, sizeof(udp_server_addr));
		if(err<0)
		{
			myprintf("bind error\r\n");
		}
		myprintf("connect NTP IP: %s\n", ip4addr_ntoa(&tirming_addr));
		memset(&udp_tirming_addr, 0, sizeof(udp_tirming_addr));
		udp_tirming_addr.sin_family = AF_INET;
		udp_tirming_addr.sin_addr.s_addr =  inet_addr(ip4addr_ntoa(&tirming_addr));
		udp_tirming_addr.sin_port = htons(123);
//		udp_tirming_addr.sin_addr.s_addr = inet_addr(SERVER_IP);	//测试上位机UDP服务器
//		udp_tirming_addr.sin_port = htons(20018);
//		err = bind(ntp_socket, (struct sockaddr*)&udp_tirming_addr, sizeof(udp_tirming_addr));
		if(-1 == connect(ntp_socket, (struct sockaddr *)&udp_tirming_addr, sizeof(udp_tirming_addr)))
		{
			myprintf("UDP 连接失败\r\n");
		}
		myprintf("UDP 客户端开始 ok\r\n");
		/*********** ntp udp send packed*************/
		{
			memset(tmp,0,48);
			tmp[0] = 0xD3;
			tmp[1] = 0;
			tmp[2] = 6;
			tmp[3] = 0xec;
			tmp[12]  = 49;
			tmp[13]  = 0x4E;
			tmp[14]  = 49;
			tmp[15]  = 52;
		}			
		myprintf("\r\n向服务器发送：");
		myprintf_buf_unic((uint8_t *)tmp,48);
		sendto(ntp_socket, (char *)tmp, sizeof(tmp),0,(struct sockaddr*)&udp_tirming_addr,sizeof(udp_tirming_addr));
		while(1)	
		{
			static uint8_t count = 0;

			count++;
			recv_data_len = recv(ntp_socket,udp_server_recvbuf,TCP_BUFF_SIZE,MSG_DONTWAIT);

			if(recv_data_len>0)
			{
				/*显示发送端的IP地址*/
				myprintf("receive from %s\n",inet_ntoa(remote_addr.sin_addr));
				myprintf("\r\n UDP recever:");
				myprintf_buf_unic((uint8_t *)udp_server_recvbuf,recv_data_len);
				{
					unsigned long highWord;
					unsigned long lowWord;
					 unsigned long secsSince1900;
					// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
					const unsigned long seventyYears = 2208988800UL;
					// subtract seventy years:
					unsigned long epoch;
					highWord = (udp_server_recvbuf[32]<<8 | (udp_server_recvbuf[33]));
					lowWord = (udp_server_recvbuf[34]<<8 | (udp_server_recvbuf[35]));
					 secsSince1900 = highWord << 16 | lowWord;
					epoch = secsSince1900 - seventyYears;
					
					{
						rtc_time_t rtc_time;
						/*****************进入临界态保护RtcTime*************/
						covUnixTimeStp2Beijing(epoch,&rtc_time);
						
						myprintf("\r\n现在是北京时间  %d-%d-%d  %d:%d:%d\r\n",\
						rtc_time.ui8Year,rtc_time.ui8Month,rtc_time.ui8DayOfMonth,\
						rtc_time.ui8Hour,rtc_time.ui8Minute,rtc_time.ui8Second); // print the second
						/*******将数据写入RTC*********/
						SetRTC(&rtc_time);
						closesocket(ntp_socket);
						get_ntp_flag = 1;
					}
					break;
				}
			}
			myprintf("-------------------send count %d-------------------\n",count);
			if(count>10)
			{
				static uint8_t re_cnt = 0;
				count = 0;
				closesocket(ntp_socket);
				re_cnt++;
				if(re_cnt>10)
				{
					re_cnt = 0;
					break;
				}
				else
					goto restart;
			}
			vTaskDelay(100);
		}
	}
}
#endif

void socket_SendData(uint8_t sn,uint16_t type, uint32_t id, uint8_t cmd,uint16_t len,uint8_t *senddata)
{
    uint8_t *pbuf;
	xSemaphoreTake (TcpMutex, portMAX_DELAY);

    pbuf = Protocol_Pack(sn,type,id,cmd,len,(uint8_t *)senddata);
#if TCP_Data_Printf
    myprintf("\r\n TCP send:");
    myprintf_buf_unic((uint8_t *)pbuf, 10+len+2);
#endif

    send(sock_fd, (char *)pbuf, 10+len+2, 0);		//10为负载以前的固定数据的数据长度+负载数据长度+crc
	xSemaphoreGive (TcpMutex);
}

void RemoteUpdata_ack(uint8_t sn, uint16_t type, uint32_t id, uint8_t cmd,uint16_t len, uint8_t *senddata)
{
	uint8_t *pbuf;
	xSemaphoreTake (TcpMutex, portMAX_DELAY);

//	myprintf("\r\n updata ack：");
	sn++;
	pbuf = Protocol_Pack_BB(sn,type,id,cmd,len,(uint8_t *)senddata);
    send(sock_fd, (char *)pbuf, 10+len+2, 0);		//10为负载以前的固定数据的数据长度+负载数据长度+crc
	xSemaphoreGive (TcpMutex);

}

int16_t socket_recv_data(void)
{
	int16_t ret = 0;
	int recvFlag = MSG_DONTWAIT;
	
	xSemaphoreTake (TcpMutex, portMAX_DELAY);
	ret = recv(sock_fd,tcp_server_recvbuf,TCP_BUFF_SIZE,recvFlag);
	xSemaphoreGive (TcpMutex);
	
	return ret;
}

void socket_close(void)
{
	int ret = 0;
	
	xSemaphoreTake (TcpMutex, portMAX_DELAY);
	if(sock_fd < 0)
		return;
	ret = closesocket(sock_fd);
//	ret = lwip_shutdown(sock_fd,SHUT_RDWR);
	myprintf("close socket:%d ret:%d\r\n",sock_fd,ret);
	if(ret < 0)
	{
//		uint8_t i = 0;
//		myprintf("close socket:%x failed\r\n",sock_fd);
//		for(i = 0;i<MEMP_NUM_NETCONN;i++)
//		{
//			ret = closesocket(i);
//			if(ret < 0)
//				myprintf("close socket:%x failed\r\n",i);
//			else
//				myprintf("close socket:%x success\r\n",i);
//		}
	}
	xSemaphoreGive (TcpMutex);
}

void socket_RecvDate_Process(uint8_t *recvdata)
{
	Platform_Data_Process(recvdata);
}

int8_t Create_TCP_connect(void)
{
	struct sockaddr_in server_addr;
	int32_t reuseaddr = 1; // 开启reuseaddr属性
	int32_t bDontLinger = 0; 
	int32_t keepAlive = 1; // 开启keepalive属性
	int32_t keepIdle =  60; // 如该连接在60秒内没有任何数据往来,则进行探测
	int32_t keepInterval = 1; // 探测时发包的时间间隔为1 秒
	int32_t keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
	int32_t keepflag = 1;

	xSemaphoreTake (TcpMutex, portMAX_DELAY);

socket_again:
    sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_fd < 0)
	{
		static uint8_t re_cnt = 0;
		myprintf("\r\nSocket error");
		vTaskDelay(200);
		if(++re_cnt < 5)
		{
			goto socket_again;
		}
		re_cnt = 0;
		xSemaphoreGive (TcpMutex);
		return -1;
	}
	else
	{
		myprintf("\r\nsock_fd:%d",sock_fd);
	}
	
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
//	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
//	server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = TCPServerIP;
    server_addr.sin_port = TCPServerPort;
socketkeepalive_again:	
	//closesocket（一般不会立即关闭而经历TIME_WAIT的过程）后想继续重用该socket
//	if(setsockopt(sock_fd,SOL_SOCKET ,SO_REUSEADDR,(void*)&reuseaddr,sizeof(reuseaddr)) < 0)
//	{
//		goto free_socket;
//	}
	//如果要已经处于连接状态的soket在调用closesocket后强制关闭，不经历 TIME_WAIT的过程：
//	if(setsockopt(sock_fd,SOL_SOCKET,SO_DONTLINGER,(const char*)&bDontLinger,sizeof(bDontLinger)) < 0)
//	{
//		goto free_socket;
//	}
	if(setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive)) < 0)// 打开keepalive功能，需要在执行connect()之前执行
	{
		static uint8_t re_cnt = 0;
		myprintf("\r\nset keepalive fail");
		vTaskDelay(200);
		if(++re_cnt < 5)
		{
			goto socketkeepalive_again;
		}
		re_cnt = 0;
		goto free_socket;
	}
	else
	{
		myprintf("\r\nset keepalive success");
	}
socketconnect_again:	
    if(connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
	{
		static uint8_t re_cnt = 0;
		myprintf("\r\n socket connect error");
		vTaskDelay(200);
 		if(++re_cnt < 5)
		{
			goto socketconnect_again;
		}
		re_cnt = 0;
		goto free_socket;
	}
	else
	{
		myprintf("\r\n socket connect success");
	}
	
//	if(setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepIdle , sizeof(keepIdle)) < 0)
//	{
//		goto free_socket;
//	}
//	if(setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepInterval , sizeof(keepInterval)) < 0)
//	{
//		goto free_socket;
//	}
//	if(setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepCount , sizeof(keepCount)) < 0)
//	{
//		goto free_socket;
//	}
	if(setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&keepflag , sizeof(keepflag)) < 0)
	{
		goto free_socket;
	}
	xSemaphoreGive (TcpMutex);
	return 0;
free_socket:
	myprintf("\r\n socket error, will be close");
	xSemaphoreGive (TcpMutex);
	socket_close();
	return -1;
}

int8_t Create_TCP_Connect_TimeOut(void)
{
	struct sockaddr_in server_addr;
	int32_t reuseaddr = 1; // 开启reuseaddr属性
	int32_t bDontLinger = 0; 
	int32_t keepAlive = 1; // 开启keepalive属性
	int32_t keepIdle =  60; // 如该连接在60秒内没有任何数据往来,则进行探测
	int32_t keepInterval = 1; // 探测时发包的时间间隔为1 秒
	int32_t keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
	int32_t keepflag = 1;
	uint8_t re_cnt = 0;
	
	unsigned long mode = 1;
	int ret=0;

	xSemaphoreTake (TcpMutex, portMAX_DELAY);

socket_again:
    sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_fd < 0)
	{
		myprintf("\r\nSocket error");
		vTaskDelay(200);
		if(++re_cnt < 5)
		{
			goto socket_again;
		}
		re_cnt = 0;
		xSemaphoreGive (TcpMutex);
		return -1;
	}
	else
	{
		re_cnt = 0;
		myprintf("\r\nsock_fd:%d",sock_fd);
	}
	ret = lwip_ioctl(sock_fd, FIONBIO, &mode);	//设置为非阻塞模式
	if(ret)
	{
		goto free_socket;
	}
	
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
//	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
//	server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = TCPServerIP;
    server_addr.sin_port = TCPServerPort;
socketkeepalive_again:	
	//closesocket（一般不会立即关闭而经历TIME_WAIT的过程）后想继续重用该socket
//	if(setsockopt(sock_fd,SOL_SOCKET ,SO_REUSEADDR,(void*)&reuseaddr,sizeof(reuseaddr)) < 0)
//	{
//		goto free_socket;
//	}
	//如果要已经处于连接状态的soket在调用closesocket后强制关闭，不经历 TIME_WAIT的过程：
//	if(setsockopt(sock_fd,SOL_SOCKET,SO_DONTLINGER,(const char*)&bDontLinger,sizeof(bDontLinger)) < 0)
//	{
//		goto free_socket;
//	}
	if(setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive)) < 0)// 打开keepalive功能，需要在执行connect()之前执行
	{
		myprintf("\r\nset keepalive fail");
		vTaskDelay(200);
		if(++re_cnt < 5)
		{
			goto socketkeepalive_again;
		}
		re_cnt = 0;
		goto free_socket;
	}
	else
	{
		re_cnt = 0;
		myprintf("\r\nset keepalive success");
	}
socketconnect_again:	
    if(connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
	{
		myprintf("\r\n socket connect error");
		if(errno != EINPROGRESS)
		{
			goto free_socket;
		}
//		vTaskDelay(200);
// 		if(++re_cnt < 5)
//		{
//			goto socketconnect_again;
//		}
//		re_cnt = 0;
//		goto free_socket;
	}
	else
	{
		re_cnt = 0;
		myprintf("\r\n socket connect success");
	}
	{
		struct timeval tm;
		fd_set rest, west;
		int selectFlag = -1;
		int error=-1, len;
		uint8_t select_cnt = 0;
		
		tm.tv_sec = 1000/1000;
		tm.tv_usec = 1000%1000;
		FD_ZERO(&rest);
		FD_ZERO(&west);
		FD_SET(sock_fd, &rest);
		FD_SET(sock_fd, &west);
		select_again:
		selectFlag = select(sock_fd+1, &rest, &west, NULL, &tm);
		if(selectFlag > 0)
		{
			//如果套接口及可写也可读，需要进一步判断
			if(FD_ISSET(sock_fd, &rest) && FD_ISSET(sock_fd, &west)) 
			{
				if(getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len) < 0)
				{
					myprintf("\r\ngetsockopt error!!!");
				}
				else
				{
					if(sock_fd == 0)
					{
						ret = 1;
					}
					else 
					{
						myprintf("\r\nconnect getsockopt error!!! %d",error);
					}
				}
			}
			//如果套接口可写不可读,则链接完成
			else if(FD_ISSET(sock_fd, &west) && !FD_ISSET(sock_fd, &rest)) 
			{ 
				myprintf("\r\nselect only WD");
				ret = 1;
			}
//			else
//			{
//				myprintf("\r\nconnect select error!!!");
//				goto free_socket;
//			}
		}
		else if(selectFlag == 0)
		{
			#if 1
				select_cnt = 0;
				myprintf("\r\nconnect select timeout!!!");
				goto free_socket;
			#else
				select_cnt++;
				if(select_cnt >= 1)
				{
					select_cnt = 0;
					myprintf("\r\nconnect select timeout!!!");
					goto free_socket;
				}
				else 
				{
					vTaskDelay(1000);
					goto select_again;
				}
			#endif
		}
		else 
		{
			myprintf("\r\nconnect select error!!!");
			goto free_socket;
		}
	}
	
//	if(setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepIdle , sizeof(keepIdle)) < 0)
//	{
//		goto free_socket;
//	}
//	if(setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepInterval , sizeof(keepInterval)) < 0)
//	{
//		goto free_socket;
//	}
//	if(setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepCount , sizeof(keepCount)) < 0)
//	{
//		goto free_socket;
//	}
	if(setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&keepflag , sizeof(keepflag)) < 0)
	{
		goto free_socket;
	}
	xSemaphoreGive (TcpMutex);
	return 0;
free_socket:
	myprintf("\r\n socket error, will be close");
	xSemaphoreGive (TcpMutex);
	socket_close();
	return -1;
}

ReConnect_By_Socekt tcp_reconnect;

int16_t rebuild_TCP_connect(void)
{
	static int16_t connect_info = -1;
	struct sockaddr_in server_addr;
	int recvFlag = MSG_DONTWAIT;
	uint8_t netsta;
	
//	connect_info = recv(sock_fd,tcp_server_recvbuf,TCP_BUFF_SIZE,recvFlag);
	connect_info = socket_recv_data();
	if(connect_info > 0)		//获得到数据
	{
		uint8_t tmp = 1;
		
		tcp_reconnect.connect_cnt = 0;
		netsta = ConnectOnLine;
		TcpConnectFlag_RW(1,&netsta);
		/************数据接收处理************/
		socket_RecvDate_Process(tcp_server_recvbuf);	
		tcp_reconnect.refresh_tick = HAL_GetTick();
	}
	else if((connect_info < 0) && ((errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)))  // 没收到数据
	{
		static uint8_t first_flag = 0;
		
		netsta = ConnectOnLine;
		TcpConnectFlag_RW(1,&netsta);
		if(first_flag == 0)
		{
			first_flag = 1;
			CheckBackUp_Backup();		//备份程序
		}
//		if(tcp_reconnect.refresh_tick == 0)
//			tcp_reconnect.refresh_tick = HAL_GetTick();
		tcp_reconnect.connect_cnt++;
		if(tcp_reconnect.connect_cnt >= 60*10*10)
		{
			tcp_reconnect.connect_cnt = 0;
			myprintf("长时间无数据\r\n");
			goto reconnect;
		}
	}
	else
	{
reconnect:
		{
			int8_t ret = 0;
			tcp_reconnect.connect_cnt = 0;
//			ret = Create_TCP_connect();	//创建一个新的SOCKET
			if((HAL_GetTick() - tcp_reconnect.rebuild_interval) >= 1000 || tcp_reconnect.rebuild_interval == 0)
			{
				myprintf("与服务器断开连接\r\n");
				netsta = ConnectOffLine;
				TcpConnectFlag_RW(1,&netsta);
				socket_close();
				vTaskDelay(100);
				ret = Create_TCP_Connect_TimeOut();
				tcp_reconnect.rebuild_interval = HAL_GetTick();
				if(ret >= 0)
				{
					tcp_reconnect.socket_flag = 0;		//socket 连接成功
//					tcp_reconnect.refresh_tick = HAL_GetTick();
				}
				else
				{
//					if(tcp_reconnect.socket_flag == 0) //前一次socket连接是成功的
//					{
//						tcp_reconnect.socket_flag = 1;
//						tcp_reconnect.socket_reboot_tick = HAL_GetTick();
//					}
//					else if((HAL_GetTick() - tcp_reconnect.socket_reboot_tick)>=3600*1000) //持续超过1小时socket连接失败
//					{
//						rtc_time_t rtc_time;

//						myprintf("超过1小时与服务器socket连接不上\r\n");
//						GetRTC(&rtc_time);
//						if((rtc_time.ui8Hour >= 23) || (rtc_time.ui8Hour <= 5))
//						{
//							myprintf("时间符合夜深人静--->重启\r\n");
//							NVIC_SystemReset();
//						}
//					}
				}
			}
		}
	}
	if(sockfd_err_flag == 1)
	{
		myprintf("sockfd err flag\r\n");
		sockfd_err_flag = 0;
		goto reconnect;
	}
	{
		if(socket_reconnet_flag == 1)		//网线掉线需重新连接
		{
			socket_reconnet_flag = 0;
			goto reconnect;
		}
	}
	{
		if((HAL_GetTick() - tcp_reconnect.refresh_tick)>=3600*1000)		//超过1小时接收不到平台数据
		{
			myprintf("\r\n超过1小时未接受数据-->>重启\r\n");
			netsta = ConnectOffLine;
			TcpConnectFlag_RW(1,&netsta);
			socket_close();
			vTaskDelay(100);
			NVIC_SystemReset();
		}
		if((HAL_GetTick() - tcp_reconnect.socket_reboot_tick)>=3600*24*5*1000) //定时5天重启
		{
			myprintf("定时重启\r\n");
			NVIC_SystemReset();
		}
	}
	return connect_info;
}

#if 0
static void tcp_client_thread(void *p_arg)
{
    struct sockaddr_in server_addr;
    int recvFlag = MSG_DONTWAIT;
    int8_t connect_info;
	
	uint8_t *msgdata = NULL;
//	TCP_SEND_Queue = xQueueCreate( TCP_SEND_QUEUE_LENGTH, sizeof( struct AMessage * ) );


	int32_t keepAlive = 1; // 开启keepalive属性
	int32_t keepIdle = 5; // 如该连接在60秒内没有任何数据往来,则进行探测
	int32_t keepInterval = 1; // 探测时发包的时间间隔为5 秒
	int32_t keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
	int32_t keepflag = 1;
	

    sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);
	
	if(setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive)) < 0)// 打开keepalive功能，需要在执行connect()之前执行
	{
		myprintf("set keepalive fail");
	}
	
    connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
	if (sock_fd < 0)
	{
		printf("Socket error\n");
	//		  goto __exit;
	}
	setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepIdle , sizeof(keepIdle));
	setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepInterval , sizeof(keepInterval));
	setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepCount , sizeof(keepCount));
	setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&keepflag , sizeof(keepflag));
	
    while(1)
    {
		ntpTime();
        connect_info = recv(sock_fd,tcp_server_recvbuf,TCP_BUFF_SIZE,recvFlag);
        if(connect_info > 0)		//获得到数据
        {
			uint8_t tmp = 1;
            Tcp_Connect_Flag = ConnectOnLine;
            /************开关灯代码段************/
            socket_RecvDate_Process(tcp_server_recvbuf);
			
			//测试代码
			{
//				Write_LED_Data(&tmp,&tcp_server_recvbuf[0]);
//				send(sock_fd, (char *)&tcp_server_recvbuf, 14, 0);
//				xSemaphoreGive(LedShowBinary);			//演示模式
			}
            memset(tcp_server_recvbuf,0,14);
        }
        else if((connect_info == -1) && (errno == EWOULDBLOCK))  // 没收到数据
        {
            Tcp_Connect_Flag = ConnectOnLine;
        }
        else
        {
reconnect:
            {
				static uint8_t re_cnt = 0;
                myprintf("与服务器断开连接\r\n");
				Tcp_Connect_Flag = ConnectOffLine;
                closesocket(sock_fd);
				vTaskDelay(100);
                sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
                server_addr.sin_port = htons(SERVER_PORT);
                connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
            }
        }
		{
			uint8_t tmp = 0;
			taskENTER_CRITICAL();
			tmp = ETH_Link_flag;
			taskEXIT_CRITICAL();
			if(tmp == 1)
			{
				taskENTER_CRITICAL();
				ETH_Link_flag = 0;
				taskEXIT_CRITICAL();
				goto reconnect;
			}
		}
        if(xQueuePeek(TCP_SEND_Queue,tcp_server_sendbuf, ( TickType_t )100) == pdPASS)	//接收队列消息
        {
			myprintf("目前风速：%.1f m/s \n",(tcp_server_sendbuf[3]<<8|tcp_server_sendbuf[4])/ 10.0);
			myprintf("目前风向：%.1f° \n",(tcp_server_sendbuf[1]<<8|tcp_server_sendbuf[2])/ 100.0);
			myprintf("目前PM2.5：%.1f ug/m3 \n",(tcp_server_sendbuf[5]<<8|tcp_server_sendbuf[6])/ 10.0);
			myprintf("目前PM10：%.1f ug/m3 \n",(tcp_server_sendbuf[7]<<8|tcp_server_sendbuf[8])/ 10.0);
			myprintf("目前温度：%.1f℃ \n",(tcp_server_sendbuf[9]<<8|tcp_server_sendbuf[10])/ 100.0);
			myprintf("目前湿度：%.1f%% \n",(tcp_server_sendbuf[11]<<8|tcp_server_sendbuf[12])/ 100.0);
			if(Tcp_Connect_Flag == ConnectOnLine)
			{
				socket_SendData(Sn,Device_TYPE,Device_ID,Heart_Cmd,tcp_server_sendbuf[0],(uint8_t *)&tcp_server_sendbuf[1]);
			}
            xQueueReceive(TCP_SEND_Queue, (void *)&tcp_server_sendbuf, ( TickType_t )0);
        }
    }
}

void CreatTCPTask(void)	//TCP客户端初始化
{
    //进入TCP客户端线程
//	sys_thread_new("tcp_client_thread",  tcp_client_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO - 1);
	myprintf("\r\n CreatTCPTask");

    Tcp_TaskID = xTaskCreate(tcp_client_thread, "TCP", TCP_StkSIZE, NULL, TCP_TaskPrio, &pvCreatedTask_Tcp);
    if(pdPASS != Tcp_TaskID)
    {
        myprintf("\r\nTcp_Task creat error");
        while(1);
    }
}
#endif
