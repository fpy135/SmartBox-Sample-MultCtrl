#ifndef __REMOTEUPDATA_H
#define __REMOTEUPDATA_H

#include "main.h"

#define RemoteUpdataTimeOut			50000	//5000ms³¬Ê±

extern uint32_t RemoteUpdata_tick;

extern void RemoteUpdata_ACK(void);
extern uint8_t RemoteUpdataBy4G(uint8_t *buf, uint16_t len);
extern void Get_File_inform(uint8_t *buf);
extern uint8_t RemoteUpDataByEth(uint8_t *buff);
extern void CreatRemoteUpdataTask(void);

#endif
