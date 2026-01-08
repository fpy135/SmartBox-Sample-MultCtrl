#ifndef __PLATFORM_H
#define __PLATFORM_H

#include "main.h"
#include "SysConfig.h"

#define Platform_Data_Printf				1

#if USE_ETH_TO_INTERNET
#define PLATFORM_BUFF_SIZE			( TCP_BUFF_SIZE )
#endif

#if USE_4G_UART_TO_INTERNET
#define PLATFORM_BUFF_SIZE			( Uart8RxBufferSize )
#endif
#define GPRS_SendData				UART1Write

#define Herat_Rate					(60)		//单位：秒
//#define Herat_Rate					(5)		//单位：秒


#pragma pack(push,1)
 
typedef struct devLoraInfo
{
	unsigned char	com:1;
	unsigned char	network:1;
} devLoraInfo;

typedef struct devNBInfo
{
	unsigned char	com:1;
	unsigned char	sim:1;
	unsigned char	network:1;
} devNBInfo;

typedef struct dev4GInfo
{
	unsigned char	com:1;
	unsigned char	sim:1;
	unsigned char	network:1;
} dev4GInfo;

typedef struct devEthInfo
{
	unsigned char	com:1;
	unsigned char	link:1;
	unsigned char	network:1;
} devEthInfo;

typedef struct devElectricInfo
{
	unsigned char	com:1;
	unsigned char	overVol:1;
	unsigned char	overCurr:1;
	unsigned char	ledError:1;
} devElectricInfo;
#pragma pack(pop)

#define LORA_DEV 	00	//no use 00 use 01
#define NB_DEV 		00	//no use 00 use 02
#define GPRS4G_DEV	00	//no use 00 use 03
#define ETH_DEV 	04	//no use 00 use 04
#define ELECTRIC1_DEV 	05	//no use 00 use 05
#define ELECTRIC2_DEV 	06	//no use 00 use 06

#define DEVNUM			3	//设备总数

typedef struct AlarmData {
	unsigned char 	devNum;
#if LORA_DEV
	unsigned char  	LoraNum;
	unsigned char 	devLora;
#endif
#if NB_DEV
	unsigned char  	NBNum;
	unsigned char	devNB;
#endif
#if GPRS4G_DEV
	unsigned char  	GPRS4GNum;
	unsigned char	dev4G;
#endif
#if ETH_DEV
	unsigned char  	EthNum;
	unsigned char	devEth;
#endif
#if ELECTRIC1_DEV
	unsigned char  	Electric1Num;
	unsigned char	devElectric1;
#endif
#if ELECTRIC2_DEV
	unsigned char  	Electric2Num;
	unsigned char	devElectric2;
#endif
}AlarmData_Type;

extern AlarmData_Type alarmData;

extern uint8_t MSG_GPRSReceiveDataFromISR(uint8_t *buf, uint16_t len);
extern void Platform_SendDataByGPRS(uint8_t sn,uint16_t type, uint32_t id, uint8_t cmd,uint16_t len,uint8_t *senddata);
extern void Platform_Data_Process(uint8_t* msgdata);
extern void CreatPlatformTask(void);
extern void Herat_Data_Send(void);
#endif
