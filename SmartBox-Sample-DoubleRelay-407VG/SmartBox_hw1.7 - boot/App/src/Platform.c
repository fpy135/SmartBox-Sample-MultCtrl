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
#include "bl6526b.h"
#include "socketserver.h"

QueueHandle_t GPRS_UART_SEND_Queue;
QueueHandle_t GPRS_RECEIVE_Queue;

uint8_t platform_recvbuf[PLATFORM_BUFF_SIZE];					//定义数据处理Buff大小为300（为100也无所谓，只要大于等于100就好）
uint8_t platform_sendbuf[PLATFORM_BUFF_SIZE];

uint8_t Updata_Falg  =0;

static uint32_t get_time_tick = 0;
uint32_t Heart_Tick = 0;

extern void GetRTC(rtc_time_t *rtc_time);
extern void SetRTC(rtc_time_t *rtc_time);

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
		#if Platform_Data_Printf
			#if USE_ETH_TO_INTERNET
				myprintf("\r\n TCP recever:");
			#elif USE_4G_UART_TO_INTERNET
				myprintf("\r\n gprs recever:");
			#endif
			myprintf_buf_unic((uint8_t *)msgdata,10+msgdata[9]+2);
		#endif
		uint16_t tmp_type = 0;
		uint32_t tmp_id = 0;
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
				if(msgdata[8] == LedControl_Cmd)		//开关灯命令字
				{
					if(msgdata[10] == LEDON)	//LED开关状态更新
					{
						if(msgdata[11]>99)	//限位
							msgdata[11] = 99;

						Write_LED_Data(&msgdata[10],&msgdata[11]);	//LED亮度目标百分比调整
						HAL_GPIO_WritePin(GPIOC, REL_EN2_Pin|REL_EN1_Pin, GPIO_PIN_RESET);	//打开继电器电源
#if Platform_Data_Printf
						myprintf("\r\n 开灯 亮度：%d",msgdata[11]);
#endif
					}
					else if(msgdata[10] == LEDOFF)
					{
						if(msgdata[11]>99)	//限位
							msgdata[11] = 99;
						Write_LED_Data(&msgdata[10],&msgdata[11]);	//LED亮度目标百分比调整
						HAL_GPIO_WritePin(GPIOC, REL_EN2_Pin|REL_EN1_Pin, GPIO_PIN_SET);	//关闭继电器电源
#if Platform_Data_Printf
						myprintf("\r\n 关灯 亮度：%d",msgdata[11]);
#endif
					}
					Platform_SendData(Sn,Device_TYPE,Device_ID,LedControlBack_Cmd,4,&msgdata[10]);	//数据回传

					xSemaphoreGive(LedShowBinary);			//演示模式
				}
				else if(msgdata[8] == TimeControl_Cmd)		//时间策略控制命令字
				{
					TimeControl_Type timecontrol_data;
					
					timecontrol_data.start_hour = msgdata[10];
					timecontrol_data.start_min = msgdata[11];
					timecontrol_data.phase1_time = (msgdata[12]<<8) | msgdata[13];
					timecontrol_data.phase1_Pwm = msgdata[14];
					timecontrol_data.phase2_time = (msgdata[15]<<8) | msgdata[16];
					timecontrol_data.phase2_Pwm = msgdata[17];
					timecontrol_data.phase3_time = (msgdata[18]<<8) | msgdata[19];
					timecontrol_data.phase3_Pwm = msgdata[20];
					timecontrol_data.phase4_time = (msgdata[21]<<8) | msgdata[22];
					timecontrol_data.phase4_Pwm = msgdata[23];
					Write_TimeControl_Data(&timecontrol_data);
					//写入flash
					Write_UserMem_Flash(TIMECONTROL_ADDR,(uint8_t *)&timecontrol_data, sizeof(timecontrol_data));
					
					Platform_SendData(Sn,Device_TYPE,Device_ID,TimeControlBack_Cmd,0,&msgdata[10]);	//数据回传
				}
				else if(msgdata[8] == FindTimeCtr_Cmd)		//查询时间策略控制命令字
				{
					TimeControl_Type timecontrol_data;
					
					Get_TimeControl_Data(&timecontrol_data);
					timecontrol_data.phase1_time = HALFWORD_Reverse(timecontrol_data.phase1_time);
					timecontrol_data.phase2_time = HALFWORD_Reverse(timecontrol_data.phase2_time);
					timecontrol_data.phase3_time = HALFWORD_Reverse(timecontrol_data.phase3_time);
					timecontrol_data.phase4_time = HALFWORD_Reverse(timecontrol_data.phase4_time);
					Platform_SendData(Sn,Device_TYPE,Device_ID,FindTimeCtrBack_Cmd,sizeof(timecontrol_data),&timecontrol_data.start_hour);	//数据回传
				}
				else if(msgdata[8] == FindElectric_Cmd)		//查询电压、电流电气命令字
				{
					ElectricData_Type electricdata;
					BL6526B_ProcessTask(&electricdata);				/*电能计量芯片读取函数*/
					electricdata.Voltage1 = WORD_Reverse(electricdata.Voltage1);
					electricdata.Current1 = WORD_Reverse(electricdata.Current1);
					electricdata.Voltage2 = WORD_Reverse(electricdata.Voltage2);
					electricdata.Current2 = WORD_Reverse(electricdata.Current2);
					Platform_SendData(Sn,Device_TYPE,Device_ID,FindEleBack_Cmd,sizeof(electricdata),(uint8_t *)&electricdata.Voltage1);	//数据回传
				}
				else if(msgdata[8] == FindEnvironment_Cmd)		//查询环境数据命令字
				{
					Herat_Data_Send();		//回复心跳
				}
				else if(msgdata[8] == RemoteUpdata_Cmd)		//查询环境数据命令字
				{
					Get_File_inform(msgdata);
					Platform_SendData(Sn,Device_TYPE,Device_ID,RemoteUpdataBack_Cmd,0,msgdata);	//数据回传
					CreatRemoteUpdataTask();
//					Updata_Falg = 1;		//开始远程更新
					xSemaphoreGive (RemoteUpdataMutex);//开始远程更新
					return;
				}
			}
		}
	}
