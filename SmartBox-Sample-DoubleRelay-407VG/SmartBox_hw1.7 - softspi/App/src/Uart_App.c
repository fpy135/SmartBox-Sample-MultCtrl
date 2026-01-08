#include "Uart_App.h"
#include "cmsis_os.h"
#include "task.h"
#include "main.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"
#include "mystring.h"
#include "iwdg.h"

#include "socketserver.h"
#include "IDConfig.h"
#include "Protocol.h"
#include "SysConfig.h"
#include "GPRS_AIR724.h"
#include "Led_App.h"
#include "Environment_data.h"
#include "RemoteUpdata.h"
#include "BL0942.h"

extern QueueHandle_t TCP_SEND_Queue;

BaseType_t Uart_TaskID;
xTaskHandle pvCreatedTask_Uart;


/*
	在定时器中断中调用
*/
void UartTimeOutCnt(void)
{
	#if UART1_BAUD
    if(Uart1_TimeOut < 0xF0)
    {
        if(++Uart1_TimeOut >= UART_TimeOUT_MAX)
        {
            if(++Uart1_TimeOut >= UART_TimeOUT_MAX)
			{
				Uart1_TimeOut = 0xFE;
			}
        }
    }
	#endif
	
	#if UART2_BAUD
	if(Uart2_TimeOut < 0xF0)
    {
        if(++Uart2_TimeOut >= UART_TimeOUT_MAX)
        {
            Uart2_TimeOut = 0xFE;
        }
    }
	#endif
	
	#if UART3_BAUD
	if(Uart3_TimeOut < 0xF0)
    {
        if(++Uart3_TimeOut >= UART_TimeOUT_MAX)
        {
            Uart3_TimeOut = 0xFE;
        }
    }
	#endif
	
	#if UART5_BAUD
    if(Uart5_TimeOut < 0xF0)
    {
        if(++Uart5_TimeOut >= UART_TimeOUT_MAX)
        {
            Uart5_TimeOut = 0xFE;
        }
    }
	#endif
	
	#if UART6_BAUD
	if(Uart6_TimeOut < 0xF0)
    {
        if(++Uart6_TimeOut >= UART_TimeOUT_MAX)
        {
            Uart6_TimeOut = 0xFE;
        }
    }
	#endif
	
	#if UART7_BAUD
	if(Uart7_TimeOut < 0xF0)
    {
        if(++Uart7_TimeOut >= UART_TimeOUT_MAX)
        {
            Uart7_TimeOut = 0xFE;
        }
    }
	#endif
	
	#if UART8_BAUD
	if(Uart8_TimeOut < 0xF0)
    {
        if(++Uart8_TimeOut >= UART_TimeOUT_MAX)
        {
            if(++Uart8_TimeOut >= UART_TimeOUT_MAX)
			{
				Uart8_TimeOut = 0xFE;
			}
        }
    }
	#endif
	
}

#if UART1_BAUD
void Uart1DataPro(uint8_t *buf, uint16_t len)
{
	IDCofing(buf, len);
	RemoteUpdataBy4G(buf, len);
//	BL0942_REF_Set(buf, len);
}
#endif

#if UART2_BAUD
void Uart2DataPro(uint8_t *buf, uint16_t len)
{
	
}
#endif

#if UART3_BAUD
void Uart3DataPro(uint8_t *buf, uint16_t len)	//传感器 485串口
{
}
#endif

#if UART5_BAUD
void Uart5DataPro(uint8_t *buf, uint16_t len)
{
	IDCofing(buf,  len);
	RemoteUpdata(buf,  len);
}
#endif

