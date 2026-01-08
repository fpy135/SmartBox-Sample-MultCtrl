#include "Led_App.h"
#include "SysConfig.h"
#include "Usart.h"
#include "rtc.h"
#include "Unixtimer.h"
#include "Environment_data.h"
#include "iwdg.h"
#include "mystring.h"
#include "BL0942.h"
#include "IDConfig.h"
#include "socketserver.h"
#include "TempAdc_APP.h"


extern void GetRTC(rtc_time_t *rtc_time);
extern void SetRTC(rtc_time_t *rtc_time);
extern void SetRTC_BCD(rtc_time_t *rtc_time);
extern void GetRTC_BCD(rtc_time_t *rtc_time);
void CheckSelf_Flag_Rw(uint8_t rw, uint8_t * checkflag);

SemaphoreHandle_t LedShowBinary;	//演示模式信号量
SemaphoreHandle_t TimeControlMutex; //TimeControlMutex
SemaphoreHandle_t ElectricDataMutex; //ElectricDataMutex

TimeControl_Type TimeControl_Data[LED_NUM];	//时控模式结构体

xTaskHandle pvCreatedTask_Led;

uint8_t targetBrightness = 0;

uint8_t ledshowFlag __no_init;		//演示模式
uint8_t LedStatus[LED_NUM] __no_init; 		//led1继电器

uint8_t LedPwm[LED_NUM] __no_init;
#if CUSTOM_DEV_NUM
TimeCustomControl_Type TimeCustomCtrl_Data;		//自定义设备时控
TimeCustomControl_Type TimeCustomCtrl_Data2;	//自定义设备时控
uint8_t CustomStatus1 __no_init; 	//自定义设备
uint8_t CustomStatus2 __no_init; 	//自定义设备
uint8_t Get_Timecontrol_Unix_Flag3 = 0;
uint8_t Get_Timecontrol_Unix_Flag4 = 0;
uint32_t Timectrl_Unix_Time3 = 0;
uint32_t Timectrl_Unix_Time4 = 0;
#endif

extern void Get_ElectricData(uint8_t channle, CollectionData *electricdata);
extern void Write_ElectricData(uint8_t channle, CollectionData *electricdata);


void Get_ElectricData(uint8_t channle, CollectionData *electricdata)
{
	xSemaphoreTake (ElectricDataMutex, portMAX_DELAY);
	
//	if(channle == 1)
	{
		electricdata->Voltage = ElectricData[channle-1].Voltage;
		electricdata->RelayState = LedStatus[channle-1];
		electricdata->LedPwm = LedPwm[channle-1];
		electricdata->Power = ElectricData[channle-1].Power;
		electricdata->Energy = ElectricData[channle-1].Energy;
		electricdata->Current = ElectricData[channle-1].Current;
		electricdata->Bl6526bState = ElectricData[channle-1].Bl6526bState;
	}
//    else if(channle == 2)
//    {
//		electricdata->Voltage = ElectricData[1].Voltage;
//		electricdata->RelayState = LedStatus2;
//		electricdata->LedPwm = LedPwm2;
//		electricdata->Power = ElectricData[1].Power;
//		electricdata->Energy = ElectricData[1].Energy;
//		electricdata->Current = ElectricData[1].Current;
//		electricdata->Bl6526bState = ElectricData[1].Bl6526bState;
//    }

    xSemaphoreGive (ElectricDataMutex);
}

void Write_ElectricData(uint8_t channle, CollectionData *electricdata)
{
    xSemaphoreTake (ElectricDataMutex, portMAX_DELAY);
//    if(channle == 1)
    {
		ElectricData[channle-1].Voltage = electricdata->Voltage;
		ElectricData[channle-1].RelayState = LedStatus[channle-1];
		ElectricData[channle-1].LedPwm = LedPwm[channle-1];
		ElectricData[channle-1].Power = electricdata->Power;
		ElectricData[channle-1].Energy = SaveEnergy[channle-1] + electricdata->Energy;
		ElectricData[channle-1].Current = electricdata->Current;
		ElectricData[channle-1].Bl6526bState = electricdata->Bl6526bState;
    }
//    if(channle == 2)
//    {
//		ElectricData[1].Voltage = electricdata->Voltage;
//		ElectricData[1].RelayState = LedStatus2;
//		ElectricData[1].LedPwm = LedPwm2;
//		ElectricData[1].Power = electricdata->Power;
//		ElectricData[1].Energy = SaveEnergy[channle-1] + electricdata->Energy;
//		ElectricData[1].Current = electricdata->Current;
//		ElectricData[1].Bl6526bState = electricdata->Bl6526bState;
//    }
    xSemaphoreGive (ElectricDataMutex);
}

