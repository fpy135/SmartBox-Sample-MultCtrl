#ifndef __ENVIRONMENT_DATA_H
#define __ENVIRONMENT_DATA_H

#include "main.h"

#define Environment_Dir(x) 	HAL_GPIO_WritePin(UART6_DIR_GPIO_Port, UART6_DIR_Pin,x);
#define Environment_Send	UART6Write
#define MANHOLECOVER_ANGLE_NUM		(1)

typedef enum 
{
	windspeed 	= 1,	//上海搜博风向
	winddir		= 2,	//上海搜博风速
	temp_hum	= 3,	//上海搜博温湿度粉尘
	
	dip_angle	= 10,	//倾角
	air_switch	= 11,	//智能空开
}SENSOR_485ADDR_INFO;

typedef union 
{
	uint32_t byte_use;
	struct{
		uint32_t	SM5386:1;	//上海搜博风速
		uint32_t	SM5387B:1;	//上海搜博风向
		uint32_t	SM6333B:1;	//上海搜博温湿度粉尘
		uint32_t	reserve0:1;
		uint32_t	reserve1:1;
		uint32_t	reserve2:1;
		uint32_t	reserve4:1;
		uint32_t	reserve5:1;
		uint32_t	reserve6:1;
		uint32_t	MTC70M:1;	//上海默律倾角传感器
		uint32_t	IOTB2_16:1;	//透明物联智能空开
		uint32_t	WS301:1;	//耘农温湿度传感器
		uint32_t	HCD68XX:1;	//耘农多合一传感器
		uint32_t	ManholeCover_MTC70M:1;	//上海默律倾角传感器模拟井盖
	}bit_use;
}SENSOR_485_USE_TYPE;

#pragma pack(push,1)
typedef struct{
	uint16_t noise;			//噪音
	uint16_t SO2;			//SO2
	uint16_t NO2;			//NO2
	uint16_t CO;			//CO
	uint16_t O3;			//O3
    uint16_t pm2_5;			//PM2.5
    uint16_t pm10;			//PM10
    uint16_t temperature;	//温度
    uint16_t humidity;		//湿度
	uint16_t air_pressure;	//大气压
	uint16_t wind_speed;	//风速
    uint16_t wind_direction;//风向
	uint16_t rain;			//降雨量
	uint16_t radiation;		//辐射
	uint16_t sun_power;		//光照强度
	uint16_t ultraviolet;	//紫外线强度
	uint16_t CO2;			//CO2
	uint16_t NAI;			//负氧离子浓度 ppb
}Environment_Type;

typedef struct{
	int16_t angle_x;		//放大10倍
	int16_t angle_y;		//放大10倍
	int16_t angle_z;		//放大10倍
}ANGLE_MTC70M_Type;

typedef struct{
	uint16_t smd_temp_L;	//火线接线端子温度 0.1℃
	uint16_t smd_temp_N;	//零线接线端子温度 0.1℃
	uint16_t chip_temp;		//芯片温度	0.1℃
	uint16_t voltage;		//电压		0.1V
	uint16_t current;		//电流		0.01A
	int32_t active_power;	//有功功率	0.1W
	int32_t reactive_power;	//无功功率	0.1W
	uint16_t freq;			//频率		0.1Hz
	uint32_t active_ele;	//有功电量	0.01kwh
	uint32_t reactive_ele;	//无功电量	0.01kvarh
	uint16_t power_factor;	//功率因数
	uint16_t switch_sta;	//合闸状态	0x1A关 0x15开
	uint16_t section1_1;	//1区复制01
	uint16_t section1_2;	//1区复制02
	uint32_t apparent_power;//视在功率	0.1VA
	uint16_t over_current;	//过流时电流值
	uint16_t harmonic_ratio;//电流谐波比率当前值
	uint16_t leak_curr_val;	//漏电发生时漏电值
	uint16_t leak_current;	//漏电流
	uint16_t toggle_switch;	//拨动开关状态
	uint16_t leak_curr_freq;//漏电基波
	uint16_t current_2;		//电流2次 预留
	uint16_t current_3;		//电流3次 预留
	uint16_t current_4;		//电流4次 预留
	uint16_t current_5;		//电流5次 预留
	uint16_t reserved;		//未知预留
	uint16_t leak_curr_val_org;	//漏电原始值
	uint16_t active_reason;	//分合闸日志 0空 1预留 2:485 3定时 4按钮 5漏电 6保护 7其他 8挂锁分闸
	uint32_t utc_time;		//utc时间
}AIRSWITCH_TM_Type;
#pragma pack(pop)

typedef union{
	AIRSWITCH_TM_Type AirSwitch_TM_Data;
	uint8_t data_org[70];
}AIRSWITCH_UNION_TM_Type;

#define TM_AIRSWITCH_OPEN	0x1A	//分闸
#define TM_AIRSWITCH_CLOSE	0x15	//合闸

extern AIRSWITCH_UNION_TM_Type AirSwitch_TM_Data;
extern ANGLE_MTC70M_Type Angle_MTC70M;
extern Environment_Type Environment_Data;

extern uint8_t Tx485Buffer1[8];
extern uint8_t Tx485Buffer2[8];
extern uint8_t Tx485Buffer3[8];

//extern uint32_t Sensor_485Addr_Use;		//485传感器挂载设备
extern SENSOR_485_USE_TYPE SENSOR_485_USE;			//485传感器挂载设备

extern void Sensor_485_Use_Info_Print(SENSOR_485_USE_TYPE sensor_use_data);
extern void SENSOR_485_USE_Data_RW(uint8_t read_or_write,SENSOR_485_USE_TYPE *sensor_use_data);

extern uint32_t SaveEnergy_By_Flash_Init(void);
extern void SaveEnergy_By_Flash(uint8_t *energy, uint8_t datalen);
extern uint8_t Write_UserFlashAndSaveEnergy(uint32_t _ulFlashAddr, uint8_t *_ucpSrc, uint32_t _ulSize);

extern void Angle_Data_RW(uint8_t read_or_write,ANGLE_MTC70M_Type *angle_data);
extern void ManholeCover_angle_Data_RW(uint8_t read_or_write, uint8_t channel, ANGLE_MTC70M_Type *angle_data);

extern void AirSwitch_TM_Data_RW(uint8_t read_or_write, uint8_t *air_switch_data);

extern void Environment_device_Type(uint8_t read_or_write,uint8_t *temp_type);
extern void Environment_Data_RW(uint8_t read_or_write,Environment_Type *environment_data);
extern void CheckSelf_Flag_Rw(uint8_t rw, uint8_t * checkflag);

extern void Get_MULTIPLE_Environment_Data(void);
extern void Get_TEN_IN_ONE_Environment_Data(void);

extern void Get_Environment_Data(void);
extern void CreatEnvironment_DataTask(void);

extern void EnvironmentData_Task(void);

#endif