#if UART6_BAUD
void Uart6DataPro(uint8_t *buf, uint16_t len)	//传感器 485串口
{
	Environment_Type environment_data;
	uint16_t crc_tmp1,crc_tmp2;

    if(buf[0] == 1)
    {
        if(len>=7)
        {
            crc_tmp2 = CRC16_calc(buf,len-2);
            crc_tmp1 = buf[len-1]<<8 | buf[len-2];
            if(crc_tmp1 == crc_tmp2)
            {
				uint8_t temp_type;
				temp_type  = 0;
				Environment_device_Type(1,&temp_type);
				Environment_Data_RW(0,&environment_data);
				environment_data.wind_speed = ((buf[3] << 8) | buf[4]) ;
				Environment_Data_RW(1,&environment_data);
                memset(buf,0x00,len); //清空数组
            }
        }
    }

    else if(buf[0] == 2)
    {
        if(len>=7)
        {
            crc_tmp2 = CRC16_calc(buf,len-2);
            crc_tmp1 = buf[len-1]<<8 | buf[len-2];
            if(crc_tmp1 == crc_tmp2)
            {
				Environment_Data_RW(0,&environment_data);
				environment_data.wind_direction = ((buf[3] << 8) | buf[4]) ;
				Environment_Data_RW(1,&environment_data);
                memset(buf,0x00,len); //清空数组
            }
        }
    }

    else if(buf[0] == 3)
    {
        if(len>=13)
        {
            crc_tmp2 = CRC16_calc(buf,len-2);
            crc_tmp1 = buf[len-1]<<8 | buf[len-2];
			
            if(crc_tmp1 == crc_tmp2)
            {
				Environment_Data_RW(0,&environment_data);
				environment_data.pm2_5 = ((buf[3] << 8) | (buf[4])) ;
                environment_data.pm10 = ((buf[5] << 8) | (buf[6])) ;
                environment_data.temperature = ((buf[7] << 8) | (buf[8])) ;
                environment_data.humidity = ((buf[9] << 8) | (buf[10])) ;
                environment_data.temperature = ((buf[7] << 8) | (buf[8])) ;
				if(environment_data.temperature > 32768)		//负温度
				{
					environment_data.temperature = (environment_data.temperature - 65535) | 0x8000;
				}
				Environment_Data_RW(1,&environment_data);
                memset(buf,0x00,len); //清空数组
            }
        }
    }
	
	else if((buf[0] == 0x0A || buf[0] == 0x0C) && buf[1] == 0x03)	//倾角传感器地址 ,03功能码
	{
		uint16_t tmp_len = 0;
		
		tmp_len = buf[2];
		crc_tmp2 = CRC16_calc(buf,tmp_len+3);
		crc_tmp1 = buf[tmp_len+2+2]<<8 | buf[tmp_len+2+1];
		if(crc_tmp1 == crc_tmp2)
		{
			ANGLE_MTC70M_Type *angle_data;
			int16_t tmp_angle_xy;
			int32_t x,y,z;
			
			myprintf("\r\rangle sensor recv:");
			myprintf_buf_unic(buf, tmp_len+3+2);
			if((angle_data = pvPortMalloc(sizeof(ANGLE_MTC70M_Type))) == NULL)
			{
				myprintf("\r\nUartApp angle_data malloc error");
				return;
			}
			
			x = (buf[6]<<24) | (buf[5]<<16) | (buf[4]<<8) | buf[3];
			y = (buf[10]<<24) | (buf[9]<<16) | (buf[8]<<8) | buf[7];
			z = 0;		//传感器为水平仪无Z轴
			
			x = (x-9000)/10;
			y = (y-9000)/10;
			
			if(buf[0] == 0x0A)
			{
				Angle_Data_RW(0, angle_data);
				if((abs(x-angle_data->angle_x) >= 10) ||\
					(abs(y-angle_data->angle_y) >= 10) ||\
					(abs(z-angle_data->angle_z) >= 10))
				{
					angle_data->angle_x = x;
					angle_data->angle_y = y;
					angle_data->angle_z = z;
					Angle_Data_RW(1, angle_data);
					myprintf("\r\n angle_x:%.2f, angle_y:%.2f, angle_z:%.2f",\
					(float)angle_data->angle_x/10,(float)angle_data->angle_y/10, (float)angle_data->angle_z/10);
					xSemaphoreGive(AngleBinary);		//超过面角差距超过1°，上报数据
				}
			}
			else if(buf[0] == 0x0C)
			{
				ManholeCover_angle_Data_RW(0, 0, angle_data);
				if((abs(x-angle_data->angle_x) >= 10) ||\
					(abs(y-angle_data->angle_y) >= 10) ||\
					(abs(z-angle_data->angle_z) >= 10))
				{
					angle_data->angle_x = x;
					angle_data->angle_y = y;
					angle_data->angle_z = z;
					ManholeCover_angle_Data_RW(1, 0, angle_data);
					myprintf("\r\n ManholeCover_angle_Data: angle_x:%.2f, angle_y:%.2f, angle_z:%.2f",\
					(float)angle_data->angle_x/10,(float)angle_data->angle_y/10, (float)angle_data->angle_z/10);
					xSemaphoreGive(ManholeCoverBinary);		//超过面角差距超过1°，上报数据
				}
			}
			vPortFree(angle_data);
		}
	}
	else if(buf[0] == 0x0B && buf[1] == 0x04)	//空开 传感器地址 ,04功能码
	{
		uint16_t tmp_len = 0;
		int32_t x,y;
		
		tmp_len = buf[2];
		crc_tmp2 = CRC16_calc(buf,tmp_len+3);
		crc_tmp1 = buf[tmp_len+2+2]<<8 | buf[tmp_len+2+1];
		if(crc_tmp1 == crc_tmp2)
		{
			//由于传感器协议是放大100倍，所以缩小10倍处理
			AIRSWITCH_TM_Type * air_switch_data;
			uint16_t switch_sta;
			int32_t active_ele;
			
			myprintf("\r\rn air switch recv:");
			myprintf_buf_unic(buf, tmp_len+3+2);
			
			if((air_switch_data = pvPortMalloc(sizeof(AIRSWITCH_TM_Type))) == NULL)
			{
				myprintf("\r\nUartApp air_switch_data malloc error");
				return;
			}
			AirSwitch_TM_Data_RW(0, (uint8_t *)air_switch_data);
			
			switch_sta = air_switch_data->switch_sta;
			active_ele = air_switch_data->active_ele;
//			air_switch_data->voltage = (buf[9]<<8) | buf[10];
//			air_switch_data->current = (buf[11]<<8) | buf[12];
//			air_switch_data->active_power = (buf[15]<<8) | buf[16];
//			air_switch_data->reactive_power = (buf[19]<<8) | buf[20];
//			air_switch_data->freq = (buf[21]<<8) | buf[22];
//			air_switch_data->active_ele = (buf[25]<<8) | buf[26];
//			air_switch_data->reactive_ele = (buf[29]<<8) | buf[30];
//			air_switch_data->switch_sta = (buf[33]<<8) | buf[34];
//			if(air_switch_data->switch_sta == TM_AIRSWITCH_OPEN)
//			{
//				air_switch_data->switch_sta = 0x00;
//			}
//			else if(air_switch_data->switch_sta == TM_AIRSWITCH_CLOSE)
//			{
//				air_switch_data->switch_sta = 0x01;
//			}
			AirSwitch_TM_Data_RW(1, (uint8_t *)buf+3);
			AirSwitch_TM_Data_RW(0, (uint8_t *)air_switch_data);
			if(air_switch_data->switch_sta != switch_sta)
			{
				xSemaphoreGive(AirSwitchBinary);		//合闸状态变化上报数据
			}
			else if(air_switch_data->active_ele != active_ele)
			{
				xSemaphoreGive(AirSwitchBinary);		//电量变化上报数据
			}
			
			myprintf("\r\nAirSwitch info:\r\nL:%.1f℃, N:%.1f℃, chip:%.1f℃, %.1fV, %.2fA, %.1fW, %.1fkVar, %.2fHz,%.2fkWh, %.2fkVarh, PF:%d, sta:%d, active_reason:%d",\
			(float)HALFWORD_Reverse(air_switch_data->smd_temp_L)/10,(float)HALFWORD_Reverse(air_switch_data->smd_temp_N)/10,\
			(float)HALFWORD_Reverse(air_switch_data->chip_temp)/10,(float)HALFWORD_Reverse(air_switch_data->voltage)/10,\
			(float)HALFWORD_Reverse(air_switch_data->current)/100,(float)WORD_Reverse(air_switch_data->active_power)/10,\
			(float)WORD_Reverse(air_switch_data->reactive_power)/10,(float)HALFWORD_Reverse(air_switch_data->freq)/10,\
			(float)WORD_Reverse(air_switch_data->active_ele)/100,(float)WORD_Reverse(air_switch_data->reactive_ele)/100,\
			HALFWORD_Reverse(air_switch_data->power_factor),HALFWORD_Reverse(air_switch_data->switch_sta),\
			HALFWORD_Reverse(air_switch_data->active_reason));
			vPortFree(air_switch_data);
		}
		
	}
	else if(buf[0] == 0x15 && buf[1] == 0x03)	//耘农温湿度ws301 ,03功能码，杆内温湿度
	{
		crc_tmp2 = CRC16_calc(buf,len-2);
		crc_tmp1 = buf[len-1]<<8 | buf[len-2];
		if(crc_tmp1 == crc_tmp2)
		{
				Environment_Data_RW(0,&environment_data);
				if(((buf[3] << 8) | (buf[4])) >= 4000)		//正温度
	//				environment_data.temperature = (uint16_t)(((float)((buf[19] << 8) | (buf[20]))/100-40)*100);
					environment_data.temperature = (uint16_t)((buf[3] << 8) | buf[4])-4000;
				else 		//负温度
					environment_data.temperature = (uint16_t)(4000 - ((buf[3] << 8) | buf[4])) | 0x8000;
	//				environment_data.temperature = (uint16_t)((40 - (float)(((buf[19] << 8) | (buf[20]))/100))*100)|0x8000;
				//environment_data.temperature = (uint16_t)((float)(((buf[19] << 8) | (buf[20]))/100-40)*100);
				environment_data.humidity = ((buf[5] << 8) | (buf[6])) ;
				environment_data.air_pressure = ((buf[7] << 8) | (buf[8])) ;
//				myprintf("目前温度：%.1f℃ \n",(environment_data.temperature)/ 100.0);
//				myprintf("目前湿度：%.1f%% \n",(environment_data.humidity)/ 100.0);
//				myprintf("目前大气压力：%.1f hPa \n",(environment_data.air_pressure)/ 10.0);
				if(environment_data.air_pressure==0xffff)
					environment_data.air_pressure = 0;
				if(environment_data.CO==0xffff)
					environment_data.CO = 0;
				if(environment_data.CO2==0xffff)
					environment_data.CO2 = 0;
				if(environment_data.humidity==0xffff)
					environment_data.humidity = 0;
				if(environment_data.NO2==0xffff)
					environment_data.NO2 = 0;
				if(environment_data.noise==0xffff)
					environment_data.noise = 0;
				if(environment_data.O3==0xffff)
					environment_data.O3 = 0;
				if(environment_data.pm10==0xffff)
					environment_data.pm10 = 0;
				if(environment_data.pm2_5==0xffff)
					environment_data.pm2_5 = 0;
				if(environment_data.radiation==0xffff)
					environment_data.radiation = 0;
				if(environment_data.rain==0xffff)
					environment_data.rain = 0;
				if(environment_data.SO2==0xffff)
					environment_data.SO2 = 0;
				if(environment_data.sun_power==0xffff)
					environment_data.sun_power = 0;
				if(environment_data.temperature==0xffff)
					environment_data.temperature = 0;
				if(environment_data.ultraviolet==0xffff)
					environment_data.ultraviolet = 0;
				if(environment_data.wind_direction==0xffff)
					environment_data.wind_direction = 0;
				if(environment_data.wind_speed==0xffff)
					environment_data.wind_speed = 0;
				if(environment_data.NAI==0xffff)
					environment_data.NAI = 0;
				Environment_Data_RW(1,&environment_data);
		}
	}
	else if(buf[0] == 0xff && buf[1] == 0x03)	//默认地址 ,03功能码
	{		
		if(len<2)
			return;
		crc_tmp2 = CRC16_calc(buf,len-2);
		crc_tmp1 = buf[len-1]<<8 | buf[len-2];
		if(crc_tmp1 == crc_tmp2)
		{
			uint8_t temp_type;
			
			if(buf[2]<=6)
			{
				temp_type  = 2;
				Environment_device_Type(1,&temp_type);
				Environment_Data_RW(0,&environment_data);
				if(((buf[3] << 8) | (buf[4])) >= 4000)		//正温度
	//				environment_data.temperature = (uint16_t)(((float)((buf[19] << 8) | (buf[20]))/100-40)*100);
					environment_data.temperature = (uint16_t)((buf[3] << 8) | buf[4])-4000;
				else 		//负温度
					environment_data.temperature = (uint16_t)(4000 - ((buf[3] << 8) | buf[4])) | 0x8000;
	//				environment_data.temperature = (uint16_t)((40 - (float)(((buf[19] << 8) | (buf[20]))/100))*100)|0x8000;
				//environment_data.temperature = (uint16_t)((float)(((buf[19] << 8) | (buf[20]))/100-40)*100);
				environment_data.humidity = ((buf[5] << 8) | (buf[6])) ;
				environment_data.air_pressure = ((buf[7] << 8) | (buf[8])) ;
//				myprintf("目前温度：%.1f℃ \n",(environment_data.temperature)/ 100.0);
//				myprintf("目前湿度：%.1f%% \n",(environment_data.humidity)/ 100.0);
//				myprintf("目前大气压力：%.1f hPa \n",(environment_data.air_pressure)/ 10.0);
				if(environment_data.air_pressure==0xffff)
					environment_data.air_pressure = 0;
				if(environment_data.CO==0xffff)
					environment_data.CO = 0;
				if(environment_data.CO2==0xffff)
					environment_data.CO2 = 0;
				if(environment_data.humidity==0xffff)
					environment_data.humidity = 0;
				if(environment_data.NO2==0xffff)
					environment_data.NO2 = 0;
				if(environment_data.noise==0xffff)
					environment_data.noise = 0;
				if(environment_data.O3==0xffff)
					environment_data.O3 = 0;
				if(environment_data.pm10==0xffff)
					environment_data.pm10 = 0;
				if(environment_data.pm2_5==0xffff)
					environment_data.pm2_5 = 0;
				if(environment_data.radiation==0xffff)
					environment_data.radiation = 0;
				if(environment_data.rain==0xffff)
					environment_data.rain = 0;
				if(environment_data.SO2==0xffff)
					environment_data.SO2 = 0;
				if(environment_data.sun_power==0xffff)
					environment_data.sun_power = 0;
				if(environment_data.temperature==0xffff)
					environment_data.temperature = 0;
				if(environment_data.ultraviolet==0xffff)
					environment_data.ultraviolet = 0;
				if(environment_data.wind_direction==0xffff)
					environment_data.wind_direction = 0;
				if(environment_data.wind_speed==0xffff)
					environment_data.wind_speed = 0;
				if(environment_data.NAI==0xffff)
					environment_data.NAI = 0;
				Environment_Data_RW(1,&environment_data);
			}
			else
			{
				temp_type  = 1;
				Environment_device_Type(1,&temp_type);
				Environment_Data_RW(0,&environment_data);
				environment_data.noise = ((buf[3] << 8) | (buf[4])) ;
				environment_data.SO2 = ((buf[7] << 8) | (buf[8])) ;
				environment_data.NO2 = ((buf[9] << 8) | (buf[10])) ;
				environment_data.CO = ((buf[11] << 8) | (buf[12])) ;
				environment_data.O3 = ((buf[13] << 8) | (buf[14])) ;
				environment_data.pm2_5 = ((buf[15] << 8) | (buf[16])) ;
				environment_data.pm10 = ((buf[17] << 8) | (buf[18])) ;
				if(((buf[19] << 8) | (buf[20])) >= 4000)		//正温度
	//				environment_data.temperature = (uint16_t)(((float)((buf[19] << 8) | (buf[20]))/100-40)*100);
					environment_data.temperature = (uint16_t)((buf[19] << 8) | buf[20])-4000;
				else 		//负温度
					environment_data.temperature = (uint16_t)(4000 - ((buf[19] << 8) | buf[20])) | 0x8000;
	//				environment_data.temperature = (uint16_t)((40 - (float)(((buf[19] << 8) | (buf[20]))/100))*100)|0x8000;
				//environment_data.temperature = (uint16_t)((float)(((buf[19] << 8) | (buf[20]))/100-40)*100);
				environment_data.humidity = ((buf[21] << 8) | (buf[22])) ;
				environment_data.air_pressure = ((buf[23] << 8) | (buf[24])) ;
				environment_data.wind_speed = ((buf[25] << 8) | (buf[26])) ;
				environment_data.wind_direction = ((buf[27] << 8) | (buf[28])) ;
				environment_data.rain = ((buf[29] << 8) | (buf[30])) ;
				environment_data.radiation = ((buf[31] << 8) | (buf[32])) ;
				environment_data.sun_power = ((buf[33] << 8) | (buf[34])) ;
				environment_data.ultraviolet = ((buf[35] << 8) | (buf[36])) ;
				environment_data.CO2 = ((buf[37] << 8) | (buf[38])) ;
				environment_data.NAI = ((buf[39] << 8) | (buf[40])) ;
				
				
				if(environment_data.air_pressure==0xffff)
					environment_data.air_pressure = 0;
				if(environment_data.CO==0xffff)
					environment_data.CO = 0;
				if(environment_data.CO2==0xffff)
					environment_data.CO2 = 0;
				if(environment_data.humidity==0xffff)
					environment_data.humidity = 0;
				if(environment_data.NO2==0xffff)
					environment_data.NO2 = 0;
				if(environment_data.noise==0xffff)
					environment_data.noise = 0;
				if(environment_data.O3==0xffff)
					environment_data.O3 = 0;
				if(environment_data.pm10==0xffff)
					environment_data.pm10 = 0;
				if(environment_data.pm2_5==0xffff)
					environment_data.pm2_5 = 0;
				if(environment_data.radiation==0xffff)
					environment_data.radiation = 0;
				if(environment_data.rain==0xffff)
					environment_data.rain = 0;
				if(environment_data.SO2==0xffff)
					environment_data.SO2 = 0;
				if(environment_data.sun_power==0xffff)
					environment_data.sun_power = 0;
				if(environment_data.temperature==0xffff)
					environment_data.temperature = 0;
				if(environment_data.ultraviolet==0xffff)
					environment_data.ultraviolet = 0;
				if(environment_data.wind_direction==0xffff)
					environment_data.wind_direction = 0;
				if(environment_data.wind_speed==0xffff)
					environment_data.wind_speed = 0;
				if(environment_data.NAI==0xffff)
					environment_data.NAI = 0;
				Environment_Data_RW(1,&environment_data);
				
			}
			memset(buf,0x00,len); //清空数组
		}
	}
}
#endif

