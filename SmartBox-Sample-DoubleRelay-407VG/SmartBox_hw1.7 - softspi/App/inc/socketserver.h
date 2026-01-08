#ifndef __SOCKETSERVER_H
#define __SOCKETSERVER_H

#include "main.h"

#define TCP_Data_Printf			1

#define TIME_TIRMING_DNS	"ntp1.aliyun.com"
//#define SERVER_IP		"192.168.1.16"
//#define SERVER_IP		"47.98.223.38"
#define SERVER_IP		"121.199.69.67"
#define	SERVER_PORT		9001				//配置服务器端口号，最好设为2000以后，之前的端口部分在其它通信协议中被默认使用

#define TCP_BUFF_SIZE			( 512 )

#define LOCAL_NTP_SERVER	1

#if LOCAL_NTP_SERVER
//	#define Local_ntp_ip	"10.10.1.20"
	#define Local_ntp_ip	"120.25.115.20"
#endif

#define LI 0
#define VN 3
#define MODE 3
#define STRATUM 0
#define POLL 4 
#define PREC -6

typedef enum{
	ConnectOffLine		= 0x00,
	ConnectOnLine		= 0x01,
}Socket_Connect_Type;

typedef struct {
	uint32_t refresh_tick;		//重新连接时间
	uint32_t connect_cnt;		//长时间没收到数据时间
	uint8_t socket_flag;		//socket连接成功标志
	uint32_t socket_reboot_tick;//长时间未连接后系统复位时间
	uint32_t rebuild_interval;	//socekt重连间隔时间
}ReConnect_By_Socekt;

extern uint32_t DeviceIP;
extern uint32_t DeviceNetmask;
extern uint32_t DeviceGw;
extern uint32_t TCPServerIP;
extern uint32_t NTPServerIP;
extern uint16_t TCPServerPort;
extern uint8_t DHCP_Enable_Flag;
extern uint8_t Tcp_Connect_Flag;
extern uint8_t socket_reconnet_flag;
extern uint8_t sockfd_err_flag;

extern ReConnect_By_Socekt tcp_reconnect;


extern void TcpConnectFlag_RW(uint8_t rw,uint8_t *flag);

extern void tcp_server_init(void);	//TCP服务器初始化
extern void CreatTCPTask(void);	//TCP客户端初始化
extern void socket_SendData(uint8_t sn,uint16_t type, uint32_t id, uint8_t cmd,uint16_t len,uint8_t *senddata);
extern void RemoteUpdata_ack(uint8_t sn, uint16_t type, uint32_t id, uint8_t cmd,uint16_t len, uint8_t *content);
extern int8_t ntpTime(void);
extern int8_t Create_TCP_connect(void);
extern int16_t rebuild_TCP_connect(void);
extern int8_t Create_TCP_Connect_TimeOut(void);

#endif