//	if(Updata_Falg)
	if(xSemaphoreTake (RemoteUpdataMutex, 0))
	{
//		if(msgdata[0] == 0xBB)
		{
//			uint16_t crc_tmp1,crc_tmp2;
//			crc_tmp2 = CRC16_calc(msgdata,PLATFORM_BUFF_SIZE-2);
//			crc_tmp1 = msgdata[PLATFORM_BUFF_SIZE-1]<<8 | msgdata[PLATFORM_BUFF_SIZE-2];
//			if(crc_tmp2 == crc_tmp1)
			{
//				RemoteUpDate(msgdata);
				xSemaphoreGive (RemoteUpdataMutex);
				xQueueSendToBack(RemoteUpdata_Queue,msgdata,0);
			}
		}
	}
}

void GPRSDataPro(void)
{
	uint16_t p;
	uint16_t q;
	uint32_t num;
	rtc_time_t tempTime;
	
	uint8_t *msgdata = NULL;
	uint16_t GPRSRecviveLen = 0;
	uint8_t *GPRSRecviveBuf = NULL;
	
	static uint8_t Updata_flag = 0;
	
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
	RemoteUpdata(msgdata,256);
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

void Herat_Data_Send(void)
{
	{
		static	uint8_t TCP_PAYLOAD[31];
		Environment_Type environment_data;
		
		Environment_Data_RW(0,&environment_data);
		myprintf("目前风速：%.1f m/s \n",(environment_data.wind_speed) / 100.0);
		myprintf("目前风向：%.1f° \n",(environment_data.wind_direction) / 100.0);
		myprintf("目前PM2.5：%.1f ug/m3 \n",(environment_data.pm2_5) / 10.0);
		myprintf("目前PM10：%.1f ug/m3 \n",(environment_data.pm10)/ 10.0);
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
		TCP_PAYLOAD[0] = 26;
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
		TCP_PAYLOAD[19] = environment_data.sun_power>>8;
		TCP_PAYLOAD[20] = environment_data.sun_power;
		Get_LED_Data(&TCP_PAYLOAD[21],&TCP_PAYLOAD[22]);
		TCP_PAYLOAD[23] = 0XFF;
		TCP_PAYLOAD[24] = 0XFF;
		TCP_PAYLOAD[25] = (uint8_t)(SOFTWARE_VERSION>>8);
		TCP_PAYLOAD[26] = (uint8_t)SOFTWARE_VERSION;
		Platform_SendData(Sn,Device_TYPE,Device_ID,Heart_Cmd,TCP_PAYLOAD[0],(uint8_t *)&TCP_PAYLOAD[1]);
	}
}

void Platform_Task(void *argument)
{
	_myprintf("\r\nStart Platform_Task");
#if USE_ETH_TO_INTERNET
	Create_TCP_connect();
#endif
	while(1)
	{
//		HAL_IWDG_Refresh(&hiwdg);
#if USE_ETH_TO_INTERNET
		ntpTime();
		rebuild_TCP_connect();
#elif USE_4G_UART_TO_INTERNET
		Get_Platform_Time();
		GPRSDataPro();	//处理GPRS数据 等待消息队列阻塞100ms
#endif

		if((HAL_GetTick() - Heart_Tick)>=Herat_Rate*1000)
		{
			Heart_Tick = HAL_GetTick();
//			if(Tcp_Connect_Flag == ConnectOnLine)
			{
				Herat_Data_Send();
			}
		}
		vTaskDelay(100);
	}
}

void CreatPlatformTask(void)
{
	BaseType_t Platform_TaskID;
	
	_myprintf("\r\n CreatPlatformTask");
	Platform_TaskID = xTaskCreate (Platform_Task, "Platform", Platform_StkSIZE, NULL, Platform_TaskPrio, NULL);
	if(pdPASS != Platform_TaskID)
	{
        _myprintf("\r\n CreatPlatformTask creat error");
        while(1);
    }
}
