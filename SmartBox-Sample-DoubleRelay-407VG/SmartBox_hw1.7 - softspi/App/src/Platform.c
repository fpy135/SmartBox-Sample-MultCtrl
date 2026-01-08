#include "Platform.h"
#include "SysConfig.h"
#include "rtc.h"
#include "iwdg.h"
#include "Usart.h"
#include "mystring.h"
#include "GPRS_AIR724.h"
#include "Protocol.h"
#include "IDConfig.h"
#include "Unixtimer.h"
#include "Led_App.h"
#include "MyFlash.h"
#include "StorageConfig.h"
#include "RemoteUpdata.h"
#include "Environment_data.h"
#include "socketserver.h"
#include "lwip.h"
#include "httpd.h"
#include "BL0942.h"
#include "bsp_key.h"
#include "UdpDevSearch.h"
#include "WaterLevelMonitor.h"


QueueHandle_t GPRS_UART_SEND_Queue;
QueueHandle_t GPRS_RECEIVE_Queue;

SemaphoreHandle_t AlarmBinary;	//报警信号量
SemaphoreHandle_t EleUpBinary;	//电量数据更新信号量

AlarmData_Type alarmData;
AlarmData_Type alarmDataLast = {0xff};

uint8_t platform_recvbuf[PLATFORM_BUFF_SIZE];					//定义数据处理Buff大小为300（为100也无所谓，只要大于等于100就好）
uint8_t platform_sendbuf[PLATFORM_BUFF_SIZE];

uint8_t Updata_Falg  =0;

static uint32_t get_time_tick = 0;
uint32_t Heart_Tick = 0;
uint32_t Alarm_Tick = 0;
uint32_t NTP_Err_Tick = 0;


extern void GetRTC(rtc_time_t *rtc_time);
extern void SetRTC(rtc_time_t *rtc_time);
extern void Get_ElectricData(uint8_t channle, CollectionData *electricdata);
extern void Write_ElectricData(uint8_t channle, CollectionData *electricdata);

void socket_close(void);

void EnvAddData_Send(void);
void Angele_Data_Send(void);
void ManholeCover_angle_Data_Send(void);
void AirSwitch_Data_Send(void);
void Switch_Data_Send(uint8_t waterlevel);

extern xTaskHandle RemoteUpdata_Task_Handle;

/*=====================================================
 * 函数功能: 	GPRS接收数据队列发送 中断调用
 * 输入参数:	长度 + 内容
 * 输出参数: 
 * 返    回:	1 失败, 0 成功
=====================================================*/
uint8_t MSG_GPRSReceiveDataFromISR(uint8_t *buf, uint16_t len)
{
	uint8_t Data[len+2];
	//申请缓存

	//复制消息
	Data[0] = len;
	Data[1] = len>>8;
	memcpy((void *)&Data[2],(const void *)buf,len);
	//发送消息

	if(xQueueSendToBackFromISR(GPRS_RECEIVE_Queue, Data, 0) != pdPASS)
	{
		_myprintf("\r\nxQueueSend error, GPRS_RECEIVE_Queue");
		return 1;
	}

	return 0;
}


/*=====================================================
* 函数功能: 	GPRS发送数据
 * 输入参数:	
 * 输出参数: 
 * 返    回:	
=====================================================*/
void Platform_SendDataByGPRS(uint8_t sn,uint16_t type, uint32_t id, uint8_t cmd,uint16_t len,uint8_t *senddata)
{
    uint8_t *pbuf;

	Sn++;
	Sn &= 0x3F;
    pbuf = Protocol_Pack(Sn,type,id,cmd,len,(uint8_t *)senddata);
#if Platform_Data_Printf
    myprintf("\r\n 4G send:");
    myprintf_buf_unic((uint8_t *)pbuf, 10+len+2);
#endif
	GPRS_SendData((uint8_t *)pbuf, 10+len+2);		//10为负载以前的固定数据的数据长度+负载数据长度+crc
}

void Platform_SendData(uint8_t sn,uint16_t type, uint32_t id, uint8_t cmd,uint16_t len,uint8_t *senddata)
{
	Sn++;
	Sn &= 0x3F;
	vTaskDelay(10);
#if USE_ETH_TO_INTERNET
	socket_SendData(sn,type,id,cmd,len,senddata);
#elif USE_4G_UART_TO_INTERNET
	Platform_SendDataByGPRS(sn,type,id,cmd,len,senddata);
#endif
}

