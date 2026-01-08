#ifndef __SYSCONFIG_H
#define __SYSCONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/****************程序更新记录表*****************
1. 2021/10/19  v1.2改版 驱动修改
2. 2022/02/28  	SW:V1.0.14
				新增LED大灯状态改变电压电流反馈机制 
				新增离线led状态灯 亮1s灭3s
						
3. 2022/04/27	sw:V1.0.15
				优化 lwip socket connet机制，阻塞--->非阻塞
				利用select函数实现对connect时间的把控做到超时机制
4. 2022/05/20	sw:v1.0.16
				新增设备搜索的udp服务器，通过广播形式
				2022/05/24  修复打开udp后导致tcp假性连接问题
5. 2022/08/01	sw:1.0.17
				去除PLL时钟，兼容芯片高温差异
				webserver界面去除背景图片
6. 2022/08/12	sw:1.0.18
				新增水位检测
	2022/08/19	优化电量计耗电量统计
7. 2022/09/05	优化FLASH耗电量的存储方式，降低FLASH擦除次数，延长flash使用寿命
				优化webserver，ID兼容16进制，新增版本号，电压电流等电量计

				优化时间策略00:00上电后等不亮情况
***********************************************/

#define __no_init		__attribute__((section("NO_INIT"),zero_init))

/***************************************
控制灯的个数
***************************************/
#define LED_NUM					(2)
#define CUSTOM_DEV_NUM			(0)			//用户自定义设备

/***************************************
软件版本号：448格式
***************************************/
#define HARDWARE_VERSION		0x2000		//v2.0.0
#define SOFTWARE_VERSION		0x2001		//v2.0.1

/***************************************
选择使用以太网作为通讯方式还是4G模块
4G模块采用luat开发不需要at指令，
直接传输数据，模块会透传
***************************************/
#define USE_ETH_TO_INTERNET		1

#if	USE_ETH_TO_INTERNET == 0
	#define USE_4G_UART_TO_INTERNET		1
#else
	#define USE_4G_UART_TO_INTERNET		0
#endif

/***************************************
选择使用十合一气象仪作为环境监测还是多个传感器拼接
***************************************/
#define USE_TEN_IN_ONE_DEVICE			1

#if	USE_TEN_IN_ONE_DEVICE == 0
	#define USE_MULTIPLE_DEVICE		1
#else
	#define USE_MULTIPLE_DEVICE		0
#endif

/***************************************
系统任务规划配置
TaskPrio 任务优先级，值越大，优先级越高  范围 0 ~ configMAX_PRIORITIES
StkSIZE	任务栈空间

**************************************/
#define Start_TaskPrio     				( tskIDLE_PRIORITY + 1 )
#define	Start_StkSIZE					( 256 )

#define TCP_TaskPrio     				( tskIDLE_PRIORITY + 3 )
#define	TCP_StkSIZE						( 400 )

//#define GPRS_TaskPrio     				( tskIDLE_PRIORITY + 4 )
//#define	GPRS_StkSIZE					( 400 )

#define Uart_TaskPrio     				( tskIDLE_PRIORITY + 7 )
#define	Uart_StkSIZE					( 256 )

#define Environment_Data_TaskPrio     	( tskIDLE_PRIORITY + 10 )
#define	Environment_Data_StkSIZE		( 256 )

#define Platform_TaskPrio				( tskIDLE_PRIORITY + 4 )
#define Platform_StkSIZE				( 400 )

#define Led_TaskPrio     				( tskIDLE_PRIORITY + 6 )
#define	Led_StkSIZE						( 300 )

#define RemoteUpdata_TaskPrio			( tskIDLE_PRIORITY + 9 )
#define RemoteUpdata_StkSIZE			( 400 )

#define DHCP_TaskPrio					( tskIDLE_PRIORITY + 8 )
#define DHCP_StkSIZE					( 400 )

/*************************************
队列的长度规划
*************************************/
#define TCP_SEND_QUEUE_LENGTH		( 5 )
#define GPRS_UART_SEND_QUEUE_LENGTH	( 2 )
#define GPRS_RECEIVE_QUEUE_LENGTH	( 2 )
#define GPRS_QUEUE_LENGTH			( 2 )
#define RemoteUpdata_QUEUE_LENGTH	( 2 )
#define ElectricData_QUEUE_LENGTH	( 2 )
#define ElectricData_Size			( 128 )


extern SemaphoreHandle_t PrintMutex; //myprintf
extern SemaphoreHandle_t LedShowBinary;	//演示模式信号量
extern SemaphoreHandle_t TimeControlMutex; //TimeControlMutex
extern SemaphoreHandle_t EnvironmentMutex; //EnvironmentMutex
extern SemaphoreHandle_t ElectricDataMutex; //ElectricDataMutex

extern SemaphoreHandle_t TcpMutex; 
extern SemaphoreHandle_t RemoteUpdataMutex;

extern SemaphoreHandle_t AlarmBinary;	//报警信号量
extern SemaphoreHandle_t EleUpBinary;	//数据更新信号量

extern QueueHandle_t TCP_SEND_Queue;
extern QueueHandle_t GPRS_UART_SEND_Queue;
extern QueueHandle_t GPRS_RECEIVE_Queue;
extern QueueHandle_t RemoteUpdata_Queue;
extern QueueHandle_t BL0942_RECEIVE_Queue;

#endif
