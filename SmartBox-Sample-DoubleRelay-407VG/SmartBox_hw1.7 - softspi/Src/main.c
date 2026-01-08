/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "iwdg.h"
#include "lwip.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "SysConfig.h"
#include "mystring.h"

#include "Unixtimer.h"
#include "Environment_data.h"
#include "Led_App.h"
#include "Uart_App.h"
#include "MyFlash.h"
#include "IDConfig.h"
#include "socketserver.h"
#include "Platform.h"
#include "StorageConfig.h"
#include "RemoteUpdata.h"
#include "BL0942.h"
#include "bsp_key.h"
#include "w25qxx.h"
#include "IAP.h"

#include <cm_backtrace.h>
#include "TempAdc_APP.h"
#include "WaterLevelMonitor.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
xTaskHandle pvCreatedTask_Start;
extern xTaskHandle RemoteUpdata_Task_Handle;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void MCU_RESET_STATUS(void);
void QueSemMuxInit(void);
void Start_Task(void * argument);
void CreatStartTask(void);
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	SCB->VTOR = (FLASH_BASE |0x20000);
  /* USER CODE END 1 */
	__enable_irq();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_IWDG_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_UART6_Init();
//  MX_UART8_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
	bsp_InitKey();
//	REL1_Write(REL_OFF);
	_myprintf("\r\nUser Uart Init success\r\n");
	MCU_RESET_STATUS();
	HAL_UART_Receive_IT(&huart1, (uint8_t *)&aRxBuffer, 1);
	HAL_UART_Receive_IT(&huart2, (uint8_t *)&aRxBuffer, 1);
	HAL_UART_Receive_IT(&huart3, (uint8_t *)&aRxBuffer, 1);
	HAL_UART_Receive_IT(&huart6, (uint8_t *)&aRxBuffer, 1);
	HAL_UART_Receive_IT(&huart8, (uint8_t *)&aRxBuffer, 1);
//	BL6526B_Init();
//	BL6523GX_Init(1);
//	BL6523GX_Init(2);
	/*使能定时器1中断*/
//	W25QXX_Init();
	HAL_TIM_Base_Start_IT(&htim3);
	
	cm_backtrace_init("SampleBox", "2.0.0", "2.0.24");

	MX_ADC1_Init();

	QueSemMuxInit();
	CreatStartTask();
	vTaskStartScheduler();//启动调度，开始执行任务	
//	while(1);
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in freertos.c) */
//  MX_FREERTOS_Init(); 
//  /* Start scheduler */
//  osKernelStart();
 
  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
		/* USER CODE END WHILE */
		
		/* USER CODE BEGIN 3 */
		HAL_IWDG_Refresh(&hiwdg);
		HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
		HAL_Delay(100);
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
#if 0
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_PWREx_EnableOverDrive(); //开启Over-Driver功能
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

#else
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 120;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
//  HAL_PWREx_EnableOverDrive(); //开启Over-Driver功能
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}
#endif
/* USER CODE BEGIN 4 */