/*=====================================================
* 函数功能: 	平台数据处理
 * 输入参数:	接受数据
 * 输出参数: 
 * 返    回:	
=====================================================*/
void Platform_Data_Process(uint8_t* msgdata)
{
	if(msgdata[0] == 0xAA)
	{
		uint16_t tmp_type = 0;
		uint32_t tmp_id = 0;
		#if Platform_Data_Printf
			#if USE_ETH_TO_INTERNET
				myprintf("\r\n TCP recever:");
			#elif USE_4G_UART_TO_INTERNET
				myprintf("\r\n gprs recever:");
			#endif
			myprintf_buf_unic((uint8_t *)msgdata,10+msgdata[9]+2);
		#endif
		tmp_type = msgdata[2]<<8 | msgdata[3];
		tmp_id = msgdata[4]<<24 | msgdata[5]<<16 | msgdata[6]<<8 | msgdata[7];
		if(tmp_type == Device_TYPE && tmp_id == Device_ID)		//ID校验
		{
			uint16_t crc_tmp1,crc_tmp2;
			uint8_t datalen = 10+msgdata[9]+2;		//10为负载以前的固定数据的数据长度+负载数据长度+crc
			crc_tmp2 = CRC16_calc(msgdata,datalen-2);
			crc_tmp1 = msgdata[datalen-1]<<8 | msgdata[datalen-2];
			if(crc_tmp2 == crc_tmp1)
			{
				if(msgdata[8] == Heart_Cmd)
				{
					return;
				}
				if(msgdata[8] == LedControl_Cmd)		//开关灯命令字
				{
					uint8_t i = 0;
					if(msgdata[10] > 0)
					{
						for(i = 0; i<msgdata[10]; i++)
						{
							if(msgdata[13+i*3]>=100)	//限位
								msgdata[13+i*3] = 100;
							Write_LED_Data(msgdata[11+i*3],&msgdata[12+i*3],&msgdata[13+i*3]);	//LED亮度目标百分比调整
							myprintf("\r\n 灯%d:%d 亮度：%d",msgdata[11+i*3],msgdata[12+i*3],msgdata[13+i*3]);
						}
					}
					Platform_SendData(Sn,Device_TYPE,Device_ID,LedControlBack_Cmd,msgdata[9],&msgdata[10]);	//数据回传

					xSemaphoreGive(LedShowBinary);			//演示模式
				}
				else if(msgdata[8] == TimeControl_Cmd)		//时间策略控制命令字
				{
					uint8_t i = 0;
					TimeControl_Type timecontrol_data;
					
					for(i = 0; i<msgdata[10]; i++)
					{
						memcpy(&timecontrol_data,&msgdata[12+i*15],sizeof(timecontrol_data));
						timecontrol_data.phase1_time = HALFWORD_Reverse(timecontrol_data.phase1_time);
						timecontrol_data.phase2_time = HALFWORD_Reverse(timecontrol_data.phase2_time);
						timecontrol_data.phase3_time = HALFWORD_Reverse(timecontrol_data.phase3_time);
						timecontrol_data.phase4_time = HALFWORD_Reverse(timecontrol_data.phase4_time);
						if(timecontrol_data.phase1_Pwm >= 100)
							timecontrol_data.phase1_Pwm = 100;
						if(timecontrol_data.phase2_Pwm >= 100)
							timecontrol_data.phase2_Pwm = 100;
						if(timecontrol_data.phase3_Pwm >= 100)
							timecontrol_data.phase3_Pwm = 100;
						if(timecontrol_data.phase4_Pwm >= 100)
							timecontrol_data.phase4_Pwm = 100;
						Write_TimeControl_Data(msgdata[11+i*15], &timecontrol_data);
						myprintf("\r\n%d号灯 时间控制策略:开始时间:%02d:%02d,阶段一:time:%d  pwm:%d  阶段二:time:%d  pwm:%d  阶段三:time:%d  pwm:%d  阶段四:time:%d  pwm:%d\r\n",\
						msgdata[11+i*15], timecontrol_data.start_hour,timecontrol_data.start_min,timecontrol_data.phase1_time,timecontrol_data.phase1_Pwm,\
						timecontrol_data.phase2_time,timecontrol_data.phase2_Pwm,timecontrol_data.phase3_time,timecontrol_data.phase3_Pwm,\
						timecontrol_data.phase4_time,timecontrol_data.phase4_Pwm);
						if(msgdata[11+i*15] <= LED_NUM)
						{
							Write_UserMem_Flash(TIMECONTROL_ADDR+sizeof(timecontrol_data)*(msgdata[11+i*15]-1),(uint8_t *)&timecontrol_data, sizeof(timecontrol_data));
						}
					}
					
					Platform_SendData(Sn,Device_TYPE,Device_ID,TimeControlBack_Cmd,msgdata[10]*15+1,&msgdata[10]);	//数据回传 ,字节长度为0
				}
				else if(msgdata[8] == FindTimeCtr_Cmd)		//查询时间策略控制命令字
				{
					TimeControl_Type timecontrol_data;
					uint8_t *findtimectr_data;
					uint8_t i = 0;
					
					findtimectr_data = pvPortMalloc(sizeof(uint8_t)*(1+15*LED_NUM));
					
					findtimectr_data[0] = LED_NUM;
					for(i = 0; i<LED_NUM; i++)
					{
						findtimectr_data[1+i*15] = i+1;
						Get_TimeControl_Data(i+1, &timecontrol_data);
						
						timecontrol_data.phase1_time = HALFWORD_Reverse(timecontrol_data.phase1_time);
						timecontrol_data.phase2_time = HALFWORD_Reverse(timecontrol_data.phase2_time);
						timecontrol_data.phase3_time = HALFWORD_Reverse(timecontrol_data.phase3_time);
						timecontrol_data.phase4_time = HALFWORD_Reverse(timecontrol_data.phase4_time);
						
						memcpy(&findtimectr_data[2+i*15],&timecontrol_data.start_hour,sizeof(timecontrol_data));
					}
					Platform_SendData(Sn,Device_TYPE,Device_ID,FindTimeCtrBack_Cmd,sizeof(uint8_t)*(1+15*LED_NUM),findtimectr_data);	
					vPortFree(findtimectr_data);
				}
				else if(msgdata[8] == FindElectric_Cmd)		//查询电压、电流电气命令字
				{
					uint8_t *findelectric_data;
					CollectionData electricdata = {0};
					uint32_t energy[LED_NUM];
					uint8_t i = 0;
					
					findelectric_data = pvPortMalloc(sizeof(uint8_t)*(1+13*LED_NUM));
					
					findelectric_data[0] = LED_NUM;
					for(i = 0; i<LED_NUM; i++)
					{
						findelectric_data[1+i*13] = i+1;
						Get_ElectricData(i+1, &electricdata);
						
						energy[i] = electricdata.Energy;
						electricdata.Voltage = WORD_Reverse(electricdata.Voltage);
						electricdata.Current = WORD_Reverse(electricdata.Current);
						electricdata.Energy = WORD_Reverse(electricdata.Energy);
						
						memcpy(&findelectric_data[2+i*13],&electricdata.Voltage,sizeof(electricdata.Voltage));
						memcpy(&findelectric_data[6+i*13],&electricdata.Current,sizeof(electricdata.Current));
						memcpy(&findelectric_data[10+i*13],&electricdata.Energy,sizeof(electricdata.Energy));
					}
					Platform_SendData(Sn,Device_TYPE,Device_ID,FindEleBack_Cmd,sizeof(uint8_t)*(1+13*LED_NUM),findelectric_data);	//数据回传
//					Write_UserMem_Flash(ENERGY_ADDR,(uint8_t *)&energy, sizeof(energy));
					vPortFree(findelectric_data);
				}
				else if(msgdata[8] == FindEnvironment_Cmd)		//查询环境数据命令字
				{
					Herat_Data_Send();		//回复心跳
					EnvAddData_Send();
					Angele_Data_Send();
					AirSwitch_Data_Send();
					ManholeCover_angle_Data_Send();
					Switch_Data_Send(!(HAL_GPIO_ReadPin(INPUT_GPIO_Port, INPUT_Pin)));
				}

				#if CUSTOM_DEV_NUM
				else if(msgdata[8] == CustomCtrl_Cmd)		//开关用户自定义设备命令字
				{
					uint8_t i = 0;
					if(msgdata[10] > 0)					//用户自定义设备个数
					{
						for(i = 0; i<msgdata[10]; i++)
						{
							if(msgdata[13+i*2]>=2)	//限位
								msgdata[13+i*2] = 1;	//用户自定义设备状态大于2就默认开灯
							Write_LED_Data(msgdata[11+i*2]+LED_NUM,&msgdata[12+i*2],&i);	//LED亮度目标百分比调整
							myprintf("\r\n 用户自定义设备%d:%d ",msgdata[11+i*2],msgdata[12+i*2]);
						}
					}
					Platform_SendData(Sn,Device_TYPE,Device_ID,CustomCtrlBack_Cmd,msgdata[9],&msgdata[10]);	//数据回传

					xSemaphoreGive(LedShowBinary);			//演示模式
				}
				
				else if(msgdata[8] == TimeCustomCtrl_Cmd)		//用户自定义设备时间策略控制命令字
				{
					uint8_t i = 0;
					TimeCustomControl_Type timecontrol_data;
					
					for(i = 0; i<msgdata[10]; i++)
					{
						memcpy(&timecontrol_data,&msgdata[12+i*15],sizeof(timecontrol_data));
						timecontrol_data.phase1_time = HALFWORD_Reverse(timecontrol_data.phase1_time);
						timecontrol_data.phase2_time = HALFWORD_Reverse(timecontrol_data.phase2_time);
						timecontrol_data.phase3_time = HALFWORD_Reverse(timecontrol_data.phase3_time);
						timecontrol_data.phase4_time = HALFWORD_Reverse(timecontrol_data.phase4_time);
						if(timecontrol_data.led_sta1 >= 2)
							timecontrol_data.led_sta1 = 0;
						if(timecontrol_data.led_sta2 >= 2)
							timecontrol_data.led_sta2 = 0;
						if(timecontrol_data.led_sta3 >= 2)
							timecontrol_data.led_sta3 = 0;
						if(timecontrol_data.led_sta4 >= 2)
							timecontrol_data.led_sta4 = 0;
						Write_TimeCustomCtrl_Data(msgdata[11+i*15], &timecontrol_data);
						myprintf("\r\n%d号用户自定义设备 时间控制策略:开始时间:%02d:%02d,阶段一:time:%d  sta:%d  阶段二:time:%d  sta:%d  阶段三:time:%d  sta:%d  阶段四:time:%d  sta:%d\r\n",\
						msgdata[11+i*15], timecontrol_data.start_hour,timecontrol_data.start_min,timecontrol_data.phase1_time,timecontrol_data.led_sta1,\
						timecontrol_data.phase2_time,timecontrol_data.led_sta2,timecontrol_data.phase3_time,timecontrol_data.led_sta3,\
						timecontrol_data.phase4_time,timecontrol_data.led_sta4);
						if(msgdata[11+i*15] <= CUSTOM_DEV_NUM)
						{
							Write_UserMem_Flash(TIMECTRCUSTOM_ADDR+sizeof(timecontrol_data)*(msgdata[11+i*15]-1),(uint8_t *)&timecontrol_data, sizeof(timecontrol_data));
						}
					}
					
					Platform_SendData(Sn,Device_TYPE,Device_ID,TimeCustomCtrlBack_Cmd,0,&msgdata[10]);	//数据回传 ,字节长度为0
				}
				else if(msgdata[8] == FindTimeCustomCtr_Cmd)		//查询用户自定义时间策略控制命令字
				{
					TimeCustomControl_Type timecontrol_data;
					uint8_t findtimectr_data[1+15*CUSTOM_DEV_NUM];
					uint8_t i = 0;
					
					findtimectr_data[0] = CUSTOM_DEV_NUM;
					for(i = 0; i<CUSTOM_DEV_NUM; i++)
					{
						findtimectr_data[1+i*15] = i+1;
						Get_TimeCustomCtrl_Data(i+1, &timecontrol_data);
						
						timecontrol_data.phase1_time = HALFWORD_Reverse(timecontrol_data.phase1_time);
						timecontrol_data.phase2_time = HALFWORD_Reverse(timecontrol_data.phase2_time);
						timecontrol_data.phase3_time = HALFWORD_Reverse(timecontrol_data.phase3_time);
						timecontrol_data.phase4_time = HALFWORD_Reverse(timecontrol_data.phase4_time);
						
						memcpy(&findtimectr_data[2+i*15],&timecontrol_data.start_hour,sizeof(timecontrol_data));
					}
					Platform_SendData(Sn,Device_TYPE,Device_ID,FindTimeCustomCtrBack_Cmd,sizeof(findtimectr_data),findtimectr_data);	
				}
				#endif
				else if(msgdata[8] == FindLedSta_Cmd)		//查询LED及继电器状态命令字
				{
					uint8_t i = 0;
					uint8_t *findledsta_data;
					
					#if CUSTOM_DEV_NUM
					findledsta_data = pvPortMalloc(1+3*LED_NUM+1+2*CUSTOM_DEV_NUM);
					#else
					
					findledsta_data = pvPortMalloc(1+3*LED_NUM);
					#endif
					
					findledsta_data[0] = LED_NUM;
					for(i = 0; i<LED_NUM; i++)
					{
						findledsta_data[1+i*3] = i+1;
						Get_LED_Data(i+1, &findledsta_data[2+i*3], &findledsta_data[3+i*3]);
					}
					
					#if CUSTOM_DEV_NUM
					findledsta_data[1+3*LED_NUM] = CUSTOM_DEV_NUM;
					for(i = 0; i<CUSTOM_DEV_NUM; i++)
					{
						findledsta_data[1+3*LED_NUM+1+i*2] = i+1;
						Get_LED_Data(i+1+LED_NUM, &findledsta_data[1+3*LED_NUM+2+i*2], &i);
					}
					#endif
					Platform_SendData(Sn,Device_TYPE,Device_ID,FindLedStaBack_Cmd,1+3*LED_NUM+1+2*CUSTOM_DEV_NUM,findledsta_data);	
					vPortFree(findledsta_data);
				}
				else if(msgdata[8] == AlarmBack_Cmd)		//报警返回命令字
				{
					
				}
				else if(msgdata[8] == SwitchDataBack_Cmd)		//开关量报警返回命令字
				{
//					Switch_Water.platform_ack_flag = 1;
					Switch_Water.platform_alarm_flag = 0;
				}
				else if(msgdata[8] == AirSwitchCtrl_Cmd)		//智能空开控制命令字
				{
					if(xQueueSend(TM_AirSwitch_Queue, &msgdata[12], 0) != pdPASS)
					{
						myprintf("\r\nxQueueSend error, TM_AirSwitch_Queue");
					}
					Platform_SendData(Sn,Device_TYPE,Device_ID,AirSwitchCtrlBack_Cmd,msgdata[9],&msgdata[10]);	//数据回传
				}
				else if(msgdata[8] == SetServerAddr_Cmd)		//设置平台服务器地址
				{
					TCPServerIP = msgdata[10] | msgdata[11]<<8 | msgdata[12]<<16 | msgdata[13]<<24;
					TCPServerPort = msgdata[15]<<8 | msgdata[14];
					NTPServerIP = msgdata[16] | msgdata[17]<<8 | msgdata[18]<<16 | msgdata[19]<<24;
					Write_UserMem_Flash(ServerIP_ADDR,(uint8_t *)&TCPServerIP,4);
					Write_UserMem_Flash(ServerPort_ADDR,(uint8_t *)&TCPServerPort,2);
					Write_UserMem_Flash(NTPServerIP_ADDR,(uint8_t *)&NTPServerIP,4);
					Platform_SendData(Sn,Device_TYPE,Device_ID,SetServerAddrBack_Cmd, 10,&msgdata[10]);		//数据回传
					socket_close();
					vTaskDelay(500);
					HAL_NVIC_SystemReset();
				}
				else if(msgdata[8] == RemoteUpdata_Cmd)		//远程更新命令字
				{
					Get_File_inform(msgdata);
					Platform_SendData(Sn,Device_TYPE,Device_ID,RemoteUpdataBack_Cmd,0,msgdata);	//数据回传
					#if USE_ETH_TO_INTERNET
//					Updata_Falg = 1;		//开始远程更新
					xSemaphoreGive (RemoteUpdataMutex);//开始远程更新
					taskENTER_CRITICAL();
					CreatRemoteUpdataTask();			//创建远程升级任务
					RemoteUpdata_tick = HAL_GetTick();
//					vTaskResume(RemoteUpdata_Task_Handle);	//恢复任务
					taskEXIT_CRITICAL();
//					return;
					#endif
				}
			}
		}
	}
	#if USE_ETH_TO_INTERNET
//	if(Updata_Falg)
	if(xSemaphoreTake (RemoteUpdataMutex, 0))
	{
//				RemoteUpDate(msgdata);
		xSemaphoreGive (RemoteUpdataMutex);
		xQueueSendToBack(RemoteUpdata_Queue,msgdata,0);
	}
	#endif
}
void GPRSDataPro(void)
{
	uint16_t p;
	uint32_t num;
	rtc_time_t tempTime;
	
	uint8_t *msgdata = NULL;
	uint16_t GPRSRecviveLen = 0;
		
	if(xQueueReceive(GPRS_RECEIVE_Queue, (void *)&platform_recvbuf, ( TickType_t )100) != pdPASS){
		return;
	}
	GPRSRecviveLen = platform_recvbuf[0] | (platform_recvbuf[1]<<8);
	msgdata = &platform_recvbuf[2];

	if((GPRSRecviveLen == 0) || (GPRSRecviveLen > PLATFORM_BUFF_SIZE))
	{
		myprintf("\r\nGPRS长度错误:%d",GPRSRecviveLen);
	}
	//远程更新
	RemoteUpdataBy4G(msgdata,256);
	//时间查找
	p = StrFindString(msgdata, GPRSRecviveLen, (uint8_t *)"time:", 5);
	if(p != 0xffff)
	{
		if(Str2Num(&msgdata[p+5], 2, &num) == 0) return;
		tempTime.ui8Year = (uint8_t)num+2000;
		if(Str2Num(&msgdata[p+8], 2, &num) == 0) return;
		tempTime.ui8Month = (uint8_t)num;
		if(Str2Num(&msgdata[p+11], 2, &num) == 0) return;
		tempTime.ui8DayOfMonth = (uint8_t)num;
		if(Str2Num(&msgdata[p+14], 2, &num) == 0) return;
		tempTime.ui8Hour = (uint8_t)num;
		if(Str2Num(&msgdata[p+17], 2, &num) == 0) return;
		tempTime.ui8Minute = (uint8_t)num;
		if(Str2Num(&msgdata[p+20], 2, &num) == 0) return;
		tempTime.ui8Second = (uint8_t)num;
		SetRTC(&tempTime);
		GetRTC(&tempTime);
#if Platform_Data_Printf
		myprintf("\r\n获取到北京时间：");
		myprintf("%02d-%02d-%02d  %02d:%02d:%02d\r\n", tempTime.ui8Year, tempTime.ui8Month,\
		tempTime.ui8DayOfMonth, tempTime.ui8Hour, tempTime.ui8Minute, tempTime.ui8Second);
#endif
		get_time_tick = HAL_GetTick();
	}
	else
	{
#if Platform_Data_Printf
		myprintf("\r\n gprs recever:");
		myprintf_buf_unic((uint8_t *)msgdata,10+msgdata[9]+2);
#endif
		Platform_Data_Process(&platform_recvbuf[2]);
	}
}

