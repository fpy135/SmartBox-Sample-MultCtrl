#include "Environment_data.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "SysConfig.h"
#include "rtc.h"
#include "Unixtimer.h"

#include "BL0942.h"
#include "IDConfig.h"
#include "bsp_key.h"
#include "Led_App.h"
#include "StorageConfig.h"
#include "MyFlash.h"
#include "w25qxx.h"
QueueHandle_t TM_AirSwitch_Queue;	//智能空开队列
SemaphoreHandle_t AirSwitchBinary;	//智能空开信号量
SemaphoreHandle_t AngleBinary;		//倾角信号量
SemaphoreHandle_t ManholeCoverBinary;		//井盖信号量
uint8_t CheckSelfFlag = 0x00;
extern void Write_LED_Data(uint8_t led_num, uint8_t *ledstatus, uint8_t *ledpwm);
extern void Get_LED_Data(uint8_t led_num, uint8_t *ledstatus, uint8_t *ledpwm);
extern void Get_ElectricData(uint8_t channle, CollectionData *electricdata);
extern void Write_ElectricData(uint8_t channle, CollectionData *electricdata);

uint8_t fenxiangAddr[8] = {0x01,0x06,0x00, 0x66,0x00,0x02,0xe8,0x14};
uint8_t wenshidugAddr[8] = {0x01,0x06,0x00, 0x66,0x00,0x03,0x29,0xD4};

Environment_Type Environment_Data;
ANGLE_MTC70M_Type Angle_MTC70M;
ANGLE_MTC70M_Type ManholeCover_angle[MANHOLECOVER_ANGLE_NUM];
//AIRSWITCH_TM_Type AirSwitch_TM_Data;
AIRSWITCH_UNION_TM_Type AirSwitch_TM_Data;
SENSOR_485_USE_TYPE SENSOR_485_USE;			//485传感器挂载设备
//uint32_t Sensor_485Addr_Use =0X0000001F;		//485传感器挂载设备
uint8_t Sensor_485Addr_GetCnt;		//获取第几个传感器计数器

BaseType_t Environment_Data_TaskID;
xTaskHandle pvCreatedTask_Environment_Data;

SemaphoreHandle_t EnvironmentMutex; //EnvironmentMutex

static uint8_t Environment_device_type = 0xff;

void Environment_device_Type(uint8_t read_or_write,uint8_t *temp_type)
{
	xSemaphoreTake (EnvironmentMutex, portMAX_DELAY);
	
	if(read_or_write == 0)
	{
		*temp_type = Environment_device_type;
	}
	else if(read_or_write == 1)
	{
		Environment_device_type = *temp_type;
	}
	xSemaphoreGive (EnvironmentMutex);
}

void Environment_Data_RW(uint8_t read_or_write,Environment_Type *environment_data)
{
	xSemaphoreTake (EnvironmentMutex, portMAX_DELAY);
	
	if(read_or_write == 0)
	{
		memcpy(environment_data,&Environment_Data,sizeof(Environment_Data));
	}
	else if(read_or_write == 1)
	{
		memcpy(&Environment_Data,environment_data,sizeof(Environment_Data));
	}
	xSemaphoreGive (EnvironmentMutex);
}

uint32_t FreeRTOSRunTimeTicks;
/*
void TaskSta_task(void *pvParameters)
{
    UBaseType_t   ArraySize;
    TaskStatus_t  *StatusArray;
    uint8_t       x;
	unsigned portBASE_TYPE uxHighWaterMark;

    ArraySize = uxTaskGetNumberOfTasks(); //获取任务个数
    StatusArray = pvPortMalloc(ArraySize*sizeof(TaskStatus_t));
    while(1)
    {
        if(StatusArray != NULL){ //内存申请成功

            ArraySize = uxTaskGetSystemState( (TaskStatus_t *)  StatusArray,
                                              (UBaseType_t   ) ArraySize,
                                              (uint32_t *    )  &FreeRTOSRunTimeTicks );

            myprintf("TaskName\t\t\tPriority\t\tTaskNumber\t\tMinStk\t\t\n");
            for(x = 0;x<ArraySize;x++)
			{
                myprintf("%s\t\t\t%d\t\t\t%d\t\t\t%d\t\t%d\r\n",
                        StatusArray[x].pcTaskName,
                        (int)StatusArray[x].uxCurrentPriority,
                        (int)StatusArray[x].xTaskNumber,
                        (int)StatusArray[x].usStackHighWaterMark,
                        (int)((float)StatusArray[x].ulRunTimeCounter/FreeRTOSRunTimeTicks*100));
            }
            myprintf("\n\n");
        }
		vTaskDelay(1000);
    }
}*/

void Set_Sensor_Addr(void)
{
	Environment_Dir(GPIO_PIN_SET);
    Environment_Send(fenxiangAddr,8);
    Environment_Dir(GPIO_PIN_RESET);
	vTaskDelay(500);
//	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8,GPIO_PIN_SET);
//    Environment_Send(&huart1,wenshidugAddr,8,10000);
//    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8,GPIO_PIN_RESET);
//	vTaskDelay(500);
}

