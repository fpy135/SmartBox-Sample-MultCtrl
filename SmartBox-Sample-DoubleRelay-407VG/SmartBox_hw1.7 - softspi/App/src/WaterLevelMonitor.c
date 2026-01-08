#include "WaterLevelMonitor.h"
#include "bsp_key.h"
#include "usart.h"

extern void NetParaRst(void);

SWITCH_WATER Switch_Water;

/*********************************
水位监测时间处理
定时器中断调用 1ms
*********************************/
void WaterTimeProcess(void)
{
	if(Switch_Water.water_recover_flag)
	{
		Switch_Water.water_recover_time++;
	}
	else if(Switch_Water.water_over_flag)
	{
		Switch_Water.water_over_time++;
	}
	if(Switch_Water.platform_alarm_flag == 1 && \
		(Switch_Water.water_over_time % WATERALARM_HOLDTIME) == 0)
	{
		Switch_Water.platform_alarmsend_flag = 1;
	}
	else if(Switch_Water.water_recover_time >= WATERLEVEL_HOLDTIME)
	{
		if(Switch_Water.platform_alarm_flag == 2 && \
			(Switch_Water.water_recover_time % WATERALARM_HOLDTIME) == 0)
		{
			Switch_Water.platform_alarmsend_flag = 2;
		}
	}
}


/*********************************
水位监测时间处理
返回值 0：无触发	1：高于警戒水位		2：低于警戒水位
*********************************/
uint8_t WaterServiceLogic(void)
{
	uint8_t ret = 0;
	uint8_t ucKeyCode;		/* 按键代码 */

//	Switch_Water.platform_alarmsend_flag = 0;
	ucKeyCode = bsp_GetKey();	/* 读取键值, 无键按下时返回 KEY_NONE = 0 */
	if (ucKeyCode != KEY_NONE)
	{
		switch (ucKeyCode)
		{
			case KEY_DOWN_K1:			/* K1键按下 */
				myprintf("\r\nK1键按下");
				break;
			case KEY_LONG_K1:			/* K1键长按 */
				myprintf("\r\nK1键长按");
				NetParaRst();		//网络参数复位
				break;
			case KEY_DOWN_K2:			/* K1键按下 */
				myprintf("\r\nK2键按下");
				if(Switch_Water.water_over_flag == 0)
				{
					Switch_Water.water_recover_flag = 0;
				}
				Switch_Water.water_recover_time = 0;
				break;
			case KEY_2_UP:
				myprintf("\r\nK2键释放");
				if(Switch_Water.water_over_flag == 1)
				{
					Switch_Water.water_recover_flag = 1;
				}
				Switch_Water.water_recover_time = 0;
				break;
			case KEY_LONG_K2:			/* K1键长按 */
				myprintf("\r\nK2键长按");
				Switch_Water.water_over_flag = 1;
				Switch_Water.water_over_time = 0;
				if(Switch_Water.water_recover_flag == 0)
				{
					Switch_Water.platform_alarm_flag = 1;	//水位报警
					Switch_Water.platform_alarmsend_flag = 1;
//					ret = 1;
				}
				else
				{
					Switch_Water.water_recover_flag = 0;
					Switch_Water.water_recover_time = 0;
				}
				break;
		}
	}
	if(Switch_Water.water_recover_flag == 1 && \
		Switch_Water.water_recover_time >= WATERLEVEL_HOLDTIME)
	{
//		Switch_Water.water_recover_flag = 0;
//		Switch_Water.water_recover_time = 0;
		if(Switch_Water.water_over_flag == 1)
		{
			Switch_Water.water_over_flag = 0;
			Switch_Water.water_over_time = 0;
			Switch_Water.platform_alarm_flag = 2;			//解除水位报警
			Switch_Water.platform_alarmsend_flag = 2;
//			ret = 2;
		}
	}
	ret = Switch_Water.platform_alarmsend_flag;
	Switch_Water.platform_alarmsend_flag = 0;
	return ret;
}

