#ifndef __SOCKETSERVER_H
#define __SOCKETSERVER_H

#include "main.h"

#define TCP_Data_Printf			1

#define TIME_TIRMING_DNS	"ntp1.aliyun.com"
//#define TIME_TIRMING_DNS	"www.baidu.com"
//#define SERVER_IP		"192.168.1.16"
//#define SERVER_IP		"172.16.10.58"
#define SERVER_IP		"47.98.223.38"
#define	SERVER_PORT		9000				//配置服务器端口号，最好设为2000以后，之前的端口部分在其它通信协议中被默认使用

#define TCP_BUFF_SIZE			( 512 )

typedef enum{
	ConnectOnLine		= 0x00,
	ConnectOffLine		= 0x01,
}Socket_Connect_Type;


extern void tcp_server_init(void);	//TCP服务器初始化
extern void CreatTCPTask(void);	//TCP客户端初始化
extern void socket_SendData(uint8_t sn,uint16_t type, uint32_t id, uint8_t cmd,uint16_t len,uint8_t *senddata);
extern void RemoteUpdata_ack(uint8_t sn, uint16_t type, uint32_t id, uint8_t cmd,uint16_t len, uint8_t *content);
extern void ntpTime(void);
extern void Create_TCP_connect(void);
extern int16_t rebuild_TCP_connect(void);

#endif

