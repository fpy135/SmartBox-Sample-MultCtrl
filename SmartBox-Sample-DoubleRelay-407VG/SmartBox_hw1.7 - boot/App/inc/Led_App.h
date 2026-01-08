#ifndef __LED_APP_H
#define __LED_APP_H

#include "main.h"

#define Led0_Write(x)				HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, x)
#define Led1_Write(x)				HAL_GPIO_WritePin(PWM1_GPIO_Port, PWM1_Pin, x)
#define Led2_Write(x)				HAL_GPIO_WritePin(PWM2_GPIO_Port, PWM2_Pin, x)

#define Led_Switch_Brightness(x) 	LedPwm = x

#define LED_SUMMER_START			4	//led策略夏令时4月开始
#define LED_SUMMER_END				9	///led策略夏令时9月结束

#define LED_SHOW_CONTINUE_TIME		(60*10)	//10分钟

typedef enum{
	LEDOFF	= 0x00,
	LEDON	= 0x01,
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
#pragma pack(pop)

extern TimeControl_Type TimeControl_Data;

extern uint8_t LedPwm;
extern uint8_t targetBrightness;
extern uint8_t LedStatus;

extern void Get_TimeControl_Data(TimeControl_Type *timecontrol_data);
extern void Write_TimeControl_Data(TimeControl_Type *timecontrol_data);

extern void Write_LED_Data(uint8_t *ledstatus,uint8_t *ledpwm);
extern void Get_LED_Data(uint8_t *ledstatus,uint8_t *ledpwm);

extern void Led_Pwm_Control(void);
extern void Switch_Brightness(void);

extern void CreatLedTask(void);

#endif