void SENSOR_485_USE_Data_RW(uint8_t read_or_write,SENSOR_485_USE_TYPE *sensor_use_data)
{
	xSemaphoreTake (EnvironmentMutex, portMAX_DELAY);
	
	if(read_or_write == 0)
	{
		sensor_use_data->byte_use = SENSOR_485_USE.byte_use;
	}
	else if(read_or_write == 1)
	{
		SENSOR_485_USE.byte_use = sensor_use_data->byte_use;
	}
	xSemaphoreGive (EnvironmentMutex);
}

void Angle_Data_RW(uint8_t read_or_write,ANGLE_MTC70M_Type *angle_data)
{
	xSemaphoreTake (EnvironmentMutex, portMAX_DELAY);
	
	if(read_or_write == 0)
	{
		angle_data->angle_x = Angle_MTC70M.angle_x;
		angle_data->angle_y = Angle_MTC70M.angle_y;
		angle_data->angle_z = Angle_MTC70M.angle_z;
	}
	else if(read_or_write == 1)
	{
		Angle_MTC70M.angle_x = angle_data->angle_x;
		Angle_MTC70M.angle_y = angle_data->angle_y;
		Angle_MTC70M.angle_z = angle_data->angle_z;
	}
	xSemaphoreGive (EnvironmentMutex);
}

void ManholeCover_angle_Data_RW(uint8_t read_or_write, uint8_t channel, ANGLE_MTC70M_Type *angle_data)
{
	xSemaphoreTake (EnvironmentMutex, portMAX_DELAY);
	
	if(read_or_write == 0)
	{
		angle_data->angle_x = ManholeCover_angle[channel].angle_x;
		angle_data->angle_y = ManholeCover_angle[channel].angle_y;
		angle_data->angle_z = ManholeCover_angle[channel].angle_z;
	}
	else if(read_or_write == 1)
	{
		ManholeCover_angle[channel].angle_x = angle_data->angle_x;
		ManholeCover_angle[channel].angle_y = angle_data->angle_y;
		ManholeCover_angle[channel].angle_z = angle_data->angle_z;
	}
	xSemaphoreGive (EnvironmentMutex);
}

void AirSwitch_TM_Data_RW(uint8_t read_or_write, uint8_t *air_switch_data)
{
	xSemaphoreTake (EnvironmentMutex, portMAX_DELAY);
	
	if(read_or_write == 0)
	{
//		air_switch_data->voltage = AirSwitch_TM_Data.voltage;
//		air_switch_data->current = AirSwitch_TM_Data.current;
//		air_switch_data->active_power = AirSwitch_TM_Data.active_power;
//		air_switch_data->reactive_power = AirSwitch_TM_Data.reactive_power;
//		air_switch_data->active_ele = AirSwitch_TM_Data.active_ele;
//		air_switch_data->reactive_ele = AirSwitch_TM_Data.reactive_ele;
//		air_switch_data->freq = AirSwitch_TM_Data.freq;
//		air_switch_data->switch_sta = AirSwitch_TM_Data.switch_sta;
//		air_switch_data->active_reason = AirSwitch_TM_Data.active_reason;
		memcpy(air_switch_data, AirSwitch_TM_Data.data_org, sizeof(AirSwitch_TM_Data));
	}
	else if(read_or_write == 1)
	{
//		AirSwitch_TM_Data.voltage = air_switch_data->voltage;
//		AirSwitch_TM_Data.current = air_switch_data->current;
//		AirSwitch_TM_Data.active_power = air_switch_data->active_power;
//		AirSwitch_TM_Data.reactive_power = air_switch_data->reactive_power;
//		AirSwitch_TM_Data.active_ele = air_switch_data->active_ele;
//		AirSwitch_TM_Data.reactive_ele = air_switch_data->reactive_ele;
//		AirSwitch_TM_Data.freq = air_switch_data->freq;
//		AirSwitch_TM_Data.switch_sta = air_switch_data->switch_sta;
//		AirSwitch_TM_Data.active_reason = air_switch_data->active_reason;
		memcpy(AirSwitch_TM_Data.data_org, air_switch_data, sizeof(AirSwitch_TM_Data));
	}
	xSemaphoreGive (EnvironmentMutex);
}

//上海默律倾角传感器角度获取
void Get_MTC70M_Angle_Data(void)
{
	//01 03 00 01 00 04 15 C9
	uint8_t tempbuff[8] = {0x0A,0x03,0x00,0x02,0x00,0x04,0xE4,0xB2};
	
	Environment_Dir(GPIO_PIN_SET);
    Environment_Send(tempbuff,8);
    Environment_Dir(GPIO_PIN_RESET);
}