void Get_Platform_Time(void)
{
	if((HAL_GetTick() - get_time_tick)>60*1000 || get_time_tick == 0)
	{
		static uint32_t last_tick = 0;
		static uint32_t now_tick = 0;
		if((now_tick - last_tick)>=2000 || now_tick == 0)		//间隔2s
		{
			uint8_t tmp[15] = "apply for time";
			
			last_tick = now_tick;
			GPRS_SendData((uint8_t *)tmp,15);
#if Platform_Data_Printf
			myprintf("\r\n 尝试获取时间");
#endif
		}
		now_tick = HAL_GetTick();
	}
}

void EnvAddData_Send(void)
{
	Environment_Type environment_data;
	uint8_t *envadddata_pbuf;
	
	envadddata_pbuf = pvPortMalloc(sizeof(uint8_t)*(1+2*ENVADDDATA_NUM));
	Environment_Data_RW(0,&environment_data);
	envadddata_pbuf[0] = ENVADDDATA_NUM;
	for(uint8_t i = 0; i<ENVADDDATA_NUM; i++)
	{
//		envadddata_pbuf[1+i*2] = *(((uint8_t*)&environment_data.NAI)+i*2);
//		envadddata_pbuf[1+i*2+1] = *(((uint8_t*)&environment_data.NAI)+i*2+1);
		envadddata_pbuf[1+i*2] = *(((uint16_t*)&environment_data.NAI)+i*2)>>8&0xff;
		envadddata_pbuf[1+i*2+1] = *(((uint8_t*)&environment_data.NAI)+i*2)&0xff;
	}
	
	Platform_SendData(Sn,Device_TYPE,Device_ID,EnvAddData_Cmd,1+2*ENVADDDATA_NUM,envadddata_pbuf);	
	vPortFree(envadddata_pbuf);
}

