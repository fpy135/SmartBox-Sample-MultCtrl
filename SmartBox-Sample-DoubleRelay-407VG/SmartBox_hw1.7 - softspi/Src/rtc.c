/**
  ******************************************************************************
  * File Name          : RTC.c
  * Description        : This file provides code for the configuration
  *                      of the RTC instances.
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

/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */
#include "SysConfig.h"
#include "Unixtimer.h"
#include "usart.h"

void GetRTC_BCD_NO_RTOS(rtc_time_t *rtc_time)
{
	RTC_DateTypeDef sdate;
	RTC_TimeTypeDef stime;
	
	/* Get the RTC current Time */
	HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BCD);
	/* Get the RTC current Date */
	HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BCD);
//    RtcTime.ui8Year = sdate.Year+2000;
//    RtcTime.ui8Month = sdate.Month;
//    RtcTime.ui8DayOfMonth = sdate.Date;
//    RtcTime.ui8Hour = stime.Hours;
//    RtcTime.ui8Minute = stime.Minutes;
//    RtcTime.ui8Second = stime.Seconds;
	
	rtc_time->ui8Year = sdate.Year;
    rtc_time->ui8Month = sdate.Month;
    rtc_time->ui8DayOfMonth = sdate.Date;
    rtc_time->ui8Hour = stime.Hours;
    rtc_time->ui8Minute = stime.Minutes;
    rtc_time->ui8Second = stime.Seconds;
	
}

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /** Initialize RTC Only 
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */
    
  {
	rtc_time_t rtc_time;
	  
	GetRTC_BCD_NO_RTOS(&rtc_time);
	if(HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR1)!= 0x5051)
	{
	  /** Initialize RTC and set the Time and Date
	  */
	  sTime.Hours = 0;
	  sTime.Minutes = 0;
	  sTime.Seconds = 1;

	  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
	  {
		Error_Handler();
	  }
	  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
	  sDate.Month = RTC_MONTH_JANUARY;
	  sDate.Date = 0;
	  sDate.Year = 0;

	  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
	  {
		Error_Handler();
	  }
	}
	else
	{
//		RTC_DateTypeDef sdate;
//		RTC_TimeTypeDef stime;
		/* Get the RTC current Time */
		HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
		/* Get the RTC current Date */
		HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
		_myprintf("\r\n上电时间：%02d-%02d-%02d  %02d:%02d:%02d ", sDate.Year+2000, sDate.Month,\
		sDate.Date, sTime.Hours, sTime.Minutes, sTime.Seconds);
//		DateToUpdate.Year    = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
//		DateToUpdate.Month   = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR3);
//		DateToUpdate.Date    = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR4);
//		DateToUpdate.WeekDay = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR5);
//		_myprintf("\r\n备份时间：%02d-%02d-%02d  %02d:%02d:%02d ", DateToUpdate.Year+2000, DateToUpdate.Month,\
//			DateToUpdate.Date, stime.Hours, stime.Minutes, stime.Seconds);
		if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
		{
			Error_Handler();
		}
	}
  }
  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date 
  */
//  sTime.Hours = 0x0;
//  sTime.Minutes = 0x0;
//  sTime.Seconds = 0x0;
//  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
//  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
//  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
//  sDate.Month = RTC_MONTH_JANUARY;
//  sDate.Date = 0x1;
//  sDate.Year = 0x0;

//  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
//  {
//    Error_Handler();
//  }

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */
    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
} 

/* USER CODE BEGIN 1 */


void SetRTC(rtc_time_t *rtc_time)
{
    RTC_DateTypeDef sdate;
	RTC_TimeTypeDef stime;

	taskENTER_CRITICAL();
	
	RtcTime.ui8Year = rtc_time->ui8Year;
    RtcTime.ui8Month = rtc_time->ui8Month;
    RtcTime.ui8DayOfMonth = rtc_time->ui8DayOfMonth;
    RtcTime.ui8Hour = rtc_time->ui8Hour;
    RtcTime.ui8Minute = rtc_time->ui8Minute;
    RtcTime.ui8Second = rtc_time->ui8Second;
	
	sdate.Year = RtcTime.ui8Year-2000;
    sdate.Month = RtcTime.ui8Month;
    sdate.Date = RtcTime.ui8DayOfMonth;
	sdate.WeekDay = 0x01;

	/* Set the RTC current Date */
	HAL_RTC_SetDate(&hrtc, &sdate, RTC_FORMAT_BIN);	
	
    stime.Hours = RtcTime.ui8Hour;
    stime.Minutes = RtcTime.ui8Minute;
    stime.Seconds = RtcTime.ui8Second;
	stime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	stime.StoreOperation = RTC_STOREOPERATION_RESET;
	/* Set the RTC current Time */
	HAL_RTC_SetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	
	taskEXIT_CRITICAL();
}

void GetRTC(rtc_time_t *rtc_time)
{
	RTC_DateTypeDef sdate;
	RTC_TimeTypeDef stime;
	
	taskENTER_CRITICAL();

	/* Get the RTC current Time */
	HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	/* Get the RTC current Date */
	HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
    RtcTime.ui8Year = sdate.Year+2000;
    RtcTime.ui8Month = sdate.Month;
    RtcTime.ui8DayOfMonth = sdate.Date;
    RtcTime.ui8Hour = stime.Hours;
    RtcTime.ui8Minute = stime.Minutes;
    RtcTime.ui8Second = stime.Seconds;
	
	rtc_time->ui8Year = RtcTime.ui8Year;
    rtc_time->ui8Month = RtcTime.ui8Month;
    rtc_time->ui8DayOfMonth = RtcTime.ui8DayOfMonth;
    rtc_time->ui8Hour = RtcTime.ui8Hour;
    rtc_time->ui8Minute = RtcTime.ui8Minute;
    rtc_time->ui8Second = RtcTime.ui8Second;
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x5051);//向指定的后备区域寄存器写入数据
	taskEXIT_CRITICAL();

}


/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
