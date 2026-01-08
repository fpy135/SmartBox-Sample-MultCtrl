#include "IDConfig.h"
#include "SysConfig.h"
#include "mystring.h"
#include "socketserver.h"
#include "MyFlash.h"
#include "Protocol.h"
#include "usart.h"
#include "Led_App.h"
#include "StorageConfig.h"
#include "BL0942.h"
#include "w25qxx.h"
#include "Environment_data.h"

/********初始化不为0变量***********/
uint32_t NoInitFlag __no_init;

uint16_t Device_TYPE = 0;	//产品ID或者产品类型，设备类型
uint32_t Device_ID = 0;		//设备ID
uint8_t Sn = 0;				//数据包流水号
uint8_t IDStatus = 0;		//ID预制状态
WEBSERVER_LOGIN_INFO login_info;
/**************************************************
功能：生产预制ID
输入参数：buf预制数据    len 长度
**************************************************/
void ReadID(void)
{
	uint8_t i = 0;
	uint32_t *tmp_v;
	uint32_t *tmp_i;
	TimeControl_Type timecontrol_data;
	#if CUSTOM_DEV_NUM
	TimeCustomControl_Type timecustomctrl_data;
	#endif

	
	bsp_ReadCpuFlash (IDSTATUS_ADDR, (uint8_t *)&IDStatus, 1);
//	bsp_ReadCpuFlash (LEDSTATUS_ADDR, (uint8_t *)&LedStatus, 1);
//	bsp_ReadCpuFlash (LEDSTATUS_ADDR+1, (uint8_t *)&LedPwm, 1);
	
	/********************网络参数相关数据************************/
	bsp_ReadCpuFlash (DeviceIP_ADDR, (uint8_t *)&DeviceIP, 4);
	bsp_ReadCpuFlash (DeviceNetmask_ADDR, (uint8_t *)&DeviceNetmask, 4);
	bsp_ReadCpuFlash (DeviceGw_ADDR, (uint8_t *)&DeviceGw, 4);
	bsp_ReadCpuFlash (ServerIP_ADDR, (uint8_t *)&TCPServerIP, 4);
	bsp_ReadCpuFlash (ServerPort_ADDR, (uint8_t *)&TCPServerPort, 2);
	bsp_ReadCpuFlash (DHCP_Enable_ADDR, (uint8_t *)&DHCP_Enable_Flag, 1);
	bsp_ReadCpuFlash (NTPServerIP_ADDR, (uint8_t *)&NTPServerIP, 4);
	bsp_ReadCpuFlash (WebUsername_ADDR, (uint8_t *)login_info.username, 10);
	bsp_ReadCpuFlash (WebPassword_ADDR, (uint8_t *)login_info.password, 10);
	if(login_info.username[0] == 0xff)										//web页面管理账号
	{
		StrCopy((uint8_t *)&login_info.username ,(uint8_t *)"admin", sizeof("admin"));
	}
	if(login_info.password[0] == 0xff)
	{
		StrCopy((uint8_t *)&login_info.password ,(uint8_t *)"123456", sizeof("123456"));
	}
	
	/********************电量计参数相关数据*************************/
	#ifdef ELE_TRIM
	bsp_ReadCpuFlash(ENERGY_PARA_ADDR, (uint8_t *)&U_k, 4);
	bsp_ReadCpuFlash(ENERGY_PARA_ADDR+4,(uint8_t *)&I_k, 4);
	bsp_ReadCpuFlash(ENERGY_PARA_ADDR+8,(uint8_t *)&Watt_k, 4);
	bsp_ReadCpuFlash(ENERGY_PARA_ADDR+12,(uint8_t *)&U2_k, 4);
	bsp_ReadCpuFlash(ENERGY_PARA_ADDR+12+4,(uint8_t *)&I2_k, 4);
	bsp_ReadCpuFlash(ENERGY_PARA_ADDR+12+8,(uint8_t *)&Watt2_k, 4);	
	
	bsp_ReadCpuFlash(ENERGY_REF_ADDR,(uint8_t *)&VREF.c, 4);		//参考电压,参考电流
	bsp_ReadCpuFlash(ENERGY_REF_ADDR+4,(uint8_t *)&IREF.c, 4);
	
	tmp_v = (uint32_t *)&VREF.c;
	tmp_i = (uint32_t *)&IREF.c;
	if(*tmp_v == 0xffffffff || *tmp_i == 0xffffffff)
	{
		VREF.f = CAL_VREF_DEF;		//校准默认电压
		IREF.f = CAL_IREF_DEF;		//校准默认电流
	}
	#endif
	bsp_ReadCpuFlash(SENSOR_485_USE_ADDR,(uint8_t *)&SENSOR_485_USE.byte_use, 4);		//485传感器挂载设备
	W25QXX_Read((uint8_t *)&SaveEnergy, SaveEnergy_By_Flash_Init(), 8);			//能量
	
	if(SaveEnergy[0] == 0xffffffff)
	{
		SaveEnergy[0] = 0;
	}
	if(SaveEnergy[1] == 0xffffffff)
	{
		SaveEnergy[1] = 0;
	}
	myprintf("\r\nSaveEnergy1:%d SaveEnergy2:%d",SaveEnergy[0],SaveEnergy[1]);
	
	if(IDStatus == 1)		//ID预制过后读出ID和产品类型
	{
		bsp_ReadCpuFlash (DEVICETYPE_ADDR, (uint8_t *)&Device_TYPE, 2);
		bsp_ReadCpuFlash (DEVICEID_ADDR, (uint8_t *)&Device_ID, 4);
	}
	else
	{
		IDStatus = 0;
		Device_TYPE = 0x1A04;
		Device_ID = 0;
	}
	if(NoInitFlag == 0x1234abcd)		//软复位
	{
		myprintf("\r\n软复位!!!");
		for(i=0; i<LED_NUM; i++)
		{
			LedPwm[i] = LedPwm[i];
			LedStatus[i] = LedStatus[i];
		}
        ledshowFlag = ledshowFlag;
		
		#if CUSTOM_DEV_NUM
		CustomStatus1 = CustomStatus1;
		CustomStatus2 = CustomStatus2;
		#endif
	}
	else
	{
		myprintf("\r\n上电复位!!!");
		NoInitFlag = 0x1234abcd;
		for(i=0; i<LED_NUM; i++)
		{
			LedPwm[i] = 0;
			LedStatus[i] = LEDOFF;
		}
        ledshowFlag = 0;
		
		#if CUSTOM_DEV_NUM
		CustomStatus1 = 0;
		CustomStatus2 = 0;
		#endif
	}
	myprintf("\r\n产品类型:0x%2X,设备ID:%d,ID状态:%d",Device_TYPE,Device_ID,IDStatus);

	for(i=0; i<LED_NUM; i++)
	{
		if(LedStatus[i] == 0xff || LedPwm[i]>100)
		{
			LedPwm[i] = 0;
			LedStatus[i] = 0;
		}
		REL_Write(i+1, LedStatus[i]);
		myprintf("\r\nLedStatus%d:%d,LedPwm:%d",\
				 i+1,LedStatus[i],LedPwm[i]);
	}
	
	if(SENSOR_485_USE.byte_use == 0xffffffff)
	{
		//默认使用温湿度，倾角，智能空开,模拟井盖 
		SENSOR_485_USE.byte_use = 0;
		SENSOR_485_USE.bit_use.WS301 = 1;
		SENSOR_485_USE.bit_use.MTC70M = 1;
		SENSOR_485_USE.bit_use.IOTB2_16 = 1;
		SENSOR_485_USE.bit_use.ManholeCover_MTC70M = 1;
	}
	Sensor_485_Use_Info_Print(SENSOR_485_USE);

	#if CUSTOM_DEV_NUM
	REL3_Write(!CustomStatus1);
	REL4_Write(!CustomStatus2);
	myprintf("CustomStatus1:%d,CustomStatus2:%d\r\n",CustomStatus1,CustomStatus2);
	#endif
	
	if(DeviceIP == 0xffffffff || DeviceGw == 0xffffffff || TCPServerIP == 0xffffffff || TCPServerPort == 0xffff || DHCP_Enable_Flag == 0xff || NTPServerIP == 0XFFFFFFFF)
	{
		uint8_t * tmp;
		
		tmp = (uint8_t *)&DeviceIP;
		tmp[0] = 192;
		tmp[1] = 168;
		tmp[2] = 1;
		tmp[3] = 105;
		
		tmp = (uint8_t *)&DeviceNetmask;
		tmp[0] = 255;
		tmp[1] = 255;
		tmp[2] = 255;
		tmp[3] = 0;
		
		tmp = (uint8_t *)&DeviceGw;
		tmp[0] = 192;
		tmp[1] = 168;
		tmp[2] = 1;
		tmp[3] = 1;
		
		tmp = (uint8_t *)&TCPServerIP;
		tmp[0] = 121;
		tmp[1] = 199;
		tmp[2] = 69;
		tmp[3] = 67;
		
		tmp = (uint8_t *)&TCPServerPort;
		tmp[0] = (9001>>8)&0xff;
		tmp[1] = (9001)&0xff;
		
		DHCP_Enable_Flag = 0x00;
		
		tmp = (uint8_t *)&NTPServerIP;
		tmp[0] = 120;
		tmp[1] = 25;
		tmp[2] = 115;
		tmp[3] = 20;
	}
	/* TCP本地调试代码，正式版本需注释或删除本行代码 */
//	DeviceIP = 192 | 168<<8 | 1 <<16 | 200<<24;
//	TCPServerIP = 192 | 168<<8 | 1 <<16 | 8<<24;
	
	myprintf("\r\n\r\n设备IP:%d.%d.%d.%d", (DeviceIP&0x000000ff),((DeviceIP&0x0000ff00)>>8),((DeviceIP&0x00ff0000)>>16),(DeviceIP&0xff000000)>>24);
	myprintf("\r\n子网掩码:%d.%d.%d.%d",(DeviceNetmask&0x000000ff),((DeviceNetmask&0x0000ff00)>>8),((DeviceNetmask&0x00ff0000)>>16),(DeviceNetmask&0xff000000)>>24);
	myprintf("\r\n默认网关:%d.%d.%d.%d",(DeviceGw&0x000000ff),((DeviceGw&0x0000ff00)>>8),((DeviceGw&0x00ff0000)>>16),(DeviceGw&0xff000000)>>24);
	
	myprintf("\r\n服务器IP:%d.%d.%d.%d", (TCPServerIP&0x000000ff),((TCPServerIP&0x0000ff00)>>8),((TCPServerIP&0x00ff0000)>>16),(TCPServerIP&0xff000000)>>24);
	myprintf("\r\n服务器Port:%d",HALFWORD_Reverse(TCPServerPort));
	myprintf("\r\nDHCP使能:%d",DHCP_Enable_Flag);
	myprintf("\r\nNTP服务器IP:%d.%d.%d.%d\r\n", (NTPServerIP&0x000000ff),((NTPServerIP&0x0000ff00)>>8),((NTPServerIP&0x00ff0000)>>16),(NTPServerIP&0xff000000)>>24);
	
	
	for(i=0; i<LED_NUM; i++)
	{
		bsp_ReadCpuFlash (TIMECONTROL_ADDR+sizeof(timecontrol_data)*i, (uint8_t *)&timecontrol_data,sizeof(timecontrol_data));
		if(timecontrol_data.start_hour >= 24 || timecontrol_data.start_min >= 60)
		{
			//默认模式
			timecontrol_data.start_hour = 18;
			timecontrol_data.start_min = 0;
			timecontrol_data.phase1_time = 60*6;
			timecontrol_data.phase1_Pwm = 100;
			timecontrol_data.phase2_time = 60*6;
			timecontrol_data.phase2_Pwm = 50;
			timecontrol_data.phase3_time = 0;
			timecontrol_data.phase3_Pwm = 0;
			timecontrol_data.phase4_time = 0;
			timecontrol_data.phase4_Pwm = 0;
		}
		if(timecontrol_data.phase1_Pwm > 100)
			timecontrol_data.phase1_Pwm = 100;
		if(timecontrol_data.phase2_Pwm > 100)
			timecontrol_data.phase2_Pwm = 100;
		if(timecontrol_data.phase3_Pwm > 100)
			timecontrol_data.phase3_Pwm = 100;
		if(timecontrol_data.phase4_Pwm > 100)
			timecontrol_data.phase4_Pwm = 100;
		Write_TimeControl_Data(i+1,&timecontrol_data);
		myprintf("\r\n灯头%d 时间控制策略:开始时间:%02d:%02d,阶段一:time:%d  pwm:%d  阶段二:time:%d  pwm:%d  阶段三:time:%d  pwm:%d  阶段四:time:%d  pwm:%d\r\n",\
		i+1,timecontrol_data.start_hour,timecontrol_data.start_min,timecontrol_data.phase1_time,timecontrol_data.phase1_Pwm,\
		timecontrol_data.phase2_time,timecontrol_data.phase2_Pwm,timecontrol_data.phase3_time,timecontrol_data.phase3_Pwm,\
		timecontrol_data.phase4_time,timecontrol_data.phase4_Pwm);
	}
	#if CUSTOM_DEV_NUM
	for(i=0; i<CUSTOM_DEV_NUM; i++)
	{
		bsp_ReadCpuFlash (TIMECTRCUSTOM_ADDR+sizeof(timecustomctrl_data)*i, (uint8_t *)&timecustomctrl_data,sizeof(timecustomctrl_data));
		if(timecustomctrl_data.start_hour >= 24 || timecustomctrl_data.start_min >= 60)
		{
			//默认模式
			timecustomctrl_data.start_hour = 18;
			timecustomctrl_data.start_min = 0;
			timecustomctrl_data.phase1_time = 0;
			timecustomctrl_data.led_sta1 = 0;
			timecustomctrl_data.phase2_time = 0;
			timecustomctrl_data.led_sta2 = 0;
			timecustomctrl_data.phase3_time = 0;
			timecustomctrl_data.led_sta3 = 0;
			timecustomctrl_data.phase4_time = 0;
			timecustomctrl_data.led_sta4 = 0;
		}
		if(timecustomctrl_data.led_sta1 > 2)		//只有0 和 1 是有效值，除外都当作0关灯
			timecustomctrl_data.led_sta1 = 1;
		if(timecustomctrl_data.led_sta2 > 2)
			timecustomctrl_data.led_sta2 = 1;
		if(timecustomctrl_data.led_sta3 > 2)
			timecustomctrl_data.led_sta3 = 1;
		if(timecustomctrl_data.led_sta4 > 2)
			timecustomctrl_data.led_sta4 = 1;
		Write_TimeCustomCtrl_Data(i+1,&timecustomctrl_data);
		myprintf("\r\n用户自定义设备%d 时间控制策略:开始时间:%02d:%02d,阶段一:time:%d  sta:%d  阶段二:time:%d  sta:%d  阶段三:time:%d  sta:%d  阶段四:time:%d  sta:%d\r\n",\
		i+1,timecustomctrl_data.start_hour,timecustomctrl_data.start_min,timecustomctrl_data.phase1_time,timecustomctrl_data.led_sta1,\
		timecustomctrl_data.phase2_time,timecustomctrl_data.led_sta2,timecustomctrl_data.phase3_time,timecustomctrl_data.led_sta3,\
		timecustomctrl_data.phase4_time,timecustomctrl_data.led_sta4);
	}
	#endif
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
				uint16_t devtype;
				uint32_t devid;

				bsp_ReadCpuFlash (DEVICETYPE_ADDR, (uint8_t *)&devtype, 2);
				bsp_ReadCpuFlash (DEVICEID_ADDR, (uint8_t *)&devid, 4);
				devtype = Device_TYPE;
                pbuf = Protocol_Pack(0,devtype,devid,ReadID_Cmd,0,(uint8_t *)buf);
                PrintWrite(pbuf, len);		//返回的长度和写入长度一致
            }
            else if(buf[8] == WriteID_Cmd)		//预制ID
            {
				if(IDStatus == 0)				//仅未预制ID的情况下可以预制ID
				{
					uint8_t *pbuf;

					IDStatus = 1;
//					Device_TYPE = ((buf[2]<<8) | buf[3]);
					Device_TYPE = 0x1A04;
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
