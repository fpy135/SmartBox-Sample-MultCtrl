#ifndef __GPRS_AIR724_H
#define __GPRS_AIR724_H

#include "stdint.h"

#define GPRSPrint		1		//GPRS打印

#define GPRSSend		UART7Write

#define GPRSBuf			Uart7RxBuffer
#define GPRSCount		Uart7_Rx_Cnt
#define GPRSBuf_Size	Uart7RxBufferSize

#define SignalMaxValue	31
#define SignalLowValue	10	//GPRS信号少于10时，不去连接服务器

#define ReConfigTime	5		//重置次数

extern uint8_t MSG_GPRSReceiveDataFromISR(uint8_t *buf, uint16_t len);
extern void GPRSDataPro(void);
extern void CreatGRRSTask(void);

#endif
