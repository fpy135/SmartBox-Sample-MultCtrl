#include "IDConfig.h"
#include "SysConfig.h"
#include "mystring.h"
#include "socketserver.h"
#include "MyFlash.h"
#include "Protocol.h"
#include "usart.h"
#include "Led_App.h"
#include "StorageConfig.h"

uint16_t Device_TYPE = 0;	//产品ID或者产品类型，设备类型
uint32_t Device_ID = 0;		//设备ID
uint8_t Sn = 0;				//数据包流水号
uint8_t IDStatus = 0;		//ID预制状态

/**************************************************
功能：生产预制ID
输入参数：buf预制数据    len 长度
**************************************************/
void ReadID(void)
{
	TimeControl_Type timecontrol_data;
	
	bsp_ReadCpuFlash (IDSTATUS_ADDR, (uint8_t *)&IDStatus, 1);
	bsp_ReadCpuFlash (LEDSTATUS_ADDR, (uint8_t *)&LedStatus, 1);
	bsp_ReadCpuFlash (LEDSTATUS_ADDR+1, (uint8_t *)&LedPwm, 1);
	bsp_ReadCpuFlash (TIMECONTROL_ADDR, (uint8_t *)&timecontrol_data,sizeof(timecontrol_data));
	_myprintf("\r\nLedStatus:%d,LedPwm:%d",LedStatus,LedPwm);
	if(IDStatus == 1)		//ID预制过后读出ID和产品类型
	{
		bsp_ReadCpuFlash (DEVICETYPE_ADDR, (uint8_t *)&Device_TYPE, 2);
		bsp_ReadCpuFlash (DEVICEID_ADDR, (uint8_t *)&Device_ID, 4);
	}
	else
	{
		IDStatus = 0;
		Device_TYPE = 0x1A01;
		Device_ID = 0;
	}
	if(LedStatus == 0xff || LedPwm>100)
	{
		LedStatus = 0;
		LedPwm = 0;
	}
	if(timecontrol_data.start_hour >= 24 || timecontrol_data.start_min >= 60)
	{
		//默认模式
		timecontrol_data.start_hour = 18;
		timecontrol_data.start_min = 0;
		timecontrol_data.phase1_time = 60*6;
		timecontrol_data.phase1_Pwm = 99;
		timecontrol_data.phase2_time = 60*6;
		timecontrol_data.phase2_Pwm = 50;
		timecontrol_data.phase3_time = 0;
		timecontrol_data.phase3_Pwm = 0;
		timecontrol_data.phase4_time = 0;
		timecontrol_data.phase4_Pwm = 0;
	}
	Write_TimeControl_Data(&timecontrol_data);
	_myprintf("\r\n产品类型:0x%2X,设备ID:%d",Device_TYPE,Device_ID);
	_myprintf("\r\n时间控制策略:开始时间:%02d:%02d,阶段一:time:%d  pwm:%d  阶段二:time:%d  pwm:%d  阶段三:time:%d  pwm:%d  阶段四:time:%d  pwm:%d\r\n",\
	timecontrol_data.start_hour,timecontrol_data.start_min,timecontrol_data.phase1_time,timecontrol_data.phase1_Pwm,\
	timecontrol_data.phase2_time,timecontrol_data.phase2_Pwm,timecontrol_data.phase3_time,timecontrol_data.phase3_Pwm,\
	timecontrol_data.phase4_time,timecontrol_data.phase4_Pwm);
}

/**************************************************
功能：生产预制ID
输入参数：buf预制数据    len 长度
**************************************************/
void IDCofing(uint8_t *buf, uint16_t len)
{
	 if(buf[0] == 0xAA)
    {
        uint16_t crc_tmp1,crc_tmp2;

        crc_tmp2 = CRC16_calc(buf,len-2);
        crc_tmp1 = buf[len-1]<<8 | buf[len-2];
        if(crc_tmp1 == crc_tmp2)
        {
            if(buf[8] == ReadID_Cmd)		//查询ID
            {
                uint8_t *pbuf;

				bsp_ReadCpuFlash (DEVICETYPE_ADDR, (uint8_t *)&Device_TYPE, 2);
				bsp_ReadCpuFlash (DEVICEID_ADDR, (uint8_t *)&Device_ID, 4);
                pbuf = Protocol_Pack(0,Device_TYPE,Device_ID,ReadID_Cmd,0,(uint8_t *)buf);
                PrintWrite(pbuf, len);		//返回的长度和写入长度一致

            }
            else if(buf[8] == WriteID_Cmd)		//预制ID
            {
				if(IDStatus == 0)				//仅未预制ID的情况下可以预制ID
				{
					uint8_t *pbuf;

					IDStatus = 1;
					Device_TYPE = ((buf[2]<<8) | buf[3]);
					Device_ID = (buf[4]<<24 | buf[5]<<16 | buf[6]<<8 | buf[7]);
					Write_UserMem_Flash(IDSTATUS_ADDR,(uint8_t *)&IDStatus,1);
					bsp_ReadCpuFlash (IDSTATUS_ADDR, (uint8_t *)&IDStatus, 1);
					Write_UserMem_Flash(DEVICETYPE_ADDR,(uint8_t *)&Device_TYPE,2);
					bsp_ReadCpuFlash (DEVICETYPE_ADDR, (uint8_t *)&Device_TYPE, 2);
					Write_UserMem_Flash(DEVICEID_ADDR,(uint8_t *)&Device_ID,4);
					bsp_ReadCpuFlash (DEVICEID_ADDR, (uint8_t *)&Device_ID, 4);
					pbuf = Protocol_Pack(0,Device_TYPE,Device_ID,WriteID_Cmd,0,(uint8_t *)buf);
					PrintWrite(pbuf, len);		//返回的长度和写入长度一致
				}
				else
					return;
            }
        }
    }
}
