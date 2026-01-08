#ifndef __LED_APP_H
#define __LED_APP_H

#include "main.h"

/***************************************
控制灯的个数
***************************************/
#define LED_NUM					(2)
#define CUSTOM_DEV_NUM			(0)			//用户自定义设备

#define HARDWARE_PWM		0
#define LedRun_Write(x)				HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, x)
#define Led1_Write(x)				HAL_GPIO_WritePin(PWM1_GPIO_Port, PWM1_Pin, !x)
#define Led2_Write(x)				HAL_GPIO_WritePin(PWM2_GPIO_Port, PWM2_Pin, !x)

#define REL_OFF		GPIO_PIN_SET
#define REL_ON		GPIO_PIN_RESET

#define REL1_Write(x)				HAL_GPIO_WritePin(REL_EN1_GPIO_Port, REL_EN1_Pin, x)
#define REL2_Write(x)				HAL_GPIO_WritePin(REL_EN2_GPIO_Port, REL_EN2_Pin, x)
#define REL_Write(x,y)				((x==1) ? REL1_Write(y):REL2_Write(y))

#define Led_Switch_Brightness(x) 	LedPwm = x

#define LED_SUMMER_START			4	//led策略夏令时4月开始
#define LED_SUMMER_END				9	///led策略夏令时9月结束

#define LED_SHOW_CONTINUE_TIME		(60*10)	//10分钟
//#define LED_SHOW_CONTINUE_TIME		(5)

typedef enum{
	LEDOFF	= 0,
	LEDON	= 1,
}LedStatus_Type;

#pragma pack(push,1)
typedef struct
{
    uint8_t  start_hour;
    uint8_t  start_min;
    uint16_t phase1_time;
    uint8_t  phase1_Pwm;
	uint16_t phase2_time;
    uint8_t  phase2_Pwm;
    uint16_t phase3_time;
    uint8_t  phase3_Pwm;
	uint16_t phase4_time;
    uint8_t  phase4_Pwm;
}TimeControl_Type;

typedef struct
{
    uint8_t  start_hour;
    uint8_t  start_min;
    uint16_t phase1_time;
    uint8_t  led_sta1;
	uint16_t phase2_time;
    uint8_t  led_sta2;
    uint16_t phase3_time;
    uint8_t  led_sta3;
	uint16_t phase4_time;
    uint8_t  led_sta4;
}TimeCustomControl_Type;
#pragma pack(pop)

extern TimeControl_Type TimeControl_Data[LED_NUM];
extern TimeCustomControl_Type TimeCustomCtrl_Data;
extern TimeCustomControl_Type TimeCustomCtrl_Data2;

extern uint8_t targetBrightness;

extern uint8_t CustomStatus1;
extern uint8_t CustomStatus2;

extern uint8_t ledshowFlag;
extern uint8_t LedPwm[LED_NUM];
extern uint8_t LedStatus[LED_NUM];

extern uint8_t Get_TimeControl_Data(uint8_t channle, TimeControl_Type *timecontrol_data);
extern uint8_t Write_TimeControl_Data(uint8_t channle, TimeControl_Type *timecontrol_data);
extern uint8_t Get_TimeCustomCtrl_Data(uint8_t channle, TimeCustomControl_Type *timecontrol_data);
extern void Write_TimeCustomCtrl_Data(uint8_t channle, TimeCustomControl_Type *timecontrol_data);

extern void Write_LED_Data(uint8_t led_num, uint8_t *ledstatus, uint8_t *ledpwm);
extern void Get_LED_Data(uint8_t led_num, uint8_t *ledstatus, uint8_t *ledpwm);

extern void Led_Pwm_Control(void);
extern void Switch_Brightness(void);

extern void CreatLedTask(void);

#endif
