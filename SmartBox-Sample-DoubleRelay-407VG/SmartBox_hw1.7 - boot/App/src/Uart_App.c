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
            Uart1_TimeOut = 0xFE;
			MSG_GPRSReceiveDataFromISR(Uart1RxBuffer, Uart1_Rx_Cnt);
			Uart1_Rx_Cnt = 0;
			Uart1_TimeOut = 0xF0;
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
			Uart3_Rx_Cnt = 0;
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
            Uart8_TimeOut = 0xFE;
			MSG_GPRSReceiveDataFromISR(Uart8RxBuffer, Uart8_Rx_Cnt);
			Uart8_Rx_Cnt = 0;
			Uart8_TimeOut = 0xF0;
        }
    }
	#endif
	
}
#if UART7_BAUD
void Uart7DataPro(uint8_t *buf, uint16_t len)	//传感器 485串口
{
    static uint16_t wind_speed = 0;
    static uint16_t wind_direction = 0;
    static uint16_t pm2_5 = 0;
    static uint16_t pm10 = 0;
    static uint16_t temperature = 0;
    static uint16_t humidity = 0;
	static Environment_Type environment_data;

    if(buf[0] == 1)
    {
        if(len>=7)
        {
            uint16_t crc_tmp1,crc_tmp2;

            crc_tmp2 = CRC16_calc(buf,5);
            crc_tmp1 = buf[6]<<8 | buf[5];
            if(crc_tmp1 == crc_tmp2)
            {
				uint8_t temp_type;
				temp_type  = 0;
				Environment_device_Type(1,&temp_type);
				Environment_Data_RW(0,&environment_data);
				environment_data.wind_speed = ((buf[3] << 8) | buf[4]) ;
				Environment_Data_RW(1,&environment_data);
                wind_speed = ((buf[3] << 8) | buf[4]) ;
//				myprintf("目前风速：%.1f m/s \n",wind_speed/ 10.0);
                memset(buf,0x00,len); //清空数组
            }
            len = 0;
        }
    }

    else if(buf[0] == 2)
    {
        if(len>=7)
        {
            uint16_t crc_tmp1,crc_tmp2;
            crc_tmp2 = CRC16_calc(buf,5);
            crc_tmp1 = buf[6]<<8 | buf[5];
            if(crc_tmp1 == crc_tmp2)
            {
				Environment_Data_RW(0,&environment_data);
				environment_data.wind_direction = ((buf[3] << 8) | buf[4]) ;
				Environment_Data_RW(1,&environment_data);
                wind_direction = ((buf[3] << 8) | buf[4]) ;
//				myprintf("目前风向：%.1f° \n",wind_direction/ 100.0);
                memset(buf,0x00,len); //清空数组
            }
            len = 0;
        }
    }

    else if(buf[0] == 3)
    {
        if(len>=13)
        {
            static	uint8_t TCP_PAYLOAD[31];
            uint16_t crc_tmp1,crc_tmp2;
            crc_tmp2 = CRC16_calc(buf,11);
            crc_tmp1 = buf[12]<<8 | buf[11];
            if(crc_tmp1 == crc_tmp2)
            {
				Environment_Data_RW(0,&environment_data);
				environment_data.pm2_5 = ((buf[3] << 8) | (buf[4])) ;
                environment_data.pm10 = ((buf[5] << 8) | (buf[6])) ;
                environment_data.temperature = ((buf[7] << 8) | (buf[8])) ;
                environment_data.humidity = ((buf[9] << 8) | (buf[10])) ;
				Environment_Data_RW(1,&environment_data);
                pm2_5 = ((buf[3] << 8) | (buf[4])) ;
                pm10 = ((buf[5] << 8) | (buf[6])) ;
                temperature = ((buf[7] << 8) | (buf[8])) ;
                humidity = ((buf[9] << 8) | (buf[10])) ;
//				myprintf("目前PM2.5：%.1f ug/m3 \n",pm2_5/ 10.0);
//				myprintf("目前PM10：%.1f ug/m3 \n",pm10/ 10.0);
//				myprintf("目前温度：%.1f℃ \n",temperature/ 100.0);
//				myprintf("目前湿度：%.1f%% \n",humidity/ 100.0);
                {
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
#if USE_ETH_TO_INTERNET
					//xQueueSendToBack(TCP_SEND_Queue,TCP_PAYLOAD,0);
#endif
#if USE_4G_UART_TO_INTERNET
                    xQueueSendToBack(GPRS_UART_SEND_Queue,TCP_PAYLOAD,0);
#endif
                }
                memset(buf,0x00,len); //清空数组
            }
            len = 0;
        }
    }
	else if(buf[0] == 0xff && buf[1] == 0x03)	//默认地址 ,03功能码
	{
		static	uint8_t TCP_PAYLOAD[31];
		uint16_t crc_tmp1,crc_tmp2;
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
			if((((buf[19] << 8) | (buf[20]))/100) >= 40)
				environment_data.temperature = (uint16_t)(((float)((buf[19] << 8) | (buf[20]))/100-40)*100);
			else 
				environment_data.temperature = (uint16_t)((40 - (float)(((buf[19] << 8) | (buf[20]))/100))*100)|0x8000;
			environment_data.temperature = (uint16_t)((float)(((buf[19] << 8) | (buf[20]))/100-40)*100);
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
//				myprintf("目前PM2.5：%.1f ug/m3 \n",pm2_5/ 10.0);
//				myprintf("目前PM10：%.1f ug/m3 \n",pm10/ 10.0);
//				myprintf("目前温度：%.1f℃ \n",temperature/ 100.0);
//				myprintf("目前湿度：%.1f%% \n",humidity/ 100.0);
			{
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
#if USE_ETH_TO_INTERNET
				//xQueueSendToBack(TCP_SEND_Queue,TCP_PAYLOAD,0);
#endif
#if USE_4G_UART_TO_INTERNET
				xQueueSendToBack(GPRS_UART_SEND_Queue,TCP_PAYLOAD,0);
#endif
			}
			memset(buf,0x00,len); //清空数组
		}
	}
    else
        len = 0;
}
#endif

