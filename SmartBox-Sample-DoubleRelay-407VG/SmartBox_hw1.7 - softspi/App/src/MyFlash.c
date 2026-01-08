#include "MyFlash.h"
#include "StorageConfig.h"
#include "stdlib.h"

static uint32_t GetSector(uint32_t Address);


uint16_t MyFLASH_Unlock(void)
{
 
    HAL_FLASH_Unlock();
  	/* Clear pending flags (if any) */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                          FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
	return HAL_OK;
}

uint16_t MyFLASH_Lock(void)
{
    HAL_FLASH_Lock();
	return HAL_OK;
}
 
/*****************************
擦除扇区
*****************************/
uint16_t MyFLASH_Erase(uint32_t start_Add,uint32_t end_Add)
{
    /* USER CODE BEGIN 3 */
    uint32_t UserStartSector;
    uint32_t SectorError;
    FLASH_EraseInitTypeDef pEraseInit;
	__set_PRIMASK(1);  		/* 关中断 */
	MyFLASH_Unlock();
    /* Get the sector where start the user flash area */
    UserStartSector = GetSector(start_Add);
 
    pEraseInit.TypeErase = TYPEERASE_SECTORS;
    pEraseInit.Sector = UserStartSector;
    pEraseInit.NbSectors = GetSector(end_Add)-UserStartSector+1 ;
    pEraseInit.VoltageRange = VOLTAGE_RANGE_3;
 
    if (HAL_FLASHEx_Erase(&pEraseInit, &SectorError) != HAL_OK)
    {
        /* Error occurred while page erase */
		MyFLASH_Lock();
		__set_PRIMASK(0);  		/* 开中断 */
        return (1);
    }
	MyFLASH_Lock();
	__set_PRIMASK(0);  		/* 开中断 */
	return 0;
    /* USER CODE END 3 */
}

/*****************************
不带擦除写数据，不能跨页
*****************************/
uint8_t MyFLASH_Write(uint32_t address, uint8_t *data, uint32_t Len)
{
    /* USER CODE BEGIN 3 */
    uint32_t i = 0;
	uint32_t datatemp;
	uint8_t flashStatus = 0;
	uint32_t Address;
	uint32_t EndAddress;
	
	Address = address;
	EndAddress = Address+Len;
    for(i = 0; i < Len; i+=4)
    {
		if((EndAddress - Address) == 1)
		{
			datatemp = data[Address - address] | 0xffffff00;
		}
		else if((EndAddress - Address) == 2)
		{
			datatemp = (data[Address - address+1]<<8) | data[Address - address] | 0xffff0000;
		}
		else if((EndAddress - Address) == 3)
		{
			datatemp = (data[Address - address+2]<<16) | (data[Address - address+1]<<8) | data[Address - address] | 0xff000000;
		}
		else
		{
			datatemp = (data[Address - address+3]<<24) | (data[Address - address+2]<<16) | (data[Address - address+1]<<8) | data[Address - address];
		}
        /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
           be done by byte */
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)(address+i), datatemp) == HAL_OK)
        {
            /* Check the written value */
            if(datatemp != *(uint32_t*)(address+i))
            {
                /* Flash content doesn't match SRAM content */
				flashStatus = 2;
                break;
            }
			Address += 4;
        }
        else
        {
            /* Error occurred while writing data in Flash memory */
			flashStatus = 1;
            break;
        }
    }
	
    return (flashStatus);
    /* USER CODE END 3 */
}
uint8_t *MyFLASH_Read (uint32_t address, uint8_t *data, uint32_t Len)
{
    /* Return a valid address to avoid HardFault */
    /* USER CODE BEGIN 4 */
	uint32_t i = 0;
 
    for(i = 0; i < Len; i++)
    {
        data[i] = *(uint8_t * )address++;
    }
    return 0;

    /* USER CODE END 4 */
}



/*
*********************************************************************************************************
*	函 数 名: GetSector
*	功能说明: 根据地址计算扇区首地址
*	形    参:  无
*	返 回 值: 扇区首地址
*********************************************************************************************************
*/
static uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_SECTOR_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_SECTOR_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_SECTOR_10;  
  }
  else
  {
    sector = FLASH_SECTOR_11;  
  }
 
  return sector;
}

/*
*********************************************************************************************************
*	函 数 名: bsp_ReadCpuFlash
*	功能说明: 读取CPU Flash的内容
*	形    参:  _ucpDst : 目标缓冲区
*			 _ulFlashAddr : 起始地址
*			 _ulSize : 数据大小（单位是字节）
*	返 回 值: 0=成功，1=失败
*********************************************************************************************************
*/
uint8_t bsp_ReadCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpDst, uint32_t _ulSize)
{
	uint32_t i;

	/* 如果偏移地址超过芯片容量，则不改写输出缓冲区 */
	if (_ulFlashAddr + _ulSize > FLASH_BASE_ADDR + FLASH_SIZE)
	{
		return 1;
	}

	/* 长度为0时不继续操作,否则起始地址为奇地址会出错 */
	if (_ulSize == 0)
	{
		return 1;
	}

	for (i = 0; i < _ulSize; i++)
	{
		*_ucpDst++ = *(uint8_t *)_ulFlashAddr++;
	}

	return 0;
}