//上海默律倾角传感器角度获取
void Get_ManholeCover_MTC70M_Angle_Data(void)
{
	//01 03 00 01 00 04 15 C9
	uint8_t tempbuff[8] = {0x0C,0x03,0x00,0x02,0x00,0x04,0xE4,0xD4};
	
	Environment_Dir(GPIO_PIN_SET);
    Environment_Send(tempbuff,8);
    Environment_Dir(GPIO_PIN_RESET);
}

//透明电力智能空开控制
void Ctrl_TM_AirSwitch(uint8_t ctrl_data)
{
	//合闸操作的报文为： 01 06 00 04 55 AA 77 24
	//分闸操作的报文为： 01 06 00 04 55 CC F7 0E
	uint8_t tempbuff0[8] = {0x0B,0x06,0x00,0x04,0x55,0xCC,0xF7,0xA4};
	uint8_t tempbuff1[8] = {0x0B,0x06,0x00,0x04,0x55,0xAA,0x77,0x8E};
	
	Environment_Dir(GPIO_PIN_SET);
	if(ctrl_data == 0)
	{
		Environment_Send(tempbuff0,8);
	}
	else
	{
		Environment_Send(tempbuff1,8);
	}
    Environment_Dir(GPIO_PIN_RESET);
}
//透明电力智能空开数据获取
void Get_TM_AirSwitch_Data(void)
{
	//遥测功能的报文为： 0B 04 00 00 00 17 B0 AE
	uint8_t tempbuff[8] = {0x0B,0x04,0x00,0x00,0x00,0x23,0xB1,0x79};
	
	Environment_Dir(GPIO_PIN_SET);
	Environment_Send(tempbuff,8);
    Environment_Dir(GPIO_PIN_RESET);
}

//#if USE_TEN_IN_ONE_DEVICE == 1
void Get_TEN_IN_ONE_Environment_Data(void)
{
//	uint8_t tempbuff[8] = {0xFF,0x03,0x00,0x01,0x00,0x12,0x81,0xd9};
	uint8_t tempbuff[8] = {0xFF,0x03,0x00,0x01,0x00,0x1f,0x40,0x1c};
	
	Environment_Dir(GPIO_PIN_SET);
    Environment_Send(tempbuff,8);
    Environment_Dir(GPIO_PIN_RESET);
//    vTaskDelay(2000);
}
void Get_WS301_Environment_Data(void)
{
//	uint8_t tempbuff[8] = {0xFF,0x03,0x00,0x00,0x00,0x03,0x10,0x15};
	uint8_t tempbuff[8] = {0x15,0x03,0x00,0x00,0x00,0x03,0x06,0xdf};
	
	Environment_Dir(GPIO_PIN_SET);
    Environment_Send(tempbuff,8);
    Environment_Dir(GPIO_PIN_RESET);
//    vTaskDelay(2000);
}
//#endif

void Get_SM5386_WindSpeed_Data(void)
{
	uint8_t Tx485Buffer1[8] = {0x01,0x03,0x00,0x00,0x00,0x01,0x84,0x0A};
	
	Environment_Dir(GPIO_PIN_SET);
	Environment_Send(Tx485Buffer1,8);
	Environment_Dir(GPIO_PIN_RESET);
}

void Get_SM5387B_WindDiret_Data(void)
{
	uint8_t Tx485Buffer2[8] = {0x02,0x03,0x00,0x00,0x00,0x01,0x84,0x39};
	
	Environment_Dir(GPIO_PIN_SET);
	Environment_Send(Tx485Buffer2,8);
	Environment_Dir(GPIO_PIN_RESET);
}

void Get_SM6333B_TempHum_Data(void)
{
	uint8_t Tx485Buffer3[8] = {0x03,0x03,0x00,0x00,0x00,0x04,0x45,0xEB};
	
	Environment_Dir(GPIO_PIN_SET);
	Environment_Send(Tx485Buffer3,8);
	Environment_Dir(GPIO_PIN_RESET);
}

//#if USE_MULTIPLE_DEVICE == 1
void Get_MULTIPLE_Environment_Data(void)
{
	uint8_t Tx485Buffer1[8] = {0x01,0x03,0x00,0x00,0x00,0x01,0x84,0x0A};
	uint8_t Tx485Buffer2[8] = {0x02,0x03,0x00,0x00,0x00,0x01,0x84,0x39};
	uint8_t Tx485Buffer3[8] = {0x03,0x03,0x00,0x00,0x00,0x04,0x45,0xEB};
	static uint8_t send_cnt = 0;
	
	if(send_cnt == 0x00)
	{
		Environment_Dir(GPIO_PIN_SET);
		Environment_Send(Tx485Buffer1,8);
		Environment_Dir(GPIO_PIN_RESET);
		send_cnt = 0x01;
	}
//    vTaskDelay(1000);
	else if(send_cnt == 0x01)
	{
		Environment_Dir(GPIO_PIN_SET);
		Environment_Send(Tx485Buffer2,8);
		Environment_Dir(GPIO_PIN_RESET);
		send_cnt = 0x02;
	}
//    vTaskDelay(1000);
	
	else if(send_cnt == 0x02)
	{
		Environment_Dir(GPIO_PIN_SET);
		Environment_Send(Tx485Buffer3,8);
		Environment_Dir(GPIO_PIN_RESET);
		send_cnt = 0x00;
	}
}
//#endif