void Herat_Data_Send(void)
{
	{
		static	uint8_t TCP_PAYLOAD[25+LED_NUM*3+1];
		uint8_t i =0;
		Environment_Type environment_data;
		
		Environment_Data_RW(0,&environment_data);
		myprintf("目前风速：%.1f m/s \n",(environment_data.wind_speed) / 100.0);
		myprintf("目前风向：%.1f° \n",(environment_data.wind_direction) / 100.0);
		myprintf("目前PM2.5：%.1f ug/m3 \n",(environment_data.pm2_5) / 1.0);
		myprintf("目前PM10：%.1f ug/m3 \n",(environment_data.pm10)/ 1.0);
		myprintf("目前温度：%.1f℃ \n",(environment_data.temperature)/ 100.0);
		myprintf("目前湿度：%.1f%% \n",(environment_data.humidity)/ 100.0);
		myprintf("目前大气压力：%.1f hPa \n",(environment_data.air_pressure)/ 10.0);
		myprintf("目前噪声：%.1f dB \n",(environment_data.noise)/ 10.0);
		myprintf("目前降雨量：%.1f mm \n",(environment_data.rain)/ 10.0);
		myprintf("目前紫外线指数：%d \n",(environment_data.ultraviolet)/ 1.0);
/*		environment_data.air_pressure = HALFWORD_Reverse(environment_data.air_pressure);
		environment_data.CO = HALFWORD_Reverse(environment_data.CO);
		environment_data.CO2 = HALFWORD_Reverse(environment_data.CO2);
		environment_data.humidity = HALFWORD_Reverse(environment_data.humidity);
		environment_data.NO2 = HALFWORD_Reverse(environment_data.NO2);
		environment_data.noise = HALFWORD_Reverse(environment_data.noise);
		environment_data.O3 = HALFWORD_Reverse(environment_data.O3);
		environment_data.pm10 = HALFWORD_Reverse(environment_data.pm10);
		environment_data.pm2_5 = HALFWORD_Reverse(environment_data.pm2_5);
		environment_data.radiation = HALFWORD_Reverse(environment_data.radiation);
		environment_data.rain = HALFWORD_Reverse(environment_data.rain);
		environment_data.SO2 = HALFWORD_Reverse(environment_data.SO2);
		environment_data.sun_power = HALFWORD_Reverse(environment_data.sun_power);
		environment_data.temperature = HALFWORD_Reverse(environment_data.temperature);
		environment_data.ultraviolet = HALFWORD_Reverse(environment_data.ultraviolet);
		environment_data.wind_speed = HALFWORD_Reverse(environment_data.wind_speed);*/
		TCP_PAYLOAD[0] = 25+LED_NUM*3;
		TCP_PAYLOAD[1] = environment_data.wind_direction>>8;
		TCP_PAYLOAD[2] = environment_data.wind_direction;
		TCP_PAYLOAD[3] = environment_data.wind_speed>>8;
		TCP_PAYLOAD[4] = environment_data.wind_speed;
		TCP_PAYLOAD[5] = environment_data.pm2_5>>8;
		TCP_PAYLOAD[6] = environment_data.pm2_5;
		TCP_PAYLOAD[7] = environment_data.pm10>>8;
		TCP_PAYLOAD[8] = environment_data.pm10;
		TCP_PAYLOAD[9] = environment_data.temperature>>8;
		TCP_PAYLOAD[10] = environment_data.temperature;
		TCP_PAYLOAD[11] = environment_data.humidity>>8;
		TCP_PAYLOAD[12] = environment_data.humidity;
		TCP_PAYLOAD[13] = environment_data.air_pressure>>8;
		TCP_PAYLOAD[14] = environment_data.air_pressure;
		TCP_PAYLOAD[15] = environment_data.noise>>8;
		TCP_PAYLOAD[16] = environment_data.noise;
		TCP_PAYLOAD[17] = environment_data.rain>>8;
		TCP_PAYLOAD[18] = environment_data.rain;
		TCP_PAYLOAD[19] = environment_data.ultraviolet>>8;
		TCP_PAYLOAD[20] = environment_data.ultraviolet;
		
		TCP_PAYLOAD[21] = LED_NUM;
		for(i = 0; i<LED_NUM; i++)
		{
			TCP_PAYLOAD[22+i*3] = i+1;
			Get_LED_Data(i+1, &TCP_PAYLOAD[23+i*3], &TCP_PAYLOAD[24+i*3]);
		}
		TCP_PAYLOAD[22+3*i] = (uint8_t)(HARDWARE_VERSION>>8);
		TCP_PAYLOAD[22+3*i+1] = (uint8_t)HARDWARE_VERSION;
		TCP_PAYLOAD[22+3*i+2] = (uint8_t)(SOFTWARE_VERSION>>8);
		TCP_PAYLOAD[22+3*i+3] = (uint8_t)SOFTWARE_VERSION;
		Platform_SendData(Sn,Device_TYPE,Device_ID,Heart_Cmd,TCP_PAYLOAD[0],(uint8_t *)&TCP_PAYLOAD[1]);
	}
}