uint8_t Get_TimeControl_Data(uint8_t channle, TimeControl_Type *timecontrol_data)
{
    uint8_t flag = 0;
    xSemaphoreTake (TimeControlMutex, portMAX_DELAY);
//    if(channle == 1)
//    {
//        timecontrol_data->start_hour = TimeControl_Data[channle-1].start_hour;
//        timecontrol_data->start_min = TimeControl_Data[channle-1].start_min;
//        timecontrol_data->phase1_Pwm = TimeControl_Data[channle-1].phase1_Pwm;
//        timecontrol_data->phase1_time = TimeControl_Data[channle-1].phase1_time;
//        timecontrol_data->phase2_Pwm = TimeControl_Data[channle-1].phase2_Pwm;
//        timecontrol_data->phase2_time = TimeControl_Data[channle-1].phase2_time;
//        timecontrol_data->phase3_Pwm = TimeControl_Data[channle-1].phase3_Pwm;
//        timecontrol_data->phase3_time = TimeControl_Data[channle-1].phase3_time;
//        timecontrol_data->phase4_Pwm = TimeControl_Data[channle-1].phase4_Pwm;
//        timecontrol_data->phase4_time = TimeControl_Data[channle-1].phase4_time;
//    }
//    else if(channle == 2)
//    {
//        timecontrol_data->start_hour = TimeControl_Data2.start_hour;
//        timecontrol_data->start_min = TimeControl_Data2.start_min;
//        timecontrol_data->phase1_Pwm = TimeControl_Data2.phase1_Pwm;
//        timecontrol_data->phase1_time = TimeControl_Data2.phase1_time;
//        timecontrol_data->phase2_Pwm = TimeControl_Data2.phase2_Pwm;
//        timecontrol_data->phase2_time = TimeControl_Data2.phase2_time;
//        timecontrol_data->phase3_Pwm = TimeControl_Data2.phase3_Pwm;
//        timecontrol_data->phase3_time = TimeControl_Data2.phase3_time;
//        timecontrol_data->phase4_Pwm = TimeControl_Data2.phase4_Pwm;
//        timecontrol_data->phase4_time = TimeControl_Data2.phase4_time;
//        flag = Get_Timecontrol_Unix_Flag2;
//    }
	memcpy(timecontrol_data, &TimeControl_Data[channle-1], sizeof(TimeControl_Type));
    xSemaphoreGive (TimeControlMutex);
    return flag;
}

uint8_t Write_TimeControl_Data(uint8_t channle, TimeControl_Type *timecontrol_data)
{
	uint8_t ret = 0;
    xSemaphoreTake (TimeControlMutex, portMAX_DELAY);

//    if(channle == 1)
//    {
//        TimeControl_Data[channle-1].start_hour = timecontrol_data->start_hour;
//        TimeControl_Data[channle-1].start_min = timecontrol_data->start_min;
//        TimeControl_Data[channle-1].phase1_Pwm = timecontrol_data->phase1_Pwm;
//        TimeControl_Data[channle-1].phase1_time = timecontrol_data->phase1_time;
//        TimeControl_Data[channle-1].phase2_Pwm = timecontrol_data->phase2_Pwm;
//        TimeControl_Data[channle-1].phase2_time = timecontrol_data->phase2_time;
//        TimeControl_Data[channle-1].phase3_Pwm = timecontrol_data->phase3_Pwm;
//        TimeControl_Data[channle-1].phase3_time = timecontrol_data->phase3_time;
//        TimeControl_Data[channle-1].phase4_Pwm = timecontrol_data->phase4_Pwm;
//        TimeControl_Data[channle-1].phase4_time = timecontrol_data->phase4_time;
//    }
	
//    else if(channle == 2)
//    {
//        TimeControl_Data2.start_hour = timecontrol_data->start_hour;
//        TimeControl_Data2.start_min = timecontrol_data->start_min;
//        TimeControl_Data2.phase1_Pwm = timecontrol_data->phase1_Pwm;
//        TimeControl_Data2.phase1_time = timecontrol_data->phase1_time;
//        TimeControl_Data2.phase2_Pwm = timecontrol_data->phase2_Pwm;
//        TimeControl_Data2.phase2_time = timecontrol_data->phase2_time;
//        TimeControl_Data2.phase3_Pwm = timecontrol_data->phase3_Pwm;
//        TimeControl_Data2.phase3_time = timecontrol_data->phase3_time;
//        TimeControl_Data2.phase4_Pwm = timecontrol_data->phase4_Pwm;
//        TimeControl_Data2.phase4_time = timecontrol_data->phase4_time;
//        Get_Timecontrol_Unix_Flag2 = 0;
//    }
	if(memcmp(timecontrol_data, &TimeControl_Data[channle-1], sizeof(TimeControl_Type)) != 0)
	{
		ret = 1;
		memcpy(&TimeControl_Data[channle-1], timecontrol_data, sizeof(TimeControl_Type));
	}
    xSemaphoreGive (TimeControlMutex);
	return ret;
}

