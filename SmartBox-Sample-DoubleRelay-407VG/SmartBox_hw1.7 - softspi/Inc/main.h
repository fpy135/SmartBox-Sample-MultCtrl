/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED0_Pin GPIO_PIN_9
#define LED0_GPIO_Port GPIOB

#define PWM1_Pin GPIO_PIN_8
#define PWM1_GPIO_Port GPIOA
#define PWM2_Pin GPIO_PIN_9
#define PWM2_GPIO_Port GPIOC

#define REL_EN1_Pin GPIO_PIN_0
#define REL_EN1_GPIO_Port GPIOE
#define REL_EN2_Pin GPIO_PIN_1
#define REL_EN2_GPIO_Port GPIOE

#define ETH_nRST_Pin GPIO_PIN_0
#define ETH_nRST_GPIO_Port GPIOC

#define USART1_TX_Pin GPIO_PIN_9
#define USART1_TX_GPIO_Port GPIOA
#define USART1_RX_Pin GPIO_PIN_10
#define USART1_RX_GPIO_Port GPIOA

#define USART2_TX_Pin GPIO_PIN_5
#define USART2_TX_GPIO_Port GPIOD
#define USART2_RX_Pin GPIO_PIN_3
#define USART2_RX_GPIO_Port GPIOA

//#define USART3_DIR_Pin GPIO_PIN_15
//#define USART3_DIR_GPIO_Port GPIOB

#define USART3_TX_Pin GPIO_PIN_10
#define USART3_TX_GPIO_Port GPIOC
#define USART3_RX_Pin GPIO_PIN_11
#define USART3_RX_GPIO_Port GPIOC


//#define UART4_TX_Pin GPIO_PIN_0
//#define UART4_TX_GPIO_Port GPIOA
//#define UART4_RX_Pin GPIO_PIN_11
//#define UART4_RX_GPIO_Port GPIOC

//#define UART5_TX_Pin GPIO_PIN_12
//#define UART5_TX_GPIO_Port GPIOC
//#define UART5_RX_Pin GPIO_PIN_2
//#define UART5_RX_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */
#define NET_NRST_Pin GPIO_PIN_8
#define NET_NRST_GPIO_Port GPIOB

#define UART6_TX_Pin GPIO_PIN_6
#define UART6_TX_GPIO_Port GPIOC
#define UART6_RX_Pin GPIO_PIN_7
#define UART6_RX_GPIO_Port GPIOC

#define UART6_DIR_Pin GPIO_PIN_8
#define UART6_DIR_GPIO_Port GPIOC

/*
#define UART7_TX_Pin GPIO_PIN_8
#define UART7_TX_GPIO_Port GPIOE
#define UART7_RX_Pin GPIO_PIN_7
#define UART7_RX_GPIO_Port GPIOE

//#define UART7_DIR_Pin GPIO_PIN_9
//#define UART7_DIR_GPIO_Port GPIOE


#define UART8_TX_Pin GPIO_PIN_1
#define UART8_TX_GPIO_Port GPIOE
#define UART8_RX_Pin GPIO_PIN_0
#define UART8_RX_GPIO_Port GPIOE

//spi2
#define SPI2_CS1_Pin GPIO_PIN_12
#define SPI2_CS1_GPIO_Port GPIOB
#define SPI2_CS2_Pin GPIO_PIN_11
#define SPI2_CS2_GPIO_Port GPIOA

#define SPI2_SCK_Pin GPIO_PIN_13
#define SPI2_SCK_GPIO_Port GPIOB
#define SPI2_MISO_Pin GPIO_PIN_14
#define SPI2_MISO_GPIO_Port GPIOB
#define SPI2_MOSI_Pin GPIO_PIN_15
#define SPI2_MOSI_GPIO_Port GPIOB
*/

//BL0942
#define SPI1_CS1_Pin GPIO_PIN_6
#define SPI1_CS1_GPIO_Port GPIOD
#define SPI1_CS2_Pin GPIO_PIN_7
#define SPI1_CS2_GPIO_Port GPIOD

#define SPI1_SCK_Pin GPIO_PIN_3
#define SPI1_SCK_GPIO_Port GPIOB
#define SPI1_MISO_Pin GPIO_PIN_4
#define SPI1_MISO_GPIO_Port GPIOB
#define SPI1_MOSI_Pin GPIO_PIN_5
#define SPI1_MOSI_GPIO_Port GPIOB

//FLASH W25Q80
#define SPI3_SCK_Pin GPIO_PIN_10
#define SPI3_SCK_GPIO_Port GPIOC
#define SPI3_MISO_Pin GPIO_PIN_11
#define SPI3_MISO_GPIO_Port GPIOC
#define SPI3_MOSI_Pin GPIO_PIN_12
#define SPI3_MOSI_GPIO_Port GPIOC

#define SPI3_CS_Pin GPIO_PIN_0
#define SPI3_CS_GPIO_Port GPIOD
#define W25Q80_WP_Pin GPIO_PIN_1
#define W25Q80_WP_GPIO_Port GPIOD
#define W25Q80_HOLD_Pin GPIO_PIN_2
#define W25Q80_HOLD_GPIO_Port GPIOD

//水位监测等开关量输入
#define INPUT_Pin GPIO_PIN_15
#define INPUT_GPIO_Port GPIOD
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