void Environment_Type_Test(void)
{
	uint8_t temp_type;
	uint8_t Tx485Buffer1[8] = {0x01,0x03,0x00,0x00,0x00,0x01,0x84,0x0A};
	uint8_t tempbuff[8] = {0xFF,0x03,0x00,0x01,0x00,0x12,0x81,0xd9};
	uint8_t tempbuff2[8] = {0xFF,0x03,0x00,0x00,0x00,0x03,0x10,0x15};
	
    Environment_Dir(GPIO_PIN_SET);
    Environment_Send(tempbuff,8);
    Environment_Dir(GPIO_PIN_RESET);
	vTaskDelay(1000);
	
	Environment_device_Type(0,&temp_type);
	if(temp_type == 1)
	{
		myprintf("\r\nEnvironment_device: 十合一传感器");
		return;
	}
    Environment_Dir(GPIO_PIN_SET);
    Environment_Send(tempbuff2,8);
    Environment_Dir(GPIO_PIN_RESET);
	vTaskDelay(1000);
	
	Environment_device_Type(0,&temp_type);
	if(temp_type == 2)
	{
		myprintf("\r\nEnvironment_device: RY-WS301温湿度传感器");
		return;
	}
	
	Environment_Dir(GPIO_PIN_SET);
    Environment_Send(Tx485Buffer1,8);
    Environment_Dir(GPIO_PIN_RESET);
    vTaskDelay(1000);
	Environment_device_Type(0,&temp_type);
	if(temp_type == 0)
	{
		myprintf("\r\nEnvironment_device: 组合传感器");
		return;
	}
}
void EnvironmentData_Task(void)
{
	static uint32_t type_test_tick = 0;
	static uint32_t environment_sample_tick = 0;
//	Set_Sensor_Addr();

	if((HAL_GetTick() - type_test_tick) >= 60*1000 || type_test_tick == 0)
	{
		Environment_Type_Test();
		type_test_tick = HAL_GetTick();
	}
	if((HAL_GetTick() - environment_sample_tick) >= 5*1000 || environment_sample_tick == 0)
	{
		uint8_t temp_type;
		
		Environment_device_Type(0,&temp_type);
		if(temp_type == 0)
			Get_MULTIPLE_Environment_Data();
		else if(temp_type == 1)
			Get_TEN_IN_ONE_Environment_Data();
		environment_sample_tick = HAL_GetTick();
	}
}

//使用内部flash存储耗电量
typedef struct
{
	uint32_t write_addr;
	uint32_t read_addr;
}SaveEnergy_Def;
SaveEnergy_Def SaveEnergy_data;

//写用户区域FLASH并且保存耗电量
uint8_t Write_UserFlashAndSaveEnergy(uint32_t _ulFlashAddr, uint8_t *_ucpSrc, uint32_t _ulSize)
{
	uint32_t i;
	uint8_t ucRet;
	unsigned char *userBuf = NULL;
	uint32_t tmpEnergy[2];

	/* 如果偏移地址超过芯片容量，则不改写输出缓冲区 */
	if (_ulFlashAddr + _ulSize > FLASH_BASE_ADDR + FLASH_SIZE)
	{
		return 1;
	}

	/* 长度为0时不继续操作  */
	if (_ulSize == 0)
	{
		return 0;
	}

	ucRet = bsp_CmpCpuFlash(_ulFlashAddr, _ucpSrc, _ulSize);

	/* Flash内容和待写入的数据相等，不需要擦除和写操作 */
	if (ucRet == FLASH_IS_EQU)
	{
		return 0;
	}
	userBuf = malloc(USER_SECTOR_USE_LEN);
	if(userBuf == NULL)
		return 1;
	//读出用户扇区使用的数据
	bsp_ReadCpuFlash(USER_DATA_ADDR,userBuf,USER_SECTOR_USE_LEN);
	for(i=0;i<_ulSize;i++)			//复制
	{
		userBuf[_ulFlashAddr-USER_DATA_ADDR+i] = _ucpSrc[i];
	}
	/* 需要擦除 */
	if (ucRet == FLASH_REQ_ERASE)
	{
		//读出最后存储的能量
		bsp_ReadCpuFlash (SaveEnergy_By_Flash_Init(), (uint8_t *)&tmpEnergy, sizeof(tmpEnergy));			//能量
		MyFLASH_Erase(_ulFlashAddr,_ulFlashAddr+_ulSize);
	}
	
	__set_PRIMASK(1);  		/* 关中断 */

	/* FLASH 解锁 */
	MyFLASH_Unlock();

	/* 按字节模式编程（为提高效率，可以按字编程，一次写入4字节） */
	MyFLASH_Write(USER_DATA_ADDR,userBuf,USER_SECTOR_USE_LEN);

  	/* Flash 加锁，禁止写Flash控制寄存器 */
	MyFLASH_Lock();

  	__set_PRIMASK(0);  		/* 开中断 */
	free(userBuf);

	if (ucRet == FLASH_REQ_ERASE)
	{
		//由于擦除需要将能量写回flash
		xSemaphoreTake (EnvironmentMutex, portMAX_DELAY);
		SaveEnergy_data.write_addr = ENERGY_ADDR;
		xSemaphoreGive (EnvironmentMutex);
		SaveEnergy_By_Flash((uint8_t *)&tmpEnergy, sizeof(tmpEnergy));
	}
	return 0;
}