#if CUSTOM_DEV_NUM
void Write_TimeCustomCtrl_Data(uint8_t channle, TimeCustomControl_Type *timecontrol_data)
{
	xSemaphoreTake (TimeControlMutex, portMAX_DELAY);
	if(channle == 1)
	{
		TimeCustomCtrl_Data.start_hour = timecontrol_data->start_hour;
		TimeCustomCtrl_Data.start_min = timecontrol_data->start_min;
		TimeCustomCtrl_Data.led_sta1 = timecontrol_data->led_sta1;
		TimeCustomCtrl_Data.phase1_time = timecontrol_data->phase1_time;
		TimeCustomCtrl_Data.led_sta2 = timecontrol_data->led_sta2;
		TimeCustomCtrl_Data.phase2_time = timecontrol_data->phase2_time;
		TimeCustomCtrl_Data.led_sta3 = timecontrol_data->led_sta3;
		TimeCustomCtrl_Data.phase3_time = timecontrol_data->phase3_time;
		TimeCustomCtrl_Data.led_sta4 = timecontrol_data->led_sta4;
		TimeCustomCtrl_Data.phase4_time = timecontrol_data->phase4_time;
		Get_Timecontrol_Unix_Flag3 = 0;
	}
	else if(channle == 2)
	{
		TimeCustomCtrl_Data2.start_hour = timecontrol_data->start_hour;
		TimeCustomCtrl_Data2.start_min = timecontrol_data->start_min;
		TimeCustomCtrl_Data2.led_sta1 = timecontrol_data->led_sta1;
		TimeCustomCtrl_Data2.phase1_time = timecontrol_data->phase1_time;
		TimeCustomCtrl_Data2.led_sta2 = timecontrol_data->led_sta2;
		TimeCustomCtrl_Data2.phase2_time = timecontrol_data->phase2_time;
		TimeCustomCtrl_Data2.led_sta3 = timecontrol_data->led_sta3;
		TimeCustomCtrl_Data2.phase3_time = timecontrol_data->phase3_time;
		TimeCustomCtrl_Data2.led_sta4 = timecontrol_data->led_sta4;
		TimeCustomCtrl_Data2.phase4_time = timecontrol_data->phase4_time;
		Get_Timecontrol_Unix_Flag4 = 0;
	}
	xSemaphoreGive (TimeControlMutex);
}
#endif

void Get_LED_Data(uint8_t led_num, uint8_t *ledstatus, uint8_t *ledpwm)
{
	taskENTER_CRITICAL();
//	if(led_num == 1)
	{
		*ledstatus = LedStatus[led_num-1];
		*ledpwm = LedPwm[led_num-1];
	}
//	else if(led_num == 2)
//    {
//        *ledstatus = LedStatus2;
//        *ledpwm = LedPwm2;
//    }
	taskEXIT_CRITICAL();
}

