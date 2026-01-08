#ifndef __REMOTEUPDATA_H
#define __REMOTEUPDATA_H

#include "main.h"

#define RemoteUpdataTimeOut			5000	//5000ms³¬Ê±

extern void RemoteUpdata_ACK(void);
extern uint8_t RemoteUpdata(uint8_t *buf, uint16_t len);
extern void Get_File_inform(uint8_t *buf);
extern uint8_t RemoteUpDate(uint8_t *buff);
extern void CreatRemoteUpdataTask(void);

#endif