/*
*********************************************************************************************************
*	函 数 名: bsp_CmpCpuFlash
*	功能说明: 比较Flash指定地址的数据.
*	形    参: _ulFlashAddr : Flash地址
*			 _ucpBuf : 数据缓冲区
*			 _ulSize : 数据大小（单位是字节）
*	返 回 值:
*			FLASH_IS_EQU		0   Flash内容和待写入的数据相等，不需要擦除和写操作
*			FLASH_REQ_WRITE		1	Flash不需要擦除，直接写
*			FLASH_REQ_ERASE		2	Flash需要先擦除,再写
*			FLASH_PARAM_ERR		3	函数参数错误
*********************************************************************************************************
*/
uint8_t bsp_CmpCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpBuf, uint32_t _ulSize)
{
	uint32_t i;
	uint8_t ucIsEqu;	/* 相等标志 */
	uint8_t ucByte;

	/* 如果偏移地址超过芯片容量，则不改写输出缓冲区 */
	if (_ulFlashAddr + _ulSize > FLASH_BASE_ADDR + FLASH_SIZE)
	{
		return FLASH_PARAM_ERR;		/*　函数参数错误　*/
	}

	/* 长度为0时返回正确 */
	if (_ulSize == 0)
	{
		return FLASH_IS_EQU;		/* Flash内容和待写入的数据相等 */
	}

	ucIsEqu = 1;			/* 先假设所有字节和待写入的数据相等，如果遇到任何一个不相等，则设置为 0 */
	for (i = 0; i < _ulSize; i++)
	{
		ucByte = *(uint8_t *)_ulFlashAddr;

		if (ucByte != *_ucpBuf)
		{
			if (ucByte != 0xFF)
			{
				return FLASH_REQ_ERASE;		/* 需要擦除后再写 */
			}
			else
			{
				ucIsEqu = 0;	/* 不相等，需要写 */
			}
		}

		_ulFlashAddr++;
		_ucpBuf++;
	}

	if (ucIsEqu == 1)
	{
		return FLASH_IS_EQU;	/* Flash内容和待写入的数据相等，不需要擦除和写操作 */
	}
	else
	{
		return FLASH_REQ_WRITE;	/* Flash不需要擦除，直接写 */
	}
}

/*
*********************************************************************************************************
*	函 数 名: bsp_WriteCpuFlash
*	功能说明: 写数据到CPU 内部Flash。不带擦除的写
*	形    参: _ulFlashAddr : Flash地址
*			 _ucpSrc : 数据缓冲区
*			 _ulSize : 数据大小（单位是字节）
*	返 回 值: 0-成功，1-数据长度或地址溢出，2-写Flash出错(估计Flash寿命到)
*********************************************************************************************************
*/
uint8_t bsp_WriteCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpSrc, uint32_t _ulSize)
{

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

//	ucRet = bsp_CmpCpuFlash(_ulFlashAddr, _ucpSrc, _ulSize);

//	/* Flash内容和待写入的数据相等，不需要擦除和写操作 */
//	if (ucRet == FLASH_IS_EQU)
//	{
//		return 0;
//	}


	/* 需要擦除 */
//	if (ucRet == FLASH_REQ_ERASE)
//	{
//		MyFLASH_Erase(_ulFlashAddr,_ulFlashAddr+_ulSize);
//	}

	__set_PRIMASK(1);  		/* 关中断 */

	/* FLASH 解锁 */
	MyFLASH_Unlock();

	/* 按字节模式编程（为提高效率，可以按字编程，一次写入4字节） */
	MyFLASH_Write(_ulFlashAddr,_ucpSrc,_ulSize);
//	for (i = 0; i < _ulSize; i++)
//	{
//		HAL_FLASH_Program(_ulFlashAddr++, *_ucpSrc++);
//	}

  	/* Flash 加锁，禁止写Flash控制寄存器 */
	MyFLASH_Lock();

  	__set_PRIMASK(0);  		/* 开中断 */

	return 0;
}

/*************************************************
//写内部用户地址 
//在指定地址开始写入指定长度的数据
//该函数带擦除操作!
//_ucpSrc:数据存储区
//_ulFlashAddr:开始写入的地址
//_ulSize:要写入的字节数		   
*************************************************/
uint8_t Write_UserMem_Flash(uint32_t _ulFlashAddr, uint8_t *_ucpSrc, uint32_t _ulSize)
{
	uint32_t i;
	uint8_t ucRet;
	uint8_t ret = 0;
	
	unsigned char *userBuf = NULL;

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
	{
		ret = 1;
		goto malloc_failed;
	}
	//读出用户扇区使用的数据
	bsp_ReadCpuFlash(USER_DATA_ADDR,userBuf,USER_SECTOR_USE_LEN);
	for(i=0;i<_ulSize;i++)			//复制
	{
		userBuf[_ulFlashAddr-USER_DATA_ADDR+i] = _ucpSrc[i];
	}
	/* 需要擦除 */
	if (ucRet == FLASH_REQ_ERASE)
	{
		if(MyFLASH_Erase(_ulFlashAddr,_ulFlashAddr+_ulSize) != 0)
		{
			ret = 2;
			goto erase_failed;
		}
	}
	
	__set_PRIMASK(1);  		/* 关中断 */

	/* FLASH 解锁 */
	MyFLASH_Unlock();
	/* 按字节模式编程（为提高效率，可以按字编程，一次写入4字节） */
	if(MyFLASH_Write(USER_DATA_ADDR,userBuf,USER_SECTOR_USE_LEN) != 0)
	{
		ret = 3;
		goto write_failed;
	}

  	/* Flash 加锁，禁止写Flash控制寄存器 */
	MyFLASH_Lock();
  	__set_PRIMASK(0);  		/* 开中断 */
	
	return 0;
	
write_failed:
	MyFLASH_Lock();
  	__set_PRIMASK(0);  		/* 开中断 */
	
erase_failed:
	free(userBuf);
malloc_failed:
	return ret;
}