void Write_LED_Data(uint8_t led_num, uint8_t *ledstatus, uint8_t *ledpwm)
{
	taskENTER_CRITICAL();
//	if(led_num == 1)
	{
		LedStatus[led_num-1] = *ledstatus;
		LedPwm[led_num-1] = *ledpwm;
		REL_Write(led_num, LedStatus[led_num-1]);	//打开继电器电源
//		if(LedStatus)
//		{
//			REL1_Write(REL_ON);	//打开继电器电源
//		}
//		else
//		{
//			REL1_Write(REL_OFF);	//关闭继电器电源
//		}
	}
//	else if(led_num == 2)
//	{
//		LedStatus2 = *ledstatus;
//		LedPwm2 = *ledpwm;
//		REL2_Write(LedStatus2);	//打开继电器电源
////		if(LedStatus2)
////		{
////			REL2_Write(REL_ON);	//打开继电器电源
////		}
////		else
////		{
////			REL2_Write(REL_OFF);	//关闭继电器电源
////		}
//	}

	taskEXIT_CRITICAL();
}

#if HARDWARE_PWM
void Led_pwm_change(uint8_t ledpwm)
{
	static uint8_t temp_led_pwm = 0xff;
	if(temp_led_pwm != ledpwm)
	{
		temp_led_pwm = ledpwm;
		TIM_SetTIM3Compare2(ledpwm);
	}
	return;
}
#else
/******************************
功能：产生 pwm 波
用法：定时器中断内调用
注意：该版本仅适合单灯控制
******************************/
void Led_Pwm_Control(void)
{
    static uint8_t led_cnt = 1;
	
	
    if(led_cnt>LedPwm[0])
    {
		Led1_Write(0);
    }
    else
    {
		Led1_Write(1);
    }
#if LED_NUM >=2
	if(led_cnt>LedPwm[1])
    {
		Led2_Write(0);
    }
    else
    {
		Led2_Write(1);
    }
#endif
    led_cnt++;
    if(led_cnt>=100)
    {
        led_cnt = 1;
    }
}

