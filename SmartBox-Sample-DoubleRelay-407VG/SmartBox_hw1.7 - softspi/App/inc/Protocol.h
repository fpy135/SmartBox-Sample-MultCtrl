#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "main.h"

//#define HALFWORD_Reverse( x )			((((x)&0x00ff)<<8) | (((x)&0xff00)>>8))
//#define WORD_Reverse(x) 				((((x)&0xff)<<24)|(((x)&0xff00)<<8)|(((x)&0xff0000)>>8)|(((x)&0xff000000)>>24)) 


#pragma pack(push,1)
typedef struct
{
    uint8_t  head;
    uint8_t  sn;
    uint8_t  id[4];
    uint8_t  cmd;
	uint8_t	 len;
    uint8_t	 *content;
    uint16_t crc;
} Platform_Protocol;    /*通用协议*/

#pragma pack(pop)

typedef enum{
	ReadID_Cmd			= 0x00,
	WriteID_Cmd			= 0xff,
	Heart_Cmd 			= 0x01,
	LedControl_Cmd		= 0xA1,
	LedControlBack_Cmd	= 0xB1,
	TimeControl_Cmd		= 0xA2,
	TimeControlBack_Cmd	= 0xB2,
	FindTimeCtr_Cmd		= 0xA3,
	FindTimeCtrBack_Cmd	= 0xB3,
	FindElectric_Cmd	= 0xA4,
	FindEleBack_Cmd		= 0xB4,
	FindEnvironment_Cmd	= 0xA5,
	FindEnvironBack_Cmd	= 0xB5,
	
	Alarm_Cmd			= 0xAA,
	AlarmBack_Cmd		= 0xBA,
	SwitchData_Cmd		= 0xAB,
	SwitchDataBack_Cmd	= 0xBB,
	
	AngleData_Cmd		= 0xAC,
	
	AirSwitchCtrl_Cmd	= 0xAD,
	AirSwitchCtrlBack_Cmd	= 0xBD,
	AirSwitchData_Cmd	= 0xAE,
	
	CustomCtrl_Cmd		= 0xA6,
	CustomCtrlBack_Cmd	= 0xB6,
	TimeCustomCtrl_Cmd	= 0xA7,
	TimeCustomCtrlBack_Cmd	= 0xB7,
	FindTimeCustomCtr_Cmd	= 0xA8,
	FindTimeCustomCtrBack_Cmd	= 0xB8,
	
	FindLedSta_Cmd		= 0xA9,
	FindLedStaBack_Cmd	= 0xB9,
	
	RemoteUpdata_Cmd	= 0xAf,
	RemoteUpdataBack_Cmd= 0xBf,
	
	ACK_Cmd				= 0x90,
	
	EnvAddData_Cmd		= 0x81,
	ManholeCover_angle_Cmd	= 0x82,
	SetServerAddr_Cmd	= 0x8D,
	SetServerAddrBack_Cmd	= 0x9D,
}ProtocolCmd_Type;

extern uint16_t CRC16_Calc(uint8_t *tmpbuf, uint8_t len);
extern uint8_t *RF_Pack(uint8_t sn, uint8_t *id, uint8_t cmd, uint8_t *content);
extern uint8_t *Protocol_Pack(uint8_t sn, uint16_t type, uint32_t id, uint8_t cmd,uint16_t len, uint8_t *content);
extern uint8_t *Protocol_Pack_BB(uint8_t sn, uint16_t type, uint32_t id, uint8_t cmd,uint16_t len, uint8_t *content);
extern uint8_t *Protocol_Pack_2(uint8_t sn, uint16_t type, uint32_t id, uint8_t cmd,uint16_t len, uint8_t *content);

#endif