void MCU_RESET_STATUS(void)
{
	if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) != RESET)
	{
		_myprintf("\r\n发生 POR/PDR 复位或 BOR 复位");
	}
	if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET)
	{
		_myprintf("\r\n发生来自 NRST 引脚的复位");
	}
	if(__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) != RESET)
	{
		_myprintf("\r\n发生 POR/PDR 复位");
	}
	if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)!= RESET)
	{
		_myprintf("\r\n发生软件复位");
	}
	if(__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
	{
		_myprintf("\r\n发生来自独立看门狗复位");
	}
	else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != RESET)
	{
		_myprintf("\r\n发生窗口看门狗复位");
	}
	else if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST)!= RESET)
	{
		_myprintf("\r\n发生低功耗管理复位");
	}
	__HAL_RCC_CLEAR_RESET_FLAGS();//清除RCC中复位标志
}
void QueSemMuxInit(void)
{
	PrintMutex = xSemaphoreCreateMutex();		//Print mutex	
	if(NULL == PrintMutex)
	{
		_myprintf("\r\n PrintMutex error\r\n");
		while(1);
	}
	EnvironmentMutex = xSemaphoreCreateMutex();		//Environment mutex	
	if(NULL == EnvironmentMutex)
	{
		_myprintf("\r\n EnvironmentMutex error\r\n");
		while(1);
	}
	ElectricDataMutex = xSemaphoreCreateMutex();		//ElectricData Mutex
	if(NULL == ElectricDataMutex)
	{
		_myprintf("\r\n ElectricDataMutex error\r\n");
		while(1);
	}
    BL0942_RECEIVE_Queue = xQueueCreate( ElectricData_QUEUE_LENGTH, ElectricData_Size );
    if(NULL == BL0942_RECEIVE_Queue)
    {
        _myprintf("\r\n BL6523GX_RECEIVE_Queue error");
        while(1);
    }
	TimeControlMutex = xSemaphoreCreateMutex();		//TimeControl Mutex
	if(NULL == TimeControlMutex)
	{
		_myprintf("\r\n TimeControlMutex error\r\n");
		while(1);
	}
	LedShowBinary = xSemaphoreCreateBinary();		//LedShow mutex	
	if(NULL == LedShowBinary)
	{
		_myprintf("\r\n LedShowBinary error\r\n");
		while(1);
	}
	xSemaphoreTake(LedShowBinary, 0);
	AlarmBinary = xSemaphoreCreateBinary();		//AlarmBinary	
	if(NULL == AlarmBinary)
	{
		_myprintf("\r\n AlarmBinary error\r\n");
		while(1);
	}
	xSemaphoreTake(AlarmBinary, 0);
	EleUpBinary = xSemaphoreCreateBinary();		//UpDataBinary
	if(NULL == EleUpBinary)
	{
		_myprintf("\r\n EleUpBinary error\r\n");
		while(1);
	}
	xSemaphoreTake(EleUpBinary, 0);
#if USE_ETH_TO_INTERNET
	TcpMutex = xSemaphoreCreateMutex();		//TcpMutex mutex	
	if(NULL == TcpMutex)
	{
		_myprintf("\r\n TcpMutex error\r\n");
		while(1);
	}
	TCP_SEND_Queue = xQueueCreate( TCP_SEND_QUEUE_LENGTH, 128 );
	if(NULL == TCP_SEND_Queue)
	{
		_myprintf("\r\n TCP_SEND_Queue error\r\n");
		while(1);
	}
	RemoteUpdataMutex = xSemaphoreCreateMutex();		//RemoteUpdataMutex mutex	
	if(NULL == RemoteUpdataMutex)
	{
		_myprintf("\r\n RemoteUpdataMutex error\r\n");
		while(1);
	}
	xSemaphoreTake (RemoteUpdataMutex, 0);
	RemoteUpdata_Queue = xQueueCreate( RemoteUpdata_QUEUE_LENGTH, PLATFORM_BUFF_SIZE );
	if(NULL == RemoteUpdata_Queue)
	{
		_myprintf("\r\n RemoteUpdata_Queue error\r\n");
		while(1);
	}
#endif
#if USE_4G_UART_TO_INTERNET
	GPRS_UART_SEND_Queue = xQueueCreate( GPRS_UART_SEND_QUEUE_LENGTH, 128 );
	if(NULL == GPRS_UART_SEND_Queue)
	{
		_myprintf("\r\n GPRS_4G_SEND_Queue error\r\n");
		while(1);
	}
	GPRS_RECEIVE_Queue = xQueueCreate( GPRS_RECEIVE_QUEUE_LENGTH, Uart3RxBufferSize+2 );
	if(NULL == GPRS_RECEIVE_Queue)
	{
		_myprintf("\r\n GPRS_RECEIVE_Queue error");
		while(1);
	}
#endif

}

void Start_Task(void * argument)
{
//	HAL_Delay(300);
	if(W25QXX_Init() == 1)
	{
		uint8_t check_flag = 0;
		
		CheckSelf_Flag_Rw(0, &check_flag);
		check_flag |= 0x08;
		CheckSelf_Flag_Rw(1, &check_flag);
	}
	ReadID();				//
	BL0942_Init();
//	BL0942_ConfigInit(1);
//	BL0942_ConfigInit(2);
	HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin,GPIO_PIN_SET);

