/**
  ******************************************************************************
  * File Name          : USART.h
  * Description        : This file provides code for the configuration
  *                      of the USART instances.
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __usart_H
#define __usart_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef huart6;
extern UART_HandleTypeDef huart7;
extern UART_HandleTypeDef huart8;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

/* USER CODE BEGIN Private defines */
#define UART1_BAUD		115200		//用户串口
#define UART2_BAUD		0
#define UART3_BAUD		0
#define UART4_BAUD		0
#define UART5_BAUD		0
#define UART6_BAUD		0
#define UART7_BAUD		0
#define UART8_BAUD		0

#define PrintWrite				UART1Write

#define	UART_TimeOUT_MAX   		10		//10ms超时
/* USER CODE END Private defines */

void MX_UART4_Init(void);
void MX_UART5_Init(void);
void MX_UART6_Init(void);
void MX_UART7_Init(void);
void MX_UART8_Init(void);
void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void MX_USART3_UART_Init(void);

/* USER CODE BEGIN Prototypes */
extern uint8_t aRxBuffer;			//接收中断缓冲

#if UART1_BAUD
#define Uart1RxBufferSize		256		//最大接收字节数
extern uint8_t Uart1RxBuffer[Uart1RxBufferSize];	//接收数据
extern uint16_t Uart1_Rx_Cnt;			//接收缓冲计数
extern uint8_t Uart1_TimeOut;
extern void UART1Write(uint8_t *buf, uint16_t len);
#endif

#if UART2_BAUD
#define Uart2RxBufferSize		128		//最大接收字节数
extern uint8_t Uart2RxBuffer[Uart2RxBufferSize];	//接收数据
extern uint16_t Uart2_Rx_Cnt;			//接收缓冲计数
extern uint8_t Uart2_TimeOut;
extern void UART2Write(uint8_t *buf, uint16_t len);

#endif

#if UART3_BAUD
#define Uart3RxBufferSize		256		//最大接收字节数
extern uint8_t Uart3RxBuffer[Uart3RxBufferSize];	//接收数据
extern uint16_t Uart3_Rx_Cnt;			//接收缓冲计数
extern uint8_t Uart3_TimeOut;		//用户串口接受超时
extern void UART3Write(uint8_t *buf, uint16_t len);
#endif

#if UART5_BAUD
#define Uart5RxBufferSize		256		//最大接收字节数
extern uint8_t Uart5RxBuffer[Uart5RxBufferSize];	//接收数据
extern uint16_t Uart5_Rx_Cnt;			//接收缓冲计数
extern uint8_t Uart5_TimeOut;
extern void UART5Write(uint8_t *buf, uint16_t len);
#endif

#if UART6_BAUD
#define Uart6RxBufferSize		128		//最大接收字节数
extern uint8_t Uart6RxBuffer[Uart6RxBufferSize];	//接收数据
extern uint16_t Uart6_Rx_Cnt;			//接收缓冲计数
extern uint8_t Uart6_TimeOut;
extern void UART6Write(uint8_t *buf, uint16_t len);
#endif

#if UART7_BAUD
#define Uart7RxBufferSize		128		//最大接收字节数
extern uint8_t Uart7RxBuffer[Uart7RxBufferSize];	//接收数据
extern uint16_t Uart7_Rx_Cnt;			//接收缓冲计数
extern uint8_t Uart7_TimeOut;
extern void UART7Write(uint8_t *buf, uint16_t len);
#endif

#if UART8_BAUD
#define Uart8RxBufferSize		256		//最大接收字节数
extern uint8_t Uart8RxBuffer[Uart8RxBufferSize];	//接收数据
extern uint16_t Uart8_Rx_Cnt;					//接收缓冲计数
extern uint8_t Uart8_TimeOut;
extern void UART8Write(uint8_t *buf, uint16_t len);
#endif

extern void myprintf(char *s, ...);
extern void _myprintf(char *s, ...);
extern void myprintf_buf_unic(uint8_t *s, uint16_t len);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ usart_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