/******************************
功能：调整LED亮度
用法：使亮度调节变得丝滑,在定时器中断中调用
******************************/
/*
void Switch_Brightness(void)
{
    if(targetBrightness > LedPwm)
    {
        LedPwm++;
    }
    else if (targetBrightness < LedPwm)
    {
        LedPwm--;
    }
    else
    {
        LedPwm = targetBrightness;
    }
}
*/
#endif
void Led_Strategy_new(uint8_t lednum)
{
	rtc_time_t rtc_time;
	rtc_time_t temp_time;
	static TimeControl_Type timecontrol_data;
	uint32_t rtc_unix_time = 0;
	static uint32_t timecontro_unix_time = 0;
	uint8_t ledpwm,ledsta;
	
	GetRTC(&rtc_time);
	if(rtc_time.ui8Year < 2020)		//未获取到时间，不执行时间管理
		return;
	rtc_unix_time = covBeijing2UnixTimeStp(&rtc_time);
	if(lednum <= LED_NUM)
	{
		Get_TimeControl_Data(lednum, &timecontrol_data);
	
	}
	else
	{
		#if CUSTOM_DEV_NUM
		get_timecontrol_unix_flag = Get_TimeCustomCtrl_Data(lednum - LED_NUM, (TimeCustomControl_Type *)&timecontrol_data);
		#endif
	
	}
//	Get_TimeControl_flag(lednum, &get_timecontrol_unix_flag, &timecontro_unix_time);
	//每天时间到达第一个策略开始时间时更新时控策略的Unix时间戳
	if((rtc_time.ui8Hour == (timecontrol_data.start_hour) && \
		rtc_time.ui8Minute >= (timecontrol_data.start_min)) || \
		rtc_time.ui8Hour > (timecontrol_data.start_hour))
	{
		Get_TimeControl_Data(lednum, &timecontrol_data);
		temp_time.ui8Year = rtc_time.ui8Year;
		temp_time.ui8Month = rtc_time.ui8Month;
		temp_time.ui8DayOfMonth = rtc_time.ui8DayOfMonth;
		temp_time.ui8Hour = timecontrol_data.start_hour;
		temp_time.ui8Minute = timecontrol_data.start_min;
		temp_time.ui8Second = 0;
		timecontro_unix_time = covBeijing2UnixTimeStp(&temp_time);
//		Write_TimeControl_flag(lednum, &get_timecontrol_unix_flag, &timecontro_unix_time);
	}
	else
	{
		Get_TimeControl_Data(lednum, &timecontrol_data);
		temp_time.ui8Year = rtc_time.ui8Year;
		temp_time.ui8Month = rtc_time.ui8Month;
		temp_time.ui8DayOfMonth = rtc_time.ui8DayOfMonth;
		temp_time.ui8Hour = timecontrol_data.start_hour;
		temp_time.ui8Minute = timecontrol_data.start_min;
		temp_time.ui8Second = 0;
		timecontro_unix_time = covBeijing2UnixTimeStp(&temp_time);
		timecontro_unix_time -= 86400;	//时间未到当天的开始时间则用前一天的时间戳
//		Write_TimeControl_flag(lednum, &get_timecontrol_unix_flag, &timecontro_unix_time);
	}
    //如果时间超过了策略的初始时间
    if(rtc_unix_time >= timecontro_unix_time)
    {
        if((rtc_unix_time-timecontro_unix_time) <= timecontrol_data.phase1_time*60)	//阶段一
        {
            if(lednum <= LED_NUM)
            {
                ledpwm = timecontrol_data.phase1_Pwm;
                if(ledpwm <= 0)
                    ledsta = LEDOFF;
                else
                    ledsta = LEDON;
            }
            else
            {
                ledsta = timecontrol_data.phase1_Pwm;	//由于之前的格式转换，这个位置实际储存灯带的开关状态
            }
            myprintf("\r\n灯%d时控阶段一",lednum);
        }
        else if((rtc_unix_time-timecontro_unix_time) <= \
                (timecontrol_data.phase1_time+timecontrol_data.phase2_time)*60)	//阶段二
        {
            if(lednum <= LED_NUM)
            {
                ledpwm = timecontrol_data.phase2_Pwm;
                if(ledpwm <= 0)
                    ledsta = LEDOFF;
                else
                    ledsta = LEDON;
            }
            else
            {
                ledsta = timecontrol_data.phase2_Pwm;	//由于之前的格式转换，这个位置实际储存灯带的开关状态
            }
            myprintf("\r\n灯%d时控阶段二",lednum);
        }
        else if((rtc_unix_time-timecontro_unix_time) <= \
                (timecontrol_data.phase1_time+timecontrol_data.phase2_time+\
                 timecontrol_data.phase3_time)*60)	//阶段三
        {
            if(lednum <= LED_NUM)
            {
                ledpwm = timecontrol_data.phase3_Pwm;
                if(ledpwm <= 0)
                    ledsta = LEDOFF;
                else
                    ledsta = LEDON;
            }
            else
            {
                ledsta = timecontrol_data.phase3_Pwm;	//由于之前的格式转换，这个位置实际储存灯带的开关状态
            }
            myprintf("\r\n灯%d时控阶段三",lednum);
        }
        else if((rtc_unix_time-timecontro_unix_time) <= \
                (timecontrol_data.phase1_time+timecontrol_data.phase2_time+\
                 timecontrol_data.phase3_time+timecontrol_data.phase4_time)*60)	//阶段四
        {
            if(lednum <= LED_NUM)
            {
                ledpwm = timecontrol_data.phase4_Pwm;
                if(ledpwm <= 0)
                    ledsta = LEDOFF;
                else
                    ledsta = LEDON;
            }
            else
            {
                ledsta = timecontrol_data.phase4_Pwm;	//由于之前的格式转换，这个位置实际储存灯带的开关状态
            }
            myprintf("\r\n灯%d时控阶段四",lednum);
        }
        else	//策略模式亮灯结束
        {
            if(lednum <= LED_NUM)
            {
                ledsta = LEDOFF;
                ledpwm = 0;
            }
            else
            {
                ledsta = LEDOFF;
            }
//            get_timecontrol_unix_flag = 0;		//重新获取新的时间戳
//            Write_TimeControl_flag(lednum, &get_timecontrol_unix_flag, &timecontro_unix_time);
            myprintf("\r\n灯%d时控结束",lednum);
        }
    }
    else		//未到策略时间，则关灯
    {
        ledsta = LEDOFF;
        ledpwm = 0;
    }
    Write_LED_Data(lednum, &ledsta, &ledpwm);

}

/*
static uint8_t CheckSelf(void)
{
	uint8_t check_flag = 0;
	if(BL6523GX_CheckSelf(1))
	{
		check_flag |= 0x01;
		myprintf("\r\n电量计一 ERROR！！！");
	}
	else
		myprintf("\r\n电量计一 SUCCESS！！！");
	if(BL6523GX_CheckSelf(2))
	{
		check_flag |= 0x02;
		myprintf("\r\n电量计二 ERROR！！！");
	}		
	else
		myprintf("\r\n电量计二 SUCCESS！！！");
	return check_flag;
}
*/

