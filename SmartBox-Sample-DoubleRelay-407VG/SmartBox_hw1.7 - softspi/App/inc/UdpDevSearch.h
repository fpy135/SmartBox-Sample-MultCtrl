#ifndef __UDPDEVSEARCH_H
#define __UDPDEVSEARCH_H

#include "main.h"

#define DEVSEARCH_PORT		(10022)

extern int UdpDevSearch_Server_Init(void);
extern void UdpDevSearch_Send(void);
extern void UdpDevSearch_Recv(void);

#endif
