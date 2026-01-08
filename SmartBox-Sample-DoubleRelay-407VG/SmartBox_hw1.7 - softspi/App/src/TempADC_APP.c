/**
  ******************************************************************************
  * @file    adc.c
  * @brief   This file provides code for the configuration
  *          of the ADC instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "TempAdc_APP.h"

/* USER CODE BEGIN 0 */
#include "usart.h"
/* USER CODE END 0 */

ADC_HandleTypeDef hadc1;

/* ADC1 init function */
void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
	hadc1.Instance=ADC1;
    hadc1.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4;  //4分频，ADCCLK=PCLK2/4=90/4=22.5MHZ
    hadc1.Init.Resolution=ADC_RESOLUTION_12B;            //12位模式
    hadc1.Init.DataAlign=ADC_DATAALIGN_RIGHT;            //右对齐
    hadc1.Init.ScanConvMode=DISABLE;                     //非扫描模式
    hadc1.Init.EOCSelection=DISABLE;                     //关闭EOC中断
    hadc1.Init.ContinuousConvMode=DISABLE;               //关闭连续转换
    hadc1.Init.NbrOfConversion=1;                        //1个转换在规则序列中 也就是只转换规则序列1 
    hadc1.Init.DiscontinuousConvMode=DISABLE;            //禁止不连续采样模式
    hadc1.Init.NbrOfDiscConversion=0;                    //不连续采样通道数为0
    hadc1.Init.ExternalTrigConv=ADC_SOFTWARE_START;      //软件触发
    hadc1.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;//使用软件触发
    hadc1.Init.DMAContinuousRequests=DISABLE;            //关闭DMA请求
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspInit 0 */

  /* USER CODE END ADC1_MspInit 0 */
    /* ADC1 clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();
  /* USER CODE BEGIN ADC1_MspInit 1 */

  /* USER CODE END ADC1_MspInit 1 */
  }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */

  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();
  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
//得到温度值
//返回值:温度值(扩大了 100 倍,单位:℃.)
short Get_Temprate(void)
{
	uint32_t adcx;
	short result;
	double temperate;
	
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1,100);//轮询转换
//	while(HAL_IS_BIT_SET(HAL_ADC_GetState(&hadc1), HAL_ADC_STATE_REG_EOC) == 0)
		adcx = HAL_ADC_GetValue(&hadc1);	//获取转换值
	//读取内部温度传感器通道,10 次取平均
	temperate=(double)((float)adcx*3.3)/4096;
	//电压值
	temperate=((temperate-0.76)/0.0025 + 25); //转换为温度值

	result=temperate*100;
	//扩大 100 倍.
	return result;
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
