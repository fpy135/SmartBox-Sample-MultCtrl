#ifndef __WATERLEVELMONITOR_H_
#define __WATERLEVELMONITOR_H_

#include "main.h"

#define WATERLEVEL_HOLDTIME		(5000)	//单位ms
#define WATERALARM_HOLDTIME		(5000)	//单位ms

typedef struct 
{
	uint8_t water_over_flag;
	uint32_t water_over_time;
	uint8_t water_recover_flag;
	uint32_t water_recover_time;
	uint8_t platform_alarm_flag;
	uint8_t platform_alarmsend_flag;
}SWITCH_WATER;

extern SWITCH_WATER Switch_Water;

extern uint8_t WaterServiceLogic(void);
extern void WaterTimeProcess(void);


#endif
