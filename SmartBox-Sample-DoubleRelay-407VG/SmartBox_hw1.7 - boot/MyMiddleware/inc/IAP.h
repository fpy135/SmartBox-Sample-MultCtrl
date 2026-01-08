#ifndef __IAP_H
#define __IAP_H


#include "main.h"

#define	USE_OFFCHIP_FLASH		1

typedef  void (*iapfun)(void);				//定义一个函数类型的参数.   


#define PROM		1	//1	-	程序运行区
#define UPDATA		2	//2	- 更新区CRC
#define BACKUP		3	//3 - 备份区
#define CODEINFO	4	//4 - 程序信息区


#define CODE_LEN_ADDR			0 	// bin 文件长度
#define CODE_MD5_ADDR			4   // bin 文件CRC 高位在前
#define UPDATA_FLAG_ADDR		20	// 需要更新标识
#define UPEDN_FLAG_ADDR			21	// 更新失败标识
#define RESETCOUNT_ADDR			22	// 复位次数存储地址 

#define RESETCOUNT		10			// 触发退回更新的复位次数


#define iFLASH_SIZE			(256*1024)	// 内部flash 大小。
#define EXFLASH_SECTORS		4096		// 外部flash 一个擦除扇区大小

extern void IAP_Init(void);
extern void IAP_Start(uint32_t AppAddr);
extern void IAP_Jump(uint32_t AppAddr);
extern void IAP_PRO(void);
extern void rollback_Program(void);
extern uint16_t CheckCodeCRC_Boot(uint8_t sw,uint32_t len);
extern void CheckCodeMD5_Boot(uint8_t sw, uint32_t len, uint8_t * cala_md5);
extern void IAP_Write_CodeInfo_Flash(uint32_t Startaddr, uint8_t *date, uint16_t len);
extern void iap_load_app(uint32_t appxaddr);			//跳转到APP程序执行
extern void CheckBackUp_Backup(void);

#endif