#if UART7_BAUD
void Uart7DataPro(uint8_t *buf, uint16_t len)
{
	Environment_Type environment_data;
	uint16_t crc_tmp1,crc_tmp2;

    if(buf[0] == 1)
    {
        if(len>=7)
        {
            crc_tmp2 = CRC16_calc(buf,len-2);
            crc_tmp1 = buf[len-1]<<8 | buf[len-2];
            if(crc_tmp1 == crc_tmp2)
            {
				uint8_t temp_type;
				temp_type  = 0;
				Environment_device_Type(1,&temp_type);
				Environment_Data_RW(0,&environment_data);
				environment_data.wind_speed = ((buf[3] << 8) | buf[4]) ;
				Environment_Data_RW(1,&environment_data);
                memset(buf,0x00,len); //清空数组
            }
        }
    }

    else if(buf[0] == 2)
    {
        if(len>=7)
        {
            crc_tmp2 = CRC16_calc(buf,len-2);
            crc_tmp1 = buf[len-1]<<8 | buf[len-2];
            if(crc_tmp1 == crc_tmp2)
            {
				Environment_Data_RW(0,&environment_data);
				environment_data.wind_direction = ((buf[3] << 8) | buf[4]) ;
				Environment_Data_RW(1,&environment_data);
                memset(buf,0x00,len); //清空数组
            }
        }
    }

    else if(buf[0] == 3)
    {
        if(len>=13)
        {
            crc_tmp2 = CRC16_calc(buf,len-2);
            crc_tmp1 = buf[len-1]<<8 | buf[len-2];
			
            if(crc_tmp1 == crc_tmp2)
            {
				Environment_Data_RW(0,&environment_data);
				environment_data.pm2_5 = ((buf[3] << 8) | (buf[4])) ;
                environment_data.pm10 = ((buf[5] << 8) | (buf[6])) ;
                environment_data.temperature = ((buf[7] << 8) | (buf[8])) ;
                environment_data.humidity = ((buf[9] << 8) | (buf[10])) ;
                environment_data.temperature = ((buf[7] << 8) | (buf[8])) ;
				if(environment_data.temperature > 32768)		//负温度
				{
					environment_data.temperature = (environment_data.temperature - 65535) | 0x8000;
				}
				Environment_Data_RW(1,&environment_data);
                memset(buf,0x00,len); //清空数组
            }
        }
    }
	else if(buf[0] == 0xff && buf[1] == 0x03)	//默认地址 ,03功能码
	{		
		if(len<2)
			return;
		crc_tmp2 = CRC16_calc(buf,len-2);
		crc_tmp1 = buf[len-1]<<8 | buf[len-2];
		if(crc_tmp1 == crc_tmp2)
		{
			uint8_t temp_type;
			temp_type  = 1;
			Environment_device_Type(1,&temp_type);
			Environment_Data_RW(0,&environment_data);
			environment_data.noise = ((buf[3] << 8) | (buf[4])) ;
			environment_data.SO2 = ((buf[7] << 8) | (buf[8])) ;
			environment_data.NO2 = ((buf[9] << 8) | (buf[10])) ;
			environment_data.CO = ((buf[11] << 8) | (buf[12])) ;
			environment_data.O3 = ((buf[13] << 8) | (buf[14])) ;
			environment_data.pm2_5 = ((buf[15] << 8) | (buf[16])) ;
			environment_data.pm10 = ((buf[17] << 8) | (buf[18])) ;
			if((((buf[19] << 8) | (buf[20]))/100) >= 40)		//正温度
				environment_data.temperature = (uint16_t)(((float)((buf[19] << 8) | (buf[20]))/100-40)*100);
			else 		//负温度
				environment_data.temperature = (uint16_t)((40 - (float)(((buf[19] << 8) | (buf[20]))/100))*100)|0x8000;
			//environment_data.temperature = (uint16_t)((float)(((buf[19] << 8) | (buf[20]))/100-40)*100);
			environment_data.humidity = ((buf[21] << 8) | (buf[22])) ;
			environment_data.air_pressure = ((buf[23] << 8) | (buf[24])) ;
			environment_data.wind_speed = ((buf[25] << 8) | (buf[26])) ;
			environment_data.wind_direction = ((buf[27] << 8) | (buf[28])) ;
			environment_data.rain = ((buf[29] << 8) | (buf[30])) ;
			environment_data.radiation = ((buf[31] << 8) | (buf[32])) ;
			environment_data.sun_power = ((buf[33] << 8) | (buf[34])) ;
			environment_data.ultraviolet = ((buf[35] << 8) | (buf[36])) ;
			environment_data.CO2 = ((buf[37] << 8) | (buf[38])) ;
			Environment_Data_RW(1,&environment_data);
			memset(buf,0x00,len); //清空数组
		}
	}
}
#endif

