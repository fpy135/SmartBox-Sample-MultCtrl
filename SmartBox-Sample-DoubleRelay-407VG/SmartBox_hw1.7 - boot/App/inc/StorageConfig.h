#ifndef __STORAGECONFIG_H
#define __STORAGECONFIG_H

#define CODE_SIZE 			(240*1024)  //程序存储区内的代码大小 byte
#define SECTOR_SIZE 		W25XX_SECTOR_SIZE	

/***************************************
flash 地址规划配置		512K flash

在这里要注意FLASH_ProgramWord函数的参数 ： 
Address 必须为4的整倍数，否则写入报错：
FLASH_ERROR_PROGRAM（写入错误 查了很久也没找到关于这个错误的描述，花费了不少时间）
**************************************/
															//Boot区64k
//#define CODE_INFO_ADDR			(0x0800C000)		//程序信息区	16k
#define USER_DATA_ADDR			(0x08010000)		//用户存储段地址 64k
#define APP_START_ADDR 			(0x08020000)		//APP起始地址	240k

//片外falsh	1M
#define CODE_INFO_ADDR			(0x00)		//程序信息区	4k
#define BACKUP_ADDR 			(CODE_INFO_ADDR + W25XX_SECTOR_SIZE*1)	//程序备份区	240k 60个扇区 
#define UPDATA_ADDR 			(BACKUP_ADDR+CODE_SIZE)		//程序下载区	240k

//程序信息扇区
#define CODE_INFO_LEN			(23)
#define APP_INFO_ADDR			(CODE_INFO_ADDR)
#define BACKUP_INFO_ADDR		(CODE_INFO_ADDR+1024*1)
#define UPDATA_INFO_ADDR		(CODE_INFO_ADDR+1024*2)


//用户扇区
#define USER_SECTOR_USE_LEN		(16+28+28+28+10+10+8+24+8)
#define DEVICETYPE_ADDR			(USER_DATA_ADDR)
#define DEVICEID_ADDR			(DEVICETYPE_ADDR + 4)
#define IDSTATUS_ADDR			(DEVICEID_ADDR+4)
#define LEDSTATUS_ADDR			(IDSTATUS_ADDR+4)

#define TIMECONTROL_ADDR		(LEDSTATUS_ADDR+4)		//时间策略14字节*2 
#define TIMECTRCUSTOM_ADDR		(TIMECONTROL_ADDR+28)	//用户自定义设备时间策略14字节*2

#define DeviceIP_ADDR			(TIMECTRCUSTOM_ADDR+28)
#define DeviceNetmask_ADDR		(DeviceIP_ADDR+4)
#define DeviceGw_ADDR			(DeviceNetmask_ADDR+4)
#define ServerIP_ADDR			(DeviceGw_ADDR+4)
#define ServerPort_ADDR			(ServerIP_ADDR+4)
#define DHCP_Enable_ADDR		(ServerPort_ADDR+4)
#define NTPServerIP_ADDR		(DHCP_Enable_ADDR+4)		//28字节

#define WebUsername_ADDR		(NTPServerIP_ADDR+4)		//10字节
#define WebPassword_ADDR		(WebUsername_ADDR+10)		//10字节

//#define ENERGY_ADDR				(WebPassword_ADDR + 10)		//两路电量计能4*2 = 8 字节
#define ENERGY_PARA_ADDR		(ENERGY_ADDR + 10 + 8)			//两路电量计能4*3*2 = 24 字节
#define ENERGY_REF_ADDR			(ENERGY_PARA_ADDR + 24)		//VREF+IREF  4*2 = 8 字节

#define ENERGY_ADDR				(W25XX_PAGE_SIZE*W25XX_SECTOR_SIZE - 16*W25XX_SECTOR_SIZE)		//电量存储段地址 64k

#endif