#if UART2_BAUD
void Uart2DataPro(uint8_t *buf, uint16_t len)
{
	if(buf[0] == 0xff && buf[1] == 0x03)	//默认地址 ,03功能码
	{
		static Environment_Type environment_data;
		static	uint8_t TCP_PAYLOAD[31];
		uint16_t crc_tmp1,crc_tmp2;
		crc_tmp2 = CRC16_calc(buf,len-2);
		crc_tmp1 = buf[len-1]<<8 | buf[len-2];
		if(crc_tmp1 == crc_tmp2)
		{
			Environment_Data_RW(0,&environment_data);
			environment_data.noise = ((buf[3] << 8) | (buf[4])) ;
			environment_data.SO2 = ((buf[7] << 8) | (buf[8])) ;
			environment_data.NO2 = ((buf[9] << 8) | (buf[10])) ;
			environment_data.CO = ((buf[11] << 8) | (buf[12])) ;
			environment_data.O3 = ((buf[13] << 8) | (buf[14])) ;
			environment_data.pm2_5 = ((buf[15] << 8) | (buf[16])) ;
			environment_data.pm10 = ((buf[17] << 8) | (buf[18])) ;
			environment_data.temperature = (uint16_t)((float)(((buf[19] << 8) | (buf[20]))/100-40)*100);
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
//				myprintf("目前PM2.5：%.1f ug/m3 \n",pm2_5/ 10.0);
//				myprintf("目前PM10：%.1f ug/m3 \n",pm10/ 10.0);
//				myprintf("目前温度：%.1f℃ \n",temperature/ 100.0);
//				myprintf("目前湿度：%.1f%% \n",humidity/ 100.0);
			{
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
#if USE_ETH_TO_INTERNET
				xQueueSendToBack(TCP_SEND_Queue,TCP_PAYLOAD,0);
#endif
#if USE_4G_UART_TO_INTERNET
				xQueueSendToBack(GPRS_UART_SEND_Queue,TCP_PAYLOAD,0);
#endif
			}
			memset(buf,0x00,13); //清空数组
		}
	}
}
#endif

#if UART3_BAUD
void Uart3DataPro(uint8_t *buf, uint16_t len)
{
	IDCofing(buf,  len);
}
#endif

#if UART5_BAUD
void Uart5DataPro(uint8_t *buf, uint16_t len)
{
	IDCofing(buf,  len);
	RemoteUpdata(buf,  len);
}
#endif

#if UART1_BAUD
void Uart1DataPro(uint8_t *buf, uint16_t len)
{
	
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
//				UART4_LEN = Uart1_Rx_Cnt;
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
        vTaskDelay(100);
    }
}

void CreatUartTask(void)
{
	_myprintf("\r\n CreatUartTask");
//	PrintMutex = xSemaphoreCreateMutex();		//Print mutex	
//	if(NULL == PrintMutex)
//	{
//		_myprintf("\r\n PrintMutex error\r\n");
//		while(1);
//	}
    Uart_TaskID = xTaskCreate(Uart_Task, "UartApp", Uart_StkSIZE, NULL, Uart_TaskPrio, &pvCreatedTask_Uart);
    if(pdPASS != Uart_TaskID)
    {
        _myprintf("\r\nTcp_Task creat error");
        while(1);
    }
}

