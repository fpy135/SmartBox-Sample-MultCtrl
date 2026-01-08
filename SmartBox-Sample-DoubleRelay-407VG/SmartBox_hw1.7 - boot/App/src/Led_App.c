#include "Led_App.h"
#include "SysConfig.h"
#include "Usart.h"
#include "rtc.h"
#include "Unixtimer.h"
#include "bl6526b.h"
#include "Environment_data.h"
#include "iwdg.h"


extern void GetRTC(rtc_time_t *rtc_time);
extern void SetRTC(rtc_time_t *rtc_time);

SemaphoreHandle_t LedShowBinary;	//演示模式信号量
SemaphoreHandle_t TimeControlMutex; //TimeControlMutex

TimeControl_Type TimeControl_Data;	//时控模式结构体

xTaskHandle pvCreatedTask_Led;

uint8_t LedPwm = 0;
uint8_t targetBrightness = 0;
uint8_t LedStatus = 0;


void Get_TimeControl_Data(TimeControl_Type *timecontrol_data)
{
	xSemaphoreTake (TimeControlMutex, portMAX_DELAY);
	
	timecontrol_data->start_hour = TimeControl_Data.start_hour;
	timecontrol_data->start_min = TimeControl_Data.start_min;
	timecontrol_data->phase1_Pwm = TimeControl_Data.phase1_Pwm;
	timecontrol_data->phase1_time = TimeControl_Data.phase1_time;
	timecontrol_data->phase2_Pwm = TimeControl_Data.phase2_Pwm;
	timecontrol_data->phase2_time = TimeControl_Data.phase2_time;
	timecontrol_data->phase3_Pwm = TimeControl_Data.phase3_Pwm;
	timecontrol_data->phase3_time = TimeControl_Data.phase3_time;
	timecontrol_data->phase4_Pwm = TimeControl_Data.phase4_Pwm;
	timecontrol_data->phase4_time = TimeControl_Data.phase4_time;
	xSemaphoreGive (TimeControlMutex);
}

void Write_TimeControl_Data(TimeControl_Type *timecontrol_data)
{
	xSemaphoreTake (TimeControlMutex, portMAX_DELAY);
	
	TimeControl_Data.start_hour = timecontrol_data->start_hour;
	TimeControl_Data.start_min = timecontrol_data->start_min;
	TimeControl_Data.phase1_Pwm = timecontrol_data->phase1_Pwm;
	TimeControl_Data.phase1_time = timecontrol_data->phase1_time;
	TimeControl_Data.phase2_Pwm = timecontrol_data->phase2_Pwm;
	TimeControl_Data.phase2_time = timecontrol_data->phase2_time;
	TimeControl_Data.phase3_Pwm = timecontrol_data->phase3_Pwm;
	TimeControl_Data.phase3_time = timecontrol_data->phase3_time;
	TimeControl_Data.phase4_Pwm = timecontrol_data->phase4_Pwm;
	TimeControl_Data.phase4_time = timecontrol_data->phase4_time;
	xSemaphoreGive (TimeControlMutex);
}


void Get_LED_Data(uint8_t *ledstatus,uint8_t *ledpwm)
{
	taskENTER_CRITICAL();
	*ledstatus = LedStatus;
	*ledpwm = LedPwm;
	taskEXIT_CRITICAL();
}

void Write_LED_Data(uint8_t *ledstatus,uint8_t *ledpwm)
{
	taskENTER_CRITICAL();
	LedStatus = *ledstatus;
	LedPwm = *ledpwm;
	if(LedStatus)
		HAL_GPIO_WritePin(GPIOC, REL_EN2_Pin|REL_EN1_Pin, GPIO_PIN_RESET);	//打开继电器电源
	else
		HAL_GPIO_WritePin(GPIOC, REL_EN2_Pin|REL_EN1_Pin, GPIO_PIN_SET);	//关闭继电器电源
	taskEXIT_CRITICAL();
}

