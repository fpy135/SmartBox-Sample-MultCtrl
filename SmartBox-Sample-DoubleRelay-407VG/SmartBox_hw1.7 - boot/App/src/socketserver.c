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


QueueHandle_t TCP_SEND_Queue;
SemaphoreHandle_t TcpMutex;

int sock_fd = -1;								//服务器的 socked
int ntp_socket = -1;

uint8_t tcp_server_recvbuf[TCP_BUFF_SIZE];					//定义数据处理Buff大小为300（为100也无所谓，只要大于等于100就好）
uint8_t tcp_server_sendbuf[TCP_BUFF_SIZE];

uint8_t udp_server_recvbuf[TCP_BUFF_SIZE];
uint8_t getTrimingDNS_flag = 0;

BaseType_t Tcp_TaskID;
xTaskHandle pvCreatedTask_Tcp;


uint8_t Tcp_Connect_Flag = ConnectOffLine;

static uint32_t gettimetick = 0;
ip_addr_t tirming_addr;                   //记录IP地址
uint8_t tirming_addrtype;
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
	
	if((HAL_GetTick() - gettimetick)>24*3600*1000 || gettimetick == 0)
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
		getTrimingDNS_flag = 0;
		socklen_t  addrlen; 
		int recv_data_len;
restart:
		myprintf("\r\n 连接国家授时服务中心\r\n");
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
		myprintf("向服务器发送\r\n");
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

void socket_SendData(uint8_t sn,uint16_t type, uint32_t id, uint8_t cmd,uint16_t len,uint8_t *senddata)
{
    uint8_t *pbuf;

	sn++;
    pbuf = Protocol_Pack(sn,type,id,cmd,len,(uint8_t *)senddata);
#if TCP_Data_Printf
    myprintf("\r\n TCP send:");
    myprintf_buf_unic((uint8_t *)pbuf, 10+len+2);
#endif

    send(sock_fd, (char *)pbuf, 10+len+2, 0);		//10为负载以前的固定数据的数据长度+负载数据长度+crc
}

void RemoteUpdata_ack(uint8_t sn, uint16_t type, uint32_t id, uint8_t cmd,uint16_t len, uint8_t *senddata)
{
	uint8_t *pbuf;

//	myprintf("\r\n updata ack：");
	sn++;
	pbuf = Protocol_Pack_BB(sn,type,id,cmd,len,(uint8_t *)senddata);
    send(sock_fd, (char *)pbuf, 10+len+2, 0);		//10为负载以前的固定数据的数据长度+负载数据长度+crc

}


void socket_RecvDate_Process(uint8_t *recvdata)
{
//#if TCP_Data_Printf
//	myprintf("\r\n TCP recever:");
//	myprintf_buf_unic((uint8_t *)recvdata,10+recvdata[9]+2);
//#endif
	Platform_Data_Process(recvdata);
	/*
    if(recvdata[0] == 0xAA)
    {
		uint16_t tmp_type = 0;
		uint32_t tmp_id = 0;
		tmp_type = recvdata[2]<<8 | recvdata[3];
		tmp_id = recvdata[4]<<24 | recvdata[5]<<16 | recvdata[6]<<8 | recvdata[7];
		if(tmp_type == Device_TYPE && tmp_id == Device_ID)		//ID校验
		{
			uint16_t crc_tmp1,crc_tmp2;
			uint8_t datalen = 10+recvdata[9]+2;		//10为负载以前的固定数据的数据长度+负载数据长度+crc
			crc_tmp2 = CRC16_calc(recvdata,datalen-2);
			crc_tmp1 = recvdata[datalen-1]<<8 | recvdata[datalen-2];
			if(crc_tmp2 == crc_tmp1)
			{
				if(recvdata[8] == LedControl_Cmd)		//开关灯命令字
				{
					if(tcp_server_recvbuf[10] == LEDON)	//LED开关状态更新
					{
						if(tcp_server_recvbuf[11]>99)	//限位
							tcp_server_recvbuf[11] = 99;
//						HAL_GPIO_WritePin(GPIOI, AC_REL_Pin, GPIO_PIN_SET);	//打开继电器电源
						Write_LED_Data(&tcp_server_recvbuf[10],&tcp_server_recvbuf[11]);
//						bsp_WriteCpuFlash(LESTATUS_ADDR, (uint8_t *)&tcp_server_recvbuf[10], 2);
//						Led_Switch_Brightness(tcp_server_recvbuf[11]);	//LED亮度目标百分比调整
#if TCP_Data_Printf
						myprintf("\r\n 开灯 亮度：%d",tcp_server_recvbuf[11]);
#endif
					}
					else if(tcp_server_recvbuf[10] == LEDOFF)
					{
						if(tcp_server_recvbuf[11]>99)	//限位
							tcp_server_recvbuf[11] = 99;
//        	            Led_Switch_Brightness(0);	//LED亮度目标百分比调整
						Write_LED_Data(&tcp_server_recvbuf[10],&tcp_server_recvbuf[11]);
//						HAL_GPIO_WritePin(GPIOI, AC_REL_Pin, GPIO_PIN_RESET);	//关闭继电器电源
//						bsp_WriteCpuFlash(LESTATUS_ADDR, (uint8_t *)&tcp_server_recvbuf[10], 2);
#if TCP_Data_Printf
						myprintf("\r\n 关灯 亮度：%d",tcp_server_recvbuf[11]);
#endif
					}
					socket_SendData(Sn,Device_TYPE,Device_ID,LedControlBack_Cmd,4,&recvdata[10]);	//数据回传
					xSemaphoreGive(LedShowBinary);			//演示模式
				}
			}
		}
	}*/
}

void Create_TCP_connect(void)
{
	struct sockaddr_in server_addr;
	int32_t keepAlive = 1; // 开启keepalive属性
	int32_t keepIdle = 5; // 如该连接在60秒内没有任何数据往来,则进行探测
	int32_t keepInterval = 1; // 探测时发包的时间间隔为5 秒
	int32_t keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
	int32_t keepflag = 1;
	
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
	}
	
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);
socketkeepalive_again:	
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
	}
	
	setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepIdle , sizeof(keepIdle));
	setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepInterval , sizeof(keepInterval));
	setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepCount , sizeof(keepCount));
	setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&keepflag , sizeof(keepflag));
}

int16_t rebuild_TCP_connect(void)
{
	int16_t connect_info;
	struct sockaddr_in server_addr;
	int recvFlag = MSG_DONTWAIT;
	
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
	else if((connect_info < 0) && ((errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)))  // 没收到数据
	{
		static uint8_t first_flag = 0;
		Tcp_Connect_Flag = ConnectOnLine;
		if(first_flag == 0)
		{
			first_flag = 1;
			CheckBackUp_Backup();
		}
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
	return connect_info;
}

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
	_myprintf("\r\n CreatTCPTask");

    Tcp_TaskID = xTaskCreate(tcp_client_thread, "TCP", TCP_StkSIZE, NULL, TCP_TaskPrio, &pvCreatedTask_Tcp);
    if(pdPASS != Tcp_TaskID)
    {
        _myprintf("\r\nTcp_Task creat error");
        while(1);
    }
}