uint32_t SaveEnergy_By_Flash_Init(void)
{
	uint32_t i = 0;
	uint32_t energy[2];
	uint32_t energy_tmp[2] = {0xffffffff, 0xffffffff};
	uint8_t energy_len;
	
	energy_len = sizeof(energy);
	
	//预留64Kflash给耗电量存储，64*1024/8=8192
	for(i=0; i<64*1024/energy_len; i++)
	{
		W25QXX_Read((uint8_t *)energy, (ENERGY_ADDR+i*energy_len), energy_len);   //读取flash
		if(memcmp((const void *)energy,(const void *)energy_tmp,energy_len) == 0)
		{
			break;
		}
		if(i == 64*1024/energy_len-1)
		{
			//需要整个片区擦除,后续调用SaveEnergy_By_Flash会进行擦除
		}
	}
	SaveEnergy_data.write_addr = ENERGY_ADDR+i*energy_len;
	if(SaveEnergy_data.write_addr <= ENERGY_ADDR)
	{
		return SaveEnergy_data.write_addr;
	}
	else
	{
		return SaveEnergy_data.write_addr-energy_len;
	}
}

void SaveEnergy_By_Flash(uint8_t *energy, uint8_t datalen)
{
	//一旦flash地址超过界限则需要擦除
	if(SaveEnergy_data.write_addr >= (W25XX_PAGE_SIZE*W25XX_SECTOR_SIZE))
	{		
		SaveEnergy_data.write_addr = ENERGY_ADDR;
				
		W25QXX_Erase_NSector(ENERGY_ADDR, 64*1024/W25XX_SECTOR_SIZE);
		
		//将耗电量存入flash 并地址增加
		W25QXX_Write(energy, SaveEnergy_data.write_addr, datalen);//写入flash
//		SaveEnergy_data.write_addr += datalen;
	}
	else
	{
		//将耗电量存入flash 并地址增加
		W25QXX_Write(energy, SaveEnergy_data.write_addr, datalen);//写入flash
		SaveEnergy_data.write_addr += datalen;
	}
}

uint8_t Get_Electric(void)
{
    static uint8_t err_cnt = 0;
    uint8_t led[LED_NUM],pwm[LED_NUM];
    CollectionData electricdata[LED_NUM];
	uint32_t energy[LED_NUM];
	uint8_t updata_flag = 0;
	uint8_t i = 0;
	
	for(i=0; i<LED_NUM; i++)
	{
		Get_LED_Data(i+1,&led[i],&pwm[i]);
		Get_ElectricData(i+1, &electricdata[i]);
		energy[i] = electricdata[i].Energy;
		if(BL0942_ReadEData(i+1, &electricdata[i]))
		{
			BL0942_ConfigInit(i+1);
		//初始化后,由于电量计技术清零，需要重新将flash中的值附到SAVE_ENERGY中
			W25QXX_Read((uint8_t *)&SaveEnergy, SaveEnergy_By_Flash_Init(), 4*LED_NUM);			//能量
			for(uint8_t j=0; j<LED_NUM; j++)
			{
				if(SaveEnergy[j] == 0xffffffff )
				{
					SaveEnergy[j] = 0;
				}
			}
			return 0;
		}
		if(led[i] == 1 && pwm[i]>=7 && (electricdata[i].Voltage==0 || electricdata[i].Current==0)) {
			electricdata[i].Bl6526bState |= 0x08;
		} else {
			electricdata[i].Bl6526bState &= ~0x08;
		}
		Write_ElectricData(i+1, &electricdata[i]);
	}
	
	//当耗电量超过0.01kwh后存到flash
	#if LED_NUM == 1
	if(electricdata[0].Energy + SaveEnergy[0] - energy[0] >= 1)
	{
		if(electricdata[0].Energy + SaveEnergy[0] >= energy[0])
		{
			energy[0] = electricdata[0].Energy + SaveEnergy[0];
//			Write_UserMem_Flash(ENERGY_ADDR,(uint8_t *)&energy, sizeof(energy));
			SaveEnergy_By_Flash((uint8_t *)&energy, sizeof(energy));
			updata_flag = 1;
		}
	}
	if(electricdata[0].Bl6526bState)
    {
        err_cnt++;
        if(err_cnt>=3)
        {
            err_cnt = 0;
            myprintf("\r\nBl6526bState1:%d",electricdata[0].Bl6526bState);
			xSemaphoreGive(AlarmBinary);			//报警
        }
    }
    else {
        err_cnt = 0;
    }
	#elif LED_NUM == 2
	if(electricdata[0].Energy + SaveEnergy[0] - energy[0] >= 1 ||\
		electricdata[1].Energy + SaveEnergy[1] - energy[1] >= 1)
	{
		if(electricdata[0].Energy + SaveEnergy[0] >= energy[0] &&\
			electricdata[1].Energy + SaveEnergy[1] >= energy[1])
		{
			energy[0] = electricdata[0].Energy + SaveEnergy[0];
			energy[1] = electricdata[1].Energy + SaveEnergy[1];
//			Write_UserMem_Flash(ENERGY_ADDR,(uint8_t *)&energy, sizeof(energy));
			SaveEnergy_By_Flash((uint8_t *)&energy, sizeof(energy));
			updata_flag = 1;
		}
	}
    if(electricdata[0].Bl6526bState || electricdata[1].Bl6526bState)
    {
        err_cnt++;
        if(err_cnt>=3)
        {
            err_cnt = 0;
            myprintf("\r\nBl6526bState1:%d,Bl6526bState2:%d",electricdata[0].Bl6526bState,electricdata[1].Bl6526bState);
			xSemaphoreGive(AlarmBinary);			//报警
        }
    }
    else {
        err_cnt = 0;
    }
	#endif
	return updata_flag;
}