//	CheckBackUp_Backup();
//	while(1)
//	{
//		vTaskDelay(1000);
//	}
	
	taskENTER_CRITICAL();           //进入临界区
	if(IDStatus == 1)
	{
#if USE_ETH_TO_INTERNET
		MX_LWIP_Init();			//lwip
//    	CreatTCPTask();
		CreatPlatformTask();	//平台交互任务
#endif
		CreatLedTask();			//led任务
		CreatUartTask();		//串口任务
		CreatEnvironment_DataTask();
#if USE_4G_UART_TO_INTERNET
//		CreatGRRSTask();
		CreatPlatformTask();
#endif
	}
	else if(IDStatus == 0)
	{
//		CreatLedTask();			//led任务
		CreatUartTask();
		CreatEnvironment_DataTask();
	}
	vTaskDelete(pvCreatedTask_Start);
    taskEXIT_CRITICAL();            //退出临界区
}

void CreatStartTask(void)
{
	BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
	//创建开始任务
    xReturn = xTaskCreate(Start_Task, "Start", Start_StkSIZE, NULL, Start_TaskPrio, &pvCreatedTask_Start);
    if(pdPASS != xReturn)
    {
        myprintf("\r\n Start_TaskID creat error");
        while(1);
    }
}

/*====================================================
 * 函数功能: 	空闲任务 钩函数
 * 输入参数:	无
 * 输出参数: 
 * 返    回:	无
=====================================================*/
uint32_t FreeRTOSRunTimeTick = 0;

void vApplicationIdleHook(void)
{
	static TickType_t osTickTemp_IdleHook = 0;
	
	HAL_IWDG_Refresh(&hiwdg);
	if((xTaskGetTickCount() - osTickTemp_IdleHook) >= 1000)	//1000ms
	{
#if configUSE_TRACE_FACILITY
		uint32_t TaskRunTimeTick = 1;
		UBaseType_t   ArraySize = 0;
		uint8_t task_state[6][20] = {"运行态","就绪态","阻塞态","挂起","任务删除但TCB未删除","无效状态"};
		uint8_t       x = 0;
		TaskStatus_t  *StatusArray;

		ArraySize = uxTaskGetNumberOfTasks(); //获取任务个数
		StatusArray = pvPortMalloc(ArraySize*sizeof(TaskStatus_t));
		
//		myprintf("\r\nosTickTemp_IdleHook 100ms %d", osTickTemp_IdleHook);
		if(StatusArray != NULL){ //内存申请成功

            ArraySize = uxTaskGetSystemState( (TaskStatus_t *)  StatusArray,
                                              (UBaseType_t   ) ArraySize,
                                              (uint32_t *    )  &TaskRunTimeTick );

            myprintf("\r\nTaskName   Priority   TaskNumber   MinStk   time   state\t\n");
            for(x = 0;x<ArraySize;x++)
			{
                myprintf("%s       %d       %d       %d       %d       %s\r\n",
				StatusArray[x].pcTaskName,
				(int)StatusArray[x].uxCurrentPriority,
				(int)StatusArray[x].xTaskNumber,
				(int)StatusArray[x].usStackHighWaterMark,
				(int)((float)StatusArray[x].ulRunTimeCounter/TaskRunTimeTick*100),
				task_state[StatusArray[x].eCurrentState]);
            }
            myprintf("\n\n");
        }
		vPortFree(StatusArray);
#endif
		osTickTemp_IdleHook = xTaskGetTickCount();
	}
}

void vApplicationStackOverflowHook( TaskHandle_t xTask,char * pcTaskName )
{
	myprintf("\r\n\t\t\t\t发生堆栈溢出:%s\t\t\t\t\r\n",pcTaskName);
}

/* USER CODE END 4 */

 /**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
unsigned int htim3_cnt = 0;
unsigned int ms_cnt = 0;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
    if (htim->Instance == TIM3)		//100khz  10us中断一次
    {
			FreeRTOSRunTimeTick++;
        htim3_cnt++;
        Led_Pwm_Control();
        if(htim3_cnt >= 10)	//1ms
        {
			ms_cnt++;
			htim3_cnt = 0;
			UartTimeOutCnt();
			WaterTimeProcess();
			if(ms_cnt % 10 == 0)
			{
				bsp_KeyScan10ms();
			}
			if (ms_cnt >= 1000)		//1000ms
			{
				ms_cnt = 0;
//            Switch_Brightness();
//			_myprintf("1s\n");
			}
        }
    }
  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