#if UART8_BAUD
void Uart8DataPro(uint8_t *buf, uint16_t len)
{
	uint8_t tmp[4];
	if(len>4)
		len = 4;
	memcpy(tmp,buf,len);
	if(xQueueSend(BL6523GX_RECEIVE_Queue, tmp, 0) != pdPASS)
	{
		myprintf("\r\nxQueueSend error, BL6523GX_RECEIVE_Queue");
	}
}
#endif


void Uart_Task(void * argument)
{
    while(1)
    {
		#if UART1_BAUD
        if(Uart1_TimeOut == 0xFE)
        {
            if(Uart1_Rx_Cnt)     //有数据
            {
                Uart1DataPro(Uart1RxBuffer, Uart1_Rx_Cnt);
                Uart1_Rx_Cnt = 0;
            }
            Uart1_TimeOut = 0xF0;
        }
		#endif
		
		#if UART2_BAUD
		if(Uart2_TimeOut == 0xFE)
        {
            if(Uart2_Rx_Cnt)     //有数据
            {
                Uart2DataPro(Uart2RxBuffer, Uart2_Rx_Cnt);
                Uart2_Rx_Cnt = 0;
            }
            Uart2_TimeOut = 0xF0;
        }
		#endif
		
		#if UART3_BAUD
		if(Uart3_TimeOut == 0xFE)
        {
            if(Uart3_Rx_Cnt)     //有数据
            {
                Uart3DataPro(Uart3RxBuffer, Uart3_Rx_Cnt);
                Uart3_Rx_Cnt = 0;
            }
            Uart3_TimeOut = 0xF0;
        }
		#endif
		
		#if UART5_BAUD
        if(Uart5_TimeOut == 0xFE)
        {
            if(Uart5_Rx_Cnt)     //有数据
            {
//				UART4_LEN = Uart5_Rx_Cnt;
                Uart5DataPro(Uart5RxBuffer, Uart5_Rx_Cnt);
                Uart5_Rx_Cnt = 0;
            }
            Uart5_TimeOut = 0xF0;
        }
		#endif
		
		#if UART6_BAUD
        if(Uart6_TimeOut == 0xFE)
        {
            if(Uart6_Rx_Cnt)     //有数据
            {
                Uart6DataPro(Uart6RxBuffer, Uart6_Rx_Cnt);
                Uart6_Rx_Cnt = 0;
            }
            Uart6_TimeOut = 0xF0;
        }
		#endif
		
		#if UART7_BAUD
        if(Uart7_TimeOut == 0xFE)
        {
            if(Uart7_Rx_Cnt)     //有数据
            {
                Uart7DataPro(Uart7RxBuffer, Uart7_Rx_Cnt);
                Uart7_Rx_Cnt = 0;
            }
            Uart7_TimeOut = 0xF0;
        }
		#endif
		#if UART8_BAUD
        if(Uart8_TimeOut == 0xFE)
        {
            if(Uart8_Rx_Cnt)     //有数据
            {
                Uart8DataPro(Uart8RxBuffer, Uart8_Rx_Cnt);
                Uart8_Rx_Cnt = 0;
            }
            Uart8_TimeOut = 0xF0;
        }
		#endif
        vTaskDelay(10);
    }
}

void CreatUartTask(void)
{
	myprintf("\r\n CreatUartTask");
    Uart_TaskID = xTaskCreate(Uart_Task, "UartApp", Uart_StkSIZE, NULL, Uart_TaskPrio, &pvCreatedTask_Uart);
    if(pdPASS != Uart_TaskID)
    {
        myprintf("\r\nTcp_Task creat error");
        while(1);
    }
}