void CheckSelf_Flag_Rw(uint8_t rw, uint8_t * checkflag)
{
	xSemaphoreTake (EnvironmentMutex, portMAX_DELAY);
	
	if(rw == 0)
		*checkflag = CheckSelfFlag;
	else
		CheckSelfFlag = *checkflag;
	xSemaphoreGive (EnvironmentMutex);
}

uint8_t CheckSelf(void)
{
    uint8_t check_flag = 0;
	
	CheckSelf_Flag_Rw(0, &check_flag);
    if(BL0942_CheckSelf(1))
    {
        check_flag |= 0x01;
        myprintf("\r\n电量计一 ERROR！！！");
    }
    else
        myprintf("\r\n电量计一 SUCCESS！！！");
	if(BL0942_CheckSelf(2))
	{
		check_flag |= 0x02;
		myprintf("\r\n电量计二 ERROR！！！");
	}
	else
		myprintf("\r\n电量计二 SUCCESS！！！");
	CheckSelf_Flag_Rw(1, &check_flag);
    return check_flag;
}



void Sensor_485_Use_Info_Print(SENSOR_485_USE_TYPE sensor_use_data)
{
	myprintf("\r\n本机配置使用了以下传感器，基于485通讯：%X",sensor_use_data);
	if(sensor_use_data.bit_use.SM5387B)
		myprintf("\r\n搜博风向传感器-->已使用");
	if(sensor_use_data.bit_use.SM5386)
		myprintf("\r\n搜博风速传感器-->已使用");
	if(sensor_use_data.bit_use.SM6333B)
		myprintf("\r\n搜博温湿度粉尘传感器-->已使用");
	if(sensor_use_data.bit_use.MTC70M)
		myprintf("\r\n默律倾角传感器-->已使用");
	if(sensor_use_data.bit_use.IOTB2_16)
		myprintf("\r\n透明物联智能空开-->已使用");
	if(sensor_use_data.bit_use.IOTB2_16)
		myprintf("\r\n透明物联智能空开-->已使用");
	if(sensor_use_data.bit_use.WS301)
		myprintf("\r\n耘农温湿度传感器-->已使用");
	if(sensor_use_data.bit_use.HCD68XX)
		myprintf("\r\n耘农多合一传感器-->已使用");
	if(sensor_use_data.bit_use.ManholeCover_MTC70M)
		myprintf("\r\n默律倾角模拟井盖-->已启用");
}

