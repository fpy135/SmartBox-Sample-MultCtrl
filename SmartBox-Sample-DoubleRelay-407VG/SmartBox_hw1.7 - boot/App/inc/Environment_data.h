#ifndef __ENVIRONMENT_DATA_H
#define __ENVIRONMENT_DATA_H

#include "main.h"

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
}Environment_Type;
#pragma pack(pop)

extern Environment_Type Environment_Data;

extern uint8_t Tx485Buffer1[8];
extern uint8_t Tx485Buffer2[8];
extern uint8_t Tx485Buffer3[8];

extern void Environment_device_Type(uint8_t read_or_write,uint8_t *temp_type);
extern void Environment_Data_RW(uint8_t read_or_write,Environment_Type *environment_data);

extern void Get_MULTIPLE_Environment_Data(void);
extern void Get_TEN_IN_ONE_Environment_Data(void);

extern void Get_Environment_Data(void);
extern void CreatEnvironment_DataTask(void);

extern void EnvironmentData_Task(void);

#endif