void Alarm_Data_Set(void)
{
	alarmData.devNum = DEVNUM;
#if LORA_DEV
	alarmData.LoraNum = LORA_DEV;
	alarmData.devLora = 0x07;
#endif
#if NB_DEV
	alarmData.NBNum = NB_DEV;
	alarmData.devNB = 0x07;
#endif
#if GPRS4G_DEV
	alarmData.GPRS4GNum = GPRS4G_DEV;
	alarmData.dev4G = 0x07;
#endif
#if ETH_DEV
	alarmData.EthNum = ETH_DEV;
	alarmData.devEth = 0X00;
#endif
#if ELECTRIC1_DEV
	CollectionData electricdata1;
	Get_ElectricData(1, &electricdata1);
	alarmData.Electric1Num = ELECTRIC1_DEV;
	alarmData.devElectric1 = electricdata1.Bl6526bState;
#endif
#if ELECTRIC2_DEV
	CollectionData electricdata2;
	Get_ElectricData(2, &electricdata2);
	alarmData.Electric2Num = ELECTRIC2_DEV;
	alarmData.devElectric2 = electricdata2.Bl6526bState;
#endif
}
void Alarm_Data_Send(void)
{
	Platform_SendData(Sn,Device_TYPE,Device_ID,Alarm_Cmd,sizeof(alarmData),(uint8_t *)&alarmData);
}