void Sensor_485Addr_Task(void)
{
	uint32_t get_sensor_tick = 0;
	uint8_t i = 0;
	SENSOR_485_USE_TYPE sensor_use_data;
	
	SENSOR_485_USE_Data_RW(0, &sensor_use_data);
	if((HAL_GetTick() - get_sensor_tick) >= 1000 || get_sensor_tick == 0)
	{
		uint8_t airswitch_ctrl_data;
		get_sensor_tick = HAL_GetTick();
		if(xQueueReceive( TM_AirSwitch_Queue, (void *)&airswitch_ctrl_data, ( TickType_t ) 0) == pdPASS)	//获取空开队列
		{
			Ctrl_TM_AirSwitch(airswitch_ctrl_data);
			return;
		}
		for(i=1; i<=32; i++)
		{
			//找到下一个已经选用的传感器
			if((sensor_use_data.byte_use>>(i-1)) & 0x01)
			{
				if(Sensor_485Addr_GetCnt < i)
				{
					Sensor_485Addr_GetCnt = i;
					break;
				}
			}
			else
			{
				//剩余已经不存在使用的传感器后，退出，下一个循环
				if(sensor_use_data.byte_use>>(i-1) == 0x00)
				{
					Sensor_485Addr_GetCnt = 0;
					break;
				}
			}
		}
		switch (Sensor_485Addr_GetCnt)
		{
			case 0:
				break;
			case 1:
				Get_SM5386_WindSpeed_Data();
				break;
			case 2:
				Get_SM5387B_WindDiret_Data();
				break;
			case 3:
				Get_SM6333B_TempHum_Data();
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				break;
			case 0x0A:
				Get_MTC70M_Angle_Data();
				break;
			case 0x0B:
				Get_TM_AirSwitch_Data();
				break;
			case 0x0C:
				Get_WS301_Environment_Data();
				break;
			case 0x0D:
				Get_TEN_IN_ONE_Environment_Data();
				break;
			case 0x0E:
				Get_ManholeCover_MTC70M_Angle_Data();
				break;
			default: break;
		
		}
	}
}

void Environment_Data_Task(void *arg)
{
	uint32_t type_test_tick = 0;
	uint32_t environment_sample_tick = 0;
	uint32_t electric_tick = 0;
	uint32_t ele_reinit_tick = 0;
//	Set_Sensor_Addr();
	uint8_t temp_type = 0xff;
	uint8_t checkfalg;
	uint8_t led[LED_NUM],pwm[LED_NUM];
	uint8_t oldled[LED_NUM],oldpwm[LED_NUM];
	uint32_t energy_save_tick = 0;
	uint8_t i = 0;
	
	
	for(i=0; i<LED_NUM; i++)
	{
		oldled[i] = 0;
		oldpwm[i] = 0;
	}
	
//	BL0942_ConfigInit(1);
//	BL0942_ConfigInit(2);
//	checkfalg = CheckSelf();
//	CheckSelf_Flag_Rw(1,&checkfalg);
	
	#ifdef ELE_TRIM
	if(IDStatus == 0)			//未预制ID
	{
		uint8_t ledsta = 0;
		uint8_t ucKeyCode;		/* 按键代码 */
		while(1)
		{
			ucKeyCode = bsp_GetKey();	/* 读取键值, 无键按下时返回 KEY_NONE = 0 */
			if (ucKeyCode != KEY_NONE)
			{
				switch (ucKeyCode)
				{
					case KEY_DOWN_K1:			/* K1键按下 */
					{
						uint8_t ledstatus = 1,ledpwm = 100;
						myprintf("K1键按下\r\n");
						LedRun_Write(GPIO_PIN_RESET);
						Write_LED_Data(1,&ledstatus,&ledpwm);
						vTaskDelay(500);
						BL6523GX_Triming(1);
						ledstatus = 0,ledpwm = 0;
						Write_LED_Data(1,&ledstatus,&ledpwm);
						ledstatus = 1,ledpwm = 100;
						Write_LED_Data(2,&ledstatus,&ledpwm);
						vTaskDelay(500);
						BL6523GX_Triming(2);
						ledstatus = 0,ledpwm = 0;
						Write_LED_Data(2,&ledstatus,&ledpwm);
//						while(1)
						{
							if((U_k/VREF.f<=1000) || (U2_k/VREF.f<=1000) || (I_k/IREF.f<=100000) || (I2_k/IREF.f<=100000))
							{
//								HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin,GPIO_PIN_RESET);
								ledsta = 1;
							}
							else if(checkfalg)
							{
//								HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
								ledsta = 2;
							}
							else
							{
//								HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin,GPIO_PIN_SET);
								ledsta = 0;
								Write_UserMem_Flash(ENERGY_PARA_ADDR,(uint8_t *)&U_k, 4);
								Write_UserMem_Flash(ENERGY_PARA_ADDR+4,(uint8_t *)&I_k, 4);
								Write_UserMem_Flash(ENERGY_PARA_ADDR+8,(uint8_t *)&Watt_k, 4);
								Write_UserMem_Flash(ENERGY_PARA_ADDR+12,(uint8_t *)&U2_k, 4);
								Write_UserMem_Flash(ENERGY_PARA_ADDR+12+4,(uint8_t *)&I2_k, 4);
								Write_UserMem_Flash(ENERGY_PARA_ADDR+12+8,(uint8_t *)&Watt2_k, 4);
							}
//							vTaskDelay(100);
						}
//						ucKeyCode = bsp_GetKey();	/* 读取键值, 无键按下时返回 KEY_NONE = 0 */
						break;
					}
					case KEY_LONG_K1:			/* K1键长按 */
						myprintf("K1键长按\r\n");
						break;
				}
			}
			switch(ledsta)
			{
				case 0:
					LedRun_Write(GPIO_PIN_SET);
					break;
				case 1:
					LedRun_Write(GPIO_PIN_RESET);
					break;
				case 2:
					HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
					break;
				case 3:
					break;
			}
			vTaskDelay(100);
		}
	}
	#endif
	ele_reinit_tick = HAL_GetTick();
	while(1)
	{
		Sensor_485Addr_Task();
		if((HAL_GetTick() - ele_reinit_tick) >= 3600*24*1000)
		{
			for(i=0; i<LED_NUM; i++)
			{
				BL0942_ConfigInit(i+1);
				//初始化后,由于电量计技术清零，需要重新将flash中的值附到SAVE_ENERGY中
				W25QXX_Read((uint8_t *)&SaveEnergy, SaveEnergy_By_Flash_Init(), 4*LED_NUM);			//能量
				for(uint8_t j=0; j<LED_NUM; j++)
				{
					if(SaveEnergy[j] == 0xffffffff )
					{
						SaveEnergy[j] = 0;
					}
				}
			}
			checkfalg = CheckSelf();
			CheckSelf_Flag_Rw(1,&checkfalg);
			ele_reinit_tick = HAL_GetTick();
		}
		if((HAL_GetTick() - type_test_tick) >= 60*1000 || type_test_tick == 0 || temp_type == 0xff)
		{
//			Environment_Type_Test();
			type_test_tick = HAL_GetTick();
		}
		if((HAL_GetTick() - environment_sample_tick) >= 5*1000 || environment_sample_tick == 0 )
		{
//			Environment_device_Type(0,&temp_type);
//			if(temp_type == 0)
//				Get_MULTIPLE_Environment_Data();
//			else if(temp_type == 1)
//				Get_TEN_IN_ONE_Environment_Data();
//			else if(temp_type == 2)
//				Get_WS301_Environment_Data();
			environment_sample_tick = HAL_GetTick();
		}
		for(i=0; i<LED_NUM; i++)
		{
			Get_LED_Data(i+1,&led[i],&pwm[i]);
			if(oldled[i] != led[i] || oldpwm[i] != pwm[i])	//如果继电器状态或者亮度有异常则立马获取电压电流数据
			{
				memcpy(oldled, led, LED_NUM);
				memcpy(oldpwm, pwm, LED_NUM);
				vTaskDelay(1200);	//延迟400ms等待电量计采样
				Get_Electric();
				electric_tick = HAL_GetTick();
				xSemaphoreGive(EleUpBinary);			//数据更新
				break;
			}
		}
		
		if((HAL_GetTick() - electric_tick) >= 5*1000 || electric_tick == 0)
		{
			if(Get_Electric())
				xSemaphoreGive(EleUpBinary);			//数据更新
			electric_tick = HAL_GetTick();
		}
		vTaskDelay(1000);
	}
}

