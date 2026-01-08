#include "Environment_data.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "SysConfig.h"
#include "rtc.h"
#include "Unixtimer.h"

uint8_t fenxiangAddr[8] = {0x01,0x06,0x00, 0x66,0x00,0x02,0xe8,0x14};
uint8_t wenshidugAddr[8] = {0x01,0x06,0x00, 0x66,0x00,0x03,0x29,0xD4};

Environment_Type Environment_Data;

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
	HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_SET);
    HAL_UART_Transmit(&huart7,fenxiangAddr,8,10000);
    HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_RESET);
	vTaskDelay(500);
//	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8,GPIO_PIN_SET);
//    HAL_UART_Transmit(&huart1,wenshidugAddr,8,10000);
//    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8,GPIO_PIN_RESET);
//	vTaskDelay(500);
}
//#if USE_TEN_IN_ONE_DEVICE == 1
void Get_TEN_IN_ONE_Environment_Data(void)
{
	uint8_t tempbuff[8] = {0xFF,0x03,0x00,0x01,0x00,0x12,0x81,0xd9};
//    HAL_GPIO_WritePin(USART2_DIR_GPIO_Port, USART2_DIR_Pin,GPIO_PIN_SET);
//    HAL_UART_Transmit(&huart2,tempbuff,8,10000);
//    HAL_GPIO_WritePin(USART2_DIR_GPIO_Port, USART2_DIR_Pin,GPIO_PIN_RESET);
	
	HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_SET);
    HAL_UART_Transmit(&huart7,tempbuff,8,10000);
    HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_RESET);
//    vTaskDelay(2000);
}
//#endif

//#if USE_MULTIPLE_DEVICE == 1
void Get_MULTIPLE_Environment_Data(void)
{
	uint8_t Tx485Buffer1[8] = {0x01,0x03,0x00,0x00,0x00,0x01,0x84,0x0A};
	uint8_t Tx485Buffer2[8] = {0x02,0x03,0x00,0x00,0x00,0x01,0x84,0x39};
	uint8_t Tx485Buffer3[8] = {0x03,0x03,0x00,0x00,0x00,0x04,0x45,0xEB};
	static uint8_t send_cnt = 0;
	
	if(send_cnt == 0x00)
	{
		HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_SET);
		HAL_UART_Transmit(&huart7,Tx485Buffer1,8,10000);
		HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_RESET);
		send_cnt = 0x01;
	}
//    vTaskDelay(1000);
	else if(send_cnt == 0x01)
	{
		HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_SET);
		HAL_UART_Transmit(&huart7,Tx485Buffer2,8,10000);
		HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_RESET);
		send_cnt = 0x02;
	}
//    vTaskDelay(1000);
	
	else if(send_cnt == 0x02)
	{
		HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_SET);
		HAL_UART_Transmit(&huart7,Tx485Buffer3,8,10000);
		HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_RESET);
		send_cnt = 0x00;
	}
}
//#endif

void Environment_Type_Test(void)
{
	uint8_t temp_type;
	uint8_t Tx485Buffer1[8] = {0x01,0x03,0x00,0x00,0x00,0x01,0x84,0x0A};
	uint8_t tempbuff[8] = {0xFF,0x03,0x00,0x01,0x00,0x12,0x81,0xd9};
	
    HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_SET);
    HAL_UART_Transmit(&huart7,tempbuff,8,10000);
    HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_RESET);
	vTaskDelay(1000);
	
	Environment_device_Type(0,&temp_type);
	if(temp_type == 1)
	{
		myprintf("\r\nEnvironment_device: 十合一传感器");
		return;
	}
	HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_SET);
    HAL_UART_Transmit(&huart7,Tx485Buffer1,8,10000);
    HAL_GPIO_WritePin(UART7_DIR_GPIO_Port, UART7_DIR_Pin,GPIO_PIN_RESET);
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
	}
}

void Environment_Data_Task(void *arg)
{
    static uint8_t sample_cnt = 0;
	uint32_t type_test_tick = 0;
	uint32_t environment_sample_tick = 0;
//	Set_Sensor_Addr();

	while(1)
	{
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
		}
		vTaskDelay(1000);
	}
	
    while(0)
    {
        sample_cnt++;
        if(sample_cnt == 5-2)
        {
			uint8_t temp_type;
			
            sample_cnt = 0;
			Environment_device_Type(0,&temp_type);
			if(temp_type == 0)
				Get_MULTIPLE_Environment_Data();
			else if(temp_type == 1)
				Get_TEN_IN_ONE_Environment_Data();
/*
#if USE_TEN_IN_ONE_DEVICE == 1
			Get_TEN_IN_ONE_Environment_Data();
#endif
#if USE_MULTIPLE_DEVICE == 1
            Get_MULTIPLE_Environment_Data();
#endif*/
        }
		vTaskDelay(1000);
    }
}

void CreatEnvironment_DataTask(void)
{
	_myprintf("\r\n CreatEnvironment_DataTask\r\n");
	memset((Environment_Type *)&Environment_Data,0xff,sizeof(Environment_Type));
    Environment_Data_TaskID = xTaskCreate(Environment_Data_Task, "Environment_Data", Environment_Data_StkSIZE, NULL, Environment_Data_TaskPrio, &pvCreatedTask_Environment_Data);
    if(pdPASS != Environment_Data_TaskID)
    {
        _myprintf("\r\nEnvironment_Data_Task creat error");
        while(1);
    }
}