void Eletric_Data_Send(void)
{
	uint8_t *findelectric_data;
	CollectionData electricdata = {0};
	uint32_t energy[LED_NUM];
	uint8_t i = 0;
	uint8_t *findledsta_data;
	
	findelectric_data = pvPortMalloc(sizeof(uint8_t)*(1+13*LED_NUM));
	
	findelectric_data[0] = LED_NUM;
	for(i = 0; i<LED_NUM; i++)
	{
		findelectric_data[1+i*13] = i+1;
		Get_ElectricData(i+1, &electricdata);
		
		electricdata.Voltage = WORD_Reverse(electricdata.Voltage);
		electricdata.Current = WORD_Reverse(electricdata.Current);
		electricdata.Energy = WORD_Reverse(electricdata.Energy);
		
		memcpy(&findelectric_data[2+i*13],&electricdata.Voltage,sizeof(electricdata.Voltage));
		memcpy(&findelectric_data[6+i*13],&electricdata.Current,sizeof(electricdata.Current));
		memcpy(&findelectric_data[10+i*13],&electricdata.Energy,sizeof(electricdata.Energy));
	}
	Platform_SendData(Sn,Device_TYPE,Device_ID,FindEleBack_Cmd,1+13*LED_NUM,findelectric_data);	//数据回传
	vPortFree(findelectric_data);
	findledsta_data = pvPortMalloc(1+3*LED_NUM+1+2*CUSTOM_DEV_NUM);
	findledsta_data[0] = LED_NUM;
	for(i = 0; i<LED_NUM; i++)
	{
		findledsta_data[1+i*3] = i+1;
		Get_LED_Data(i+1, &findledsta_data[2+i*3], &findledsta_data[3+i*3]);
	}
	#if CUSTOM_DEV_NUM
	findledsta_data[1+3*LED_NUM] = CUSTOM_DEV_NUM;
	for(i = 0; i<CUSTOM_DEV_NUM; i++)
	{
		findledsta_data[1+3*LED_NUM+1+i*2] = i+1;
		Get_LED_Data(i+1+LED_NUM, &findledsta_data[1+3*LED_NUM+2+i*2], &i);
	}
	Platform_SendData(Sn,Device_TYPE,Device_ID,FindLedStaBack_Cmd,1+3*LED_NUM+1+2*CUSTOM_DEV_NUM,findledsta_data);	
	#else
		Platform_SendData(Sn,Device_TYPE,Device_ID,FindLedStaBack_Cmd,1+3*LED_NUM,findledsta_data);	
	#endif
	vPortFree(findledsta_data);
}

void Switch_Data_Send(uint8_t waterlevel)
{
	uint8_t waterdata;
	waterdata = waterlevel;
	Platform_SendData(Sn,Device_TYPE,Device_ID,SwitchData_Cmd,1,(uint8_t *)&waterdata);
}

