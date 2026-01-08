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

#define Herat_Rate					5		//µ•Œª£∫√Î

extern uint8_t MSG_GPRSReceiveDataFromISR(uint8_t *buf, uint16_t len);
extern void Platform_SendDataByGPRS(uint8_t sn,uint16_t type, uint32_t id, uint8_t cmd,uint16_t len,uint8_t *senddata);
extern void Platform_Data_Process(uint8_t* msgdata);
extern void CreatPlatformTask(void);
extern void Herat_Data_Send(void);
#endif