static uint16_t LedShowTime = 0;;
static uint32_t time_prinf_cnt = 0;
void Led_Task(void * argument)
{
	BaseType_t xReturn = pdTRUE;/* 定义一个创建信息返回值，默认为 pdPASS */
	
	uint8_t check_flag = 0;
	uint8_t netsta = 0;
	uint8_t led_offline;
//	check_flag = CheckSelf();	//自检程序
	/*
	do{
		CheckSelf_Flag_Rw(0, &check_flag);
		HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
		vTaskDelay(100);
	}while(check_flag == 0xff);
	*/
	if(IDStatus == 0)			//未预制ID
	{
//		uint8_t ledstatus = 1,ledpwm = 100;
//		Write_LED_Data(1,&ledstatus,&ledpwm);
//		Write_LED_Data(2,&ledstatus,&ledpwm);
//		BL6523GX_Triming(1);
//		BL6523GX_Triming(2);
		while(1)
		{
//			if(check_flag)
//				HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
//			else
//			{
//				HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin,GPIO_PIN_RESET);
//			}
			vTaskDelay(100);
		}
	}
	{
//		uint8_t ledstatus = 1,ledpwm = 100;
//		Write_LED_Data(1,&ledstatus,&ledpwm);
//		Write_LED_Data(2,&ledstatus,&ledpwm);
//		BL6523GX_Triming(1);
//		BL6523GX_Triming(2);
	}
    while(1)
    {
		//HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
		{
		int16_t temp;
		temp = Get_Temprate();
		myprintf("\r\nCPU cuur TEMP:%.2f℃",(float)temp/100);
		}
		time_prinf_cnt++;
//		if(time_prinf_cnt%(3600*10*24) == 0)
//		{
///*			BL6523GX_Init(2);
//			BL6523GX_Init(1); */
//			check_flag = CheckSelf();
//		}
		TcpConnectFlag_RW(0,&netsta);
		CheckSelf_Flag_Rw(0, &check_flag);
		if(check_flag)		//校验失败，运行灯长亮
		{
			HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin,GPIO_PIN_RESET);
		}
		else
		{
			if(netsta != ConnectOnLine)
			{
				if(led_offline == 0)
					HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin,GPIO_PIN_RESET);
				else
					HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin,GPIO_PIN_SET);
				led_offline++;
				if(led_offline >= 4)
					led_offline = 0;
			}
			else
				HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);	//运行灯
		}
		if((time_prinf_cnt) %5 ==0)//每5秒打印一次时间
		{
			rtc_time_t rtc_time;
			
			GetRTC(&rtc_time);
			myprintf("\r\n%02d-%02d-%02d  %02d:%02d:%02d ", rtc_time.ui8Year, rtc_time.ui8Month,\
			rtc_time.ui8DayOfMonth, rtc_time.ui8Hour, rtc_time.ui8Minute, rtc_time.ui8Second);
		}
		xReturn = xSemaphoreTake (LedShowBinary, 0);
		if(xReturn == pdTRUE)
		{
			ledshowFlag = 1;
			LedShowTime = 0;
			myprintf("led演示模式\r\n");
		}
		if(ledshowFlag == 1)
		{
			LedShowTime++;
			if(LedShowTime>=LED_SHOW_CONTINUE_TIME)		//演示模式持续时间
			{
				ledshowFlag = 0;	//临时调试注释
				LedShowTime = 0;
			}
		}
		else
		{
				for(uint8_t i=0; i<LED_NUM; i++)
				{
					Led_Strategy_new(i+1);		//leds时间策略
				}
		}
//		EnvironmentData_Task();	//获取环境监测信息
//		BL6526B_ProcessTask(&electricdata);				/*电能计量芯片读取函数*/
		vTaskDelay(1000);
    }
}

void CreatLedTask(void)
{
	BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */

	myprintf("\r\n CreatLedTask");

    xReturn = xTaskCreate(Led_Task, "Led", Led_StkSIZE, NULL, Led_TaskPrio, &pvCreatedTask_Led);
    if(pdPASS != xReturn)
    {
        myprintf("\r\n Led_Task creat error");
        while(1);
    }
}