void Angele_Data_Send(void)
{
	uint8_t send_buf[9];
	
	ANGLE_MTC70M_Type *angle_data;
	
	if((angle_data = pvPortMalloc(sizeof(ANGLE_MTC70M_Type))) == NULL)
	{
		myprintf("\r\nUartApp angle_data malloc error");
		return;
	}
	send_buf[0] = 1;
	send_buf[3] = 1;
	send_buf[6] = 0;
	Angle_Data_RW(0, angle_data);
	send_buf[1] = (uint8_t)(angle_data->angle_x>>8);
	send_buf[2] = (uint8_t)(angle_data->angle_x);
	send_buf[4] = (uint8_t)(angle_data->angle_y>>8);
	send_buf[5] = (uint8_t)(angle_data->angle_y);
	send_buf[7] = (uint8_t)(angle_data->angle_z>>8);
	send_buf[8] = (uint8_t)(angle_data->angle_z);
//	angle_data->angle_x = HALFWORD_Reverse(angle_data->angle_x);
//	angle_data->angle_y = HALFWORD_Reverse(angle_data->angle_y);
//	angle_data->angle_z = HALFWORD_Reverse(angle_data->angle_z);
	Platform_SendData(Sn,Device_TYPE,Device_ID,AngleData_Cmd,sizeof(send_buf), send_buf);

	vPortFree(angle_data);
}
void ManholeCover_angle_Data_Send(void)
{
	uint8_t send_buf[1+10*MANHOLECOVER_ANGLE_NUM];
	uint8_t i;
	
	ANGLE_MTC70M_Type *angle_data;
	
	if((angle_data = pvPortMalloc(sizeof(ANGLE_MTC70M_Type))) == NULL)
	{
		myprintf("\r\nUartApp angle_data malloc error");
		return;
	}
	send_buf[0] = MANHOLECOVER_ANGLE_NUM;
	for(i=0; i<MANHOLECOVER_ANGLE_NUM; i++)
	{
		ManholeCover_angle_Data_RW(0, i, angle_data);
		send_buf[1+10*i] = i+1;
		send_buf[2+10*i] = 1;
		send_buf[5+10*i] = 1;
		send_buf[8+10*i] = 0;
		send_buf[3+10*i] = (uint8_t)(angle_data->angle_x>>8);
		send_buf[4+10*i] = (uint8_t)(angle_data->angle_x);
		send_buf[6+10*i] = (uint8_t)(angle_data->angle_y>>8);
		send_buf[7+10*i] = (uint8_t)(angle_data->angle_y);
		send_buf[9+10*i] = (uint8_t)(angle_data->angle_z>>8);
		send_buf[10+10*i] = (uint8_t)(angle_data->angle_z);
	}
	Platform_SendData(Sn,Device_TYPE,Device_ID,ManholeCover_angle_Cmd,sizeof(send_buf), send_buf);
	
	vPortFree(angle_data);
}

void AirSwitch_Data_Send(void)
{
	uint8_t *air_switch_data;
	AIRSWITCH_TM_Type* tmp;
	if((air_switch_data = pvPortMalloc(sizeof(AIRSWITCH_TM_Type)+2)) == NULL)
	{
		myprintf("\r\nUartApp air_switch_data malloc error");
		return;
	}
	AirSwitch_TM_Data_RW(0, air_switch_data+2);

	tmp = (AIRSWITCH_TM_Type*)(air_switch_data+2);
	if(HALFWORD_Reverse(tmp->switch_sta) == TM_AIRSWITCH_OPEN)
	{
		tmp->switch_sta = HALFWORD_Reverse(0x00);
	}
	else if(HALFWORD_Reverse(tmp->switch_sta) == TM_AIRSWITCH_CLOSE)
	{
		tmp->switch_sta = HALFWORD_Reverse(0x01);
	}
	
	air_switch_data[0] = 1;		//空开个数
	air_switch_data[1] = 1;	//空开id
	Platform_SendData(Sn,Device_TYPE,Device_ID,AirSwitchData_Cmd,sizeof(AIRSWITCH_TM_Type)+2, air_switch_data);
	vPortFree(air_switch_data);
}

extern uint8_t ETH_Link_flag;

void NetParaRst(void)
{
	DeviceIP = 192 | 168<<8 | 1<<16 | 105<<24;
	Write_UserMem_Flash(DeviceIP_ADDR,(uint8_t *)&DeviceIP,4);
	DeviceNetmask = 255 | 255<<8 | 255<<16 | 0<<24;
	Write_UserMem_Flash(DeviceNetmask_ADDR,(uint8_t *)&DeviceNetmask,4);
	DeviceGw = 192 | 168<<8 | 1<<16 | 1<<24;
	Write_UserMem_Flash(DeviceGw_ADDR,(uint8_t *)&DeviceGw,4);
	TCPServerIP = 121 | 199<<8 | 69<<16 | 67<<24;
	Write_UserMem_Flash(ServerIP_ADDR,(uint8_t *)&TCPServerIP,4);
	TCPServerPort = HALFWORD_Reverse(9001);
	Write_UserMem_Flash(ServerPort_ADDR,(uint8_t *)&TCPServerPort,2);
	DHCP_Enable_Flag = 0;
	Write_UserMem_Flash(DHCP_Enable_ADDR,(uint8_t *)&DHCP_Enable_Flag,1);
	NTPServerIP = 120 | 25<<8 | 115<<16 | 20<<24;
	Write_UserMem_Flash(NTPServerIP_ADDR,(uint8_t *)&NTPServerIP,4);
	NVIC_SystemReset();
}

