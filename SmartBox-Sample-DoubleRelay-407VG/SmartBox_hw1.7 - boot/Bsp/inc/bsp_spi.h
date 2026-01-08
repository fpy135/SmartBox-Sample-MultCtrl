#ifndef __BSP_SPI_H
#define __BSP_SPI_H

#include "main.h"

#define SPI1_USE		0		//BL0942
#define SPI2_USE		0		//Ô¤Áô
#define SPI3_USE		1		//W25Q89

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi3;


extern void MX_SPI1_Init(void);
extern void MX_SPI3_Init(void);
extern void SPI_SetSpeed(SPI_HandleTypeDef* hspi, uint8_t SPI_BaudRatePrescaler);
extern uint8_t SPI_ReadWriteByte(SPI_HandleTypeDef* hspi, uint8_t txData);

#endif