void CreatEnvironment_DataTask(void)
{
	myprintf("\r\n CreatEnvironment_DataTask\r\n");
	memset((Environment_Type *)&Environment_Data,0xff,sizeof(Environment_Type));
	
	TM_AirSwitch_Queue = xQueueCreate( 1, 1 );
	if(NULL == TM_AirSwitch_Queue)
	{
		_myprintf("\r\n TM_AirSwitch_Queue error\r\n");
		while(1);
	}
	AirSwitchBinary = xSemaphoreCreateBinary();
	if(NULL == AirSwitchBinary)
	{
		myprintf("\r\n AirSwitchBinary error");
		while(1);
	}
	xSemaphoreGive(AirSwitchBinary);
//	xSemaphoreTake(AirSwitchBinary, ( TickType_t ) 0);
	
	AngleBinary = xSemaphoreCreateBinary();
	if(NULL == AngleBinary)
	{
		myprintf("\r\n AngleBinary error");
		while(1);
	}
	xSemaphoreGive(AngleBinary);
//	xSemaphoreTake(AngleBinary, ( TickType_t ) 0);
	
	ManholeCoverBinary = xSemaphoreCreateBinary();
	if(NULL == ManholeCoverBinary)
	{
		myprintf("\r\n ManholeCoverBinary error");
		while(1);
	}
	xSemaphoreGive(ManholeCoverBinary);
//	xSemaphoreTake(ManholeCoverBinary, ( TickType_t ) 0);
	
    Environment_Data_TaskID = xTaskCreate(Environment_Data_Task, "EnvApp", Environment_Data_StkSIZE, NULL, Environment_Data_TaskPrio, &pvCreatedTask_Environment_Data);
    if(pdPASS != Environment_Data_TaskID)
    {
        myprintf("\r\nEnvironment_Data_Task creat error");
        while(1);
    }
}