void Platform_Task(void *argument)
{
	int8_t ntp_flag = 1;
	uint8_t * link_state;
	uint8_t link_state_old;
	uint8_t ucKeyCode;		/* 按键代码 */
	uint8_t netsta;
	uint8_t waterlevel = 0;
	
	TcpConnectFlag_RW(0, &netsta);
	
	myprintf("\r\nStart Platform_Task");
	
#if USE_ETH_TO_INTERNET
	link_state = &ETH_Link_flag;
	link_state_old = netsta;
	while(1)		//网线未插则停留在此
	{
		if(*link_state == 1)	
		{
			break;
		}
		ucKeyCode = bsp_GetKey();	/* 读取键值, 无键按下时返回 KEY_NONE = 0 */
		if (ucKeyCode != KEY_NONE)
		{
			switch (ucKeyCode)
			{
				case KEY_DOWN_K1:			/* K1键按下 */
					myprintf("K1键按下\r\n");
					break;
				case KEY_LONG_K1:			/* K1键长按 */
					myprintf("K1键长按\r\n");
					NetParaRst();		//网络参数复位
					break;
				case KEY_DOWN_K2:			/* K1键按下 */
					myprintf("K2键按下\r\n");
					break;
				case KEY_LONG_K2:			/* K1键长按 */
					myprintf("K2键长按\r\n");
//					NetParaRst();		//网络参数复位
					break;
				case KEY_2_UP:
					myprintf("K2键释放\r\n");
					break;
			}
		}
		vTaskDelay(100);
	}
	httpd_init();		//web server
//	Create_TCP_connect();	//创建一个socket 阻塞方式创建
	UdpDevSearch_Server_Init();	//创建一个用于设备搜索的udp服务器 必须先创建
	Create_TCP_Connect_TimeOut();//创建一个socket 非阻塞方式创建
#endif
	while(1)
	{
#if USE_ETH_TO_INTERNET
		TcpConnectFlag_RW(0, &netsta);	//获取TCP连接状态
		if(*link_state == 1)
		{
			UdpDevSearch_Recv();	//设备搜索广播数据处理
			if(ntp_flag)	//获取ntp时间
			{
				if(ntpTime() <= 0)	//获取时间失败
				{
					NTP_Err_Tick = HAL_GetTick();
					ntp_flag = 0;
				}
				else
					ntp_flag = 1;
			}
			else 
			{
				if((HAL_GetTick() - NTP_Err_Tick) >= 300*1000)	//获取时间失败后间隔300s后再次获取
				{
					ntp_flag = 1;
				}
			}
			rebuild_TCP_connect();	//数据接收和socket重连
			if((HAL_GetTick() - Heart_Tick)>=Herat_Rate*1000 || Heart_Tick == 0)
			{
				Heart_Tick = HAL_GetTick();
				if(netsta == ConnectOnLine)
				{
					Herat_Data_Send();
					EnvAddData_Send();
				}
			}
			if(link_state_old == ConnectOffLine && netsta == ConnectOnLine)	//socket重新连接需立刻发一包心跳
			{
				Herat_Data_Send();
				EnvAddData_Send();
				Heart_Tick = HAL_GetTick();
			}
			link_state_old = netsta;
			
			if(xSemaphoreTake(AlarmBinary, 0) == pdTRUE)
			{
				Alarm_Data_Set();
				if(StrComplate((uint8_t *)&alarmDataLast,(uint8_t *)&alarmData,sizeof(alarmData)) == 0) {	//如果数据不同则立即报警
					alarmDataLast = alarmData;
					Alarm_Tick = HAL_GetTick();
					myprintf("产生报警信号\r\n");
					Alarm_Data_Send();
				} else  {
					if((HAL_GetTick() - Alarm_Tick)>=1*60*1000) {
						myprintf("产生报警信号\r\n");
						Alarm_Tick = HAL_GetTick();
						Alarm_Data_Send();
					}
				}
			}
			if(netsta == ConnectOnLine)
			{
				if(xSemaphoreTake (EleUpBinary, 0) == pdTRUE)		//灯状态发生变化则上报状态
				{
					Eletric_Data_Send();
				}
				if(xSemaphoreTake (AngleBinary, 0) == pdTRUE)		//倾角状态发生变化则上报状态
				{
					Angele_Data_Send();
				}
				if(xSemaphoreTake (AirSwitchBinary, 0) == pdTRUE)		//智能空开状态发生变化则上报状态
				{
					AirSwitch_Data_Send();
				}
				if(xSemaphoreTake (ManholeCoverBinary, 0) == pdTRUE)		//井盖倾角状态发生变化则上报状态
				{
					ManholeCover_angle_Data_Send();
				}
			}
		}
		else
		{
			socket_reconnet_flag = 1;	//网线掉了，下次重连需新建socket
		}
		
#elif USE_4G_UART_TO_INTERNET
		Get_Platform_Time();
		GPRSDataPro();	//处理GPRS数据 等待消息队列阻塞100ms
		if((HAL_GetTick() - Heart_Tick)>=Herat_Rate*1000 || Heart_Tick == 0)
		{
			Heart_Tick = HAL_GetTick();
//			if(Tcp_Connect_Flag == ConnectOnLine)
			{
				Herat_Data_Send();
			}
		}
#endif
		waterlevel = WaterServiceLogic();
		if(waterlevel == 1)
		{
			myprintf("\r\n水位达到警戒水位");
			if(netsta == ConnectOnLine)
			{
				Switch_Data_Send(1);
			}
		}
		else if(waterlevel == 2)
		{
			myprintf("\r\n水位降到警戒线以下");
			if(netsta == ConnectOnLine)
			{
				Switch_Data_Send(0);
			}
		}
		vTaskDelay(100);
	}
}

void CreatPlatformTask(void)
{
	BaseType_t Platform_TaskID;
	
	myprintf("\r\n CreatPlatformTask");

	tcp_reconnect.connect_cnt = 0;
	tcp_reconnect.rebuild_interval = 0;
	tcp_reconnect.refresh_tick = 0;
	tcp_reconnect.socket_flag = 0;
	tcp_reconnect.socket_reboot_tick = 0;
	
	Platform_TaskID = xTaskCreate (Platform_Task, "Platform", Platform_StkSIZE, NULL, Platform_TaskPrio, NULL);
	if(pdPASS != Platform_TaskID)
	{
        myprintf("\r\n CreatPlatformTask creat error");
        while(1);
    }
}