/******************************
功能：产生 pwm 波
用法：定时器中断内调用
注意：该版本仅适合单灯控制
******************************/
void Led_Pwm_Control(void)
{
    static uint8_t led_cnt = 1;
    if(led_cnt>LedPwm)
    {
//        Led0_Write(1);
		Led1_Write(1);
		Led2_Write(1);
    }
    else
    {
//        Led0_Write(0);
		Led1_Write(0);
		Led2_Write(0);
    }
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

void Led_Strategy(void)
{
	uint8_t ledstatus;
	uint8_t ledpwm;
	rtc_time_t rtc_time;
	rtc_time_t temp_time;
	static TimeControl_Type timecontrol_data;
	uint32_t rtc_unix_time = 0;
	static uint32_t timecontro_unix_time = 0;
	static uint8_t get_timecontrol_unix_flag = 0;
	
	GetRTC(&rtc_time);
	if(rtc_time.ui8Year < 2020)		//未获取到时间，不执行时间管理
		return;
	rtc_unix_time = covBeijing2UnixTimeStp(&rtc_time);
	if(get_timecontrol_unix_flag == 0)		//获取的一个时控的uinx时间戳
	{
		Get_TimeControl_Data(&timecontrol_data);
		temp_time.ui8Year = rtc_time.ui8Year;
		temp_time.ui8Month = rtc_time.ui8Month;
		temp_time.ui8DayOfMonth = rtc_time.ui8DayOfMonth;
		temp_time.ui8Hour = timecontrol_data.start_hour;
		temp_time.ui8Minute = timecontrol_data.start_min;
		temp_time.ui8Second = 0;
		timecontro_unix_time = covBeijing2UnixTimeStp(&temp_time);
		
	}
	//如果时间超过了策略的初始时间
	if(rtc_unix_time >= timecontro_unix_time)
	{
		get_timecontrol_unix_flag = 1;
		if((rtc_unix_time-timecontro_unix_time) <= timecontrol_data.phase1_time*60)	//阶段一
		{
			HAL_GPIO_WritePin(GPIOC, REL_EN2_Pin|REL_EN1_Pin, GPIO_PIN_RESET);	//打开继电器电源
			ledstatus = LEDON;
			ledpwm = timecontrol_data.phase1_Pwm;
			Write_LED_Data(&ledstatus,&ledpwm);
			myprintf("\r\n时控模式阶段一");
		}
		else if((rtc_unix_time-timecontro_unix_time) <= \
			(timecontrol_data.phase1_time+timecontrol_data.phase2_time)*60)	//阶段二
		{
			ledstatus = LEDON;
			ledpwm = timecontrol_data.phase2_Pwm;
			Write_LED_Data(&ledstatus,&ledpwm);
			myprintf("\r\n时控模式阶段二");
		}
		else if((rtc_unix_time-timecontro_unix_time) <= \
			(timecontrol_data.phase1_time+timecontrol_data.phase2_time+\
			timecontrol_data.phase3_time)*60)	//阶段三
		{
			ledstatus = LEDON;
			ledpwm = timecontrol_data.phase3_Pwm;
			Write_LED_Data(&ledstatus,&ledpwm);
			myprintf("\r\n时控模式阶段三");
		}
		else if((rtc_unix_time-timecontro_unix_time) <= \
			(timecontrol_data.phase1_time+timecontrol_data.phase2_time+\
			timecontrol_data.phase4_time)*60)	//阶段四
		{
			ledstatus = LEDON;
			ledpwm = timecontrol_data.phase4_Pwm;
			Write_LED_Data(&ledstatus,&ledpwm);
			myprintf("\r\n时控模式阶段四");
		}
		else	//策略模式亮灯结束
		{
			HAL_GPIO_WritePin(GPIOC, REL_EN2_Pin|REL_EN1_Pin, GPIO_PIN_SET);	//关闭继电器电源
			ledstatus = LEDOFF;
			ledpwm = 0;
			Write_LED_Data(&ledstatus,&ledpwm);
			get_timecontrol_unix_flag = 0;		//重新获取新的时间戳
			myprintf("\r\n时控模式结束");
		}
	}
/*	if(rtc_time.ui8Month >= LED_SUMMER_START && rtc_time.ui8Month <=LED_SUMMER_END)		//led夏令时
	{
		if(rtc_time.ui8Hour >= 19 || rtc_time.ui8Hour<= 1)	//上半夜全功率
		{
			ledstatus = LEDON;
			ledpwm = 99;
			Write_LED_Data(&ledstatus,&ledpwm);
		}
		else if(rtc_time.ui8Hour > 1 && rtc_time.ui8Hour<= 5)	//下半夜50%功率
		{
			ledstatus = LEDON;
			ledpwm = 50;
			Write_LED_Data(&ledstatus,&ledpwm);
		}
		else
		{
			ledstatus = LEDOFF;
			ledpwm = 0;
			Write_LED_Data(&ledstatus,&ledpwm);
		}
	}
	else			//led冬令时
	{
		if(rtc_time.ui8Hour >= 18 || rtc_time.ui8Hour<= 1)	//上半夜全功率
		{
			ledstatus = LEDON;
			ledpwm = 99;
			Write_LED_Data(&ledstatus,&ledpwm);
		}
		else if(rtc_time.ui8Hour > 1 && rtc_time.ui8Hour<= 6)	//下半夜50%功率
		{
			ledstatus = LEDON;
			ledpwm = 50;
			Write_LED_Data(&ledstatus,&ledpwm);
		}
		else
		{
			ledstatus = LEDOFF;
			ledpwm = 0;
			Write_LED_Data(&ledstatus,&ledpwm);
		}
	}
	*/
}

void Led_Task(void * argument)
{
	BaseType_t xReturn = pdTRUE;/* 定义一个创建信息返回值，默认为 pdPASS */
	uint16_t LedShowTime = 0;;
	uint8_t ledshowFlag = 0;
	static uint8_t time_prinf_cnt = 0;
	rtc_time_t rtc_time;
	ElectricData_Type electricdata;
	
	uint32_t type_test_tick = 0;
	uint32_t environment_sample_tick = 0;
	
    while(1)
    {
//		HAL_IWDG_Refresh(&hiwdg);
		GetRTC(&rtc_time);
		if((time_prinf_cnt) %5 ==0)//每5秒打印一次时间
		{
			time_prinf_cnt++;
			myprintf("%02d-%02d-%02d  %02d:%02d:%02d\r\n", rtc_time.ui8Year, rtc_time.ui8Month,\
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
//			if(LedShowTime>=10)		//演示模式持续时间
			if(LedShowTime>=LED_SHOW_CONTINUE_TIME)		//演示模式持续时间
			{
				ledshowFlag = 0;
				LedShowTime = 0;
			}
		}
		else
			Led_Strategy();		//leds时间策略
//		BL6526B_ProcessTask(&electricdata);				/*电能计量芯片读取函数*/
		HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
		EnvironmentData_Task();
		vTaskDelay(1000);
    }
}

void CreatLedTask(void)
{
	BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */

	_myprintf("\r\n CreatLedTask");

    xReturn = xTaskCreate(Led_Task, "Led", Led_StkSIZE, NULL, Led_TaskPrio, &pvCreatedTask_Led);
    if(pdPASS != xReturn)
    {
        _myprintf("\r\n Led_Task creat error");
        while(1);
    }
}
