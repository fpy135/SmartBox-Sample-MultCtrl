#include "RemoteUpdata.h"
#include "SysConfig.h"
#include "iwdg.h"

#include "IAP.h"
#include "StorageConfig.h"
#include "MyFlash.h"
#include "usart.h"
#include "mystring.h"
#include "Platform.h"
#include "socketserver.h"
#include "IDConfig.h"
#include "md5.h"

xTaskHandle RemoteUpdata_Task_Handle;
QueueHandle_t RemoteUpdata_Queue;
SemaphoreHandle_t RemoteUpdataMutex;

static uint8_t fileMD5[16];
static uint32_t file_len = 0;
static uint32_t RemoteUpdata_tick = 0;
static uint8_t firstdata_flag = 0;

void getMD5(uint8_t *md5_32,uint8_t *md5_16)
{
	uint8_t i =0;
	for(i = 0;i<32;i=i+2)
	{
		md5_16[i/2] = md5_32[i]<<8 | md5_32[i+1]<<8;
	}
}

void RemoteUpdata_ACK(void)
{
	uint8_t tmp[10] = "receive ok";
	
	GPRS_SendData((uint8_t *)tmp,10);
}

uint8_t RemoteUpdata(uint8_t *buf, uint16_t len)
{
	uint32_t time_stamp = 0;//时间戳
	static uint8_t updata_flag = 0;
	static uint16_t buf_i = 0;
	static uint32_t buff_len = 0;
	uint16_t pp;
	uint16_t qq;
	pp = StrFindString(buf, len, (uint8_t *)"firmware updata:", 16);
	if(pp != 0xffff)
	{
		uint8_t md5temp[32];
		
		qq = StrFindString(buf, len, (uint8_t *)",", 1);
		if(Str2Num(&buf[pp+16], qq-pp-16, &file_len) == 0) return 0;
		getMD5(&buf[qq+1],fileMD5);
		myprintf("\r\n 开始固件更新 \r\n");
		UART3Write(buf,len);
		MyFLASH_Erase(UPDATA_ADDR,UPDATA_ADDR+256*1024-1);	//擦除更新区
		RemoteUpdata_ACK();		//应答  开始接受第一包
		updata_flag  = 1;
		time_stamp = HAL_GetTick();//获取当前时间戳
		return 1;
	}
	if(updata_flag == 1)
	{
		bsp_WriteCpuFlash(UPDATA_ADDR +buff_len,buf,len);
		buf_i++;
		buff_len += len;
		RemoteUpdata_ACK();
		return 1;
	}
	return 0;
//	while(  abs( time_stamp - (HAL_GetTick()) ) <= timeout  )//1000ms未收到任何数据，退出更新
//	{
//		HAL_IWDG_Refresh(&hiwdg);
//	}
//	while(1)
//	{
//		
//	}
}

/******************基于以太网的远程升级**********************/
uint32_t File_Size = 0;
uint16_t file_pack_num = 0;
uint16_t packetnum = 0;
uint8_t file_type = 0;
static uint16_t next_pack = 0;

void Get_File_inform(uint8_t *buf)
{
	uint8_t i=0;
	
	file_type = buf[10];
	File_Size = buf[11]<<24 | buf[12]<<16 | buf[13]<<8 | buf[14];
	file_pack_num = File_Size/(PLATFORM_BUFF_SIZE-4);
	if(File_Size%(PLATFORM_BUFF_SIZE-4))
		file_pack_num++;
	StrCopy(fileMD5,&buf[15],16);
	if(file_type == 0x00)
		myprintf("\r\n 开始接收文件，文件类型：远程升级文件");
	else if(file_type == 0x01)
		myprintf("\r\n 开始接收文件，文件类型：MP3文件");
	myprintf("\r\n 文件长度：%d,包数：%d",File_Size,file_pack_num);
	myprintf("\r\n 文件MD5：");
	for(i=0;i<16;i++)
	{
		myprintf("%02x",fileMD5[i]);
	}
}


uint8_t RemoteUpDate(uint8_t *buff)
{
	uint16_t crc_tmp1,crc_tmp2;

	packetnum = buff[0]<<8 | buff[1];
	crc_tmp2 = CRC16_calc(buff,PLATFORM_BUFF_SIZE-2);
	crc_tmp1 = buff[PLATFORM_BUFF_SIZE-1]<<8 | buff[PLATFORM_BUFF_SIZE-2];
	if(packetnum == (file_pack_num-1))	//尾包crc校验
	{
		uint16_t buff_len = 0;
		
		buff_len = File_Size%(PLATFORM_BUFF_SIZE-4)+4;
		crc_tmp2 = CRC16_calc(buff,buff_len-2);
		crc_tmp1 = buff[buff_len-1]<<8 | buff[buff_len-2];
	}
	if(crc_tmp2 == crc_tmp1)
	{
		RemoteUpdata_tick = HAL_GetTick();
		if(packetnum == 0)		//首包
		{
			myprintf("\r\n 第一包");
			next_pack = 0;
			MyFLASH_Erase(UPDATA_ADDR,UPDATA_ADDR+256*1024-1);	//擦除更新区
			firstdata_flag = 1;	//首包通过开始远程更新更新
		}
		myprintf("%d ",packetnum);
		if(firstdata_flag)
		{
			if(next_pack == packetnum-1 || packetnum == 0)
			{
				next_pack = packetnum;
				if(packetnum < (file_pack_num-1))
					bsp_WriteCpuFlash(UPDATA_ADDR + packetnum*(PLATFORM_BUFF_SIZE-4),&buff[2],PLATFORM_BUFF_SIZE-4);
				else
					bsp_WriteCpuFlash(UPDATA_ADDR + packetnum*(PLATFORM_BUFF_SIZE-4),&buff[2],File_Size%(PLATFORM_BUFF_SIZE-4));//尾包
				RemoteUpdata_ack(Sn,Device_TYPE,Device_ID,0xc1,2,&buff[0]);	//回复接收成功
			}
		}
	}
	else
	{
		buff[0] |= 0x80;
		myprintf("重发:%d ",packetnum);
		RemoteUpdata_ack(Sn,Device_TYPE,Device_ID,0xc1,2,&buff[0]);	//需要重发
	}
	if(packetnum == (file_pack_num-1))		//接收完成所有数据包
	{
		uint8_t i=0;
		MD5_CTX md5;
		uint8_t calculate_md5[16] = {0};
		uint8_t temp_buff[CODE_INFO_LEN];
		
		firstdata_flag = 0;	//更新标志结束
		MD5Init(&md5);	
		MD5Update(&md5,(uint8_t *)UPDATA_ADDR,File_Size);
		MD5Final(&md5,calculate_md5);
		myprintf("\r\n 计算更新区内的程序MD5：");
		for(i=0;i<16;i++)
		{
			myprintf("%02x",calculate_md5[i]);
		}
		if(StrComplate(calculate_md5,fileMD5,16)==1)	//md5校验通过
		{
			myprintf("\r\n MD5校验通过");
			temp_buff[CODE_LEN_ADDR] =   (File_Size>>24)&0xff;
			temp_buff[CODE_LEN_ADDR+1] = (File_Size>>16)&0xff;
			temp_buff[CODE_LEN_ADDR+2] = (File_Size>>8)&0xff;
			temp_buff[CODE_LEN_ADDR+3] = (uint8_t)File_Size;
			StrCopy(&temp_buff[CODE_MD5_ADDR],calculate_md5,16);
			temp_buff[UPDATA_FLAG_ADDR] = 1;	//需要更新
			IAP_Write_CodeInfo_Flash(UPDATA_INFO_ADDR,temp_buff,CODE_INFO_LEN); // 写入信息
			
			next_pack = 0;
			firstdata_flag = 0;
			xSemaphoreTake (RemoteUpdataMutex, 0);
//			NVIC_SystemReset();//重启
			vTaskDelete(RemoteUpdata_Task_Handle);
			
		}
	}
}

uint8_t RemoteUpdata_buf[PLATFORM_BUFF_SIZE];

void RemoteUpdata_Task(void *argument)
{
	_myprintf("\r\nStart RemoteUpdata_Task");
//	uint8_t *RemoteUpdata_buf;
	
	RemoteUpdata_tick = HAL_GetTick();
	//申请内存
//	RemoteUpdata_buf = (uint8_t *)malloc(PLATFORM_BUFF_SIZE * sizeof(uint8_t));

	while(1)
	{
//		HAL_IWDG_Refresh(&hiwdg);
		if(xQueuePeek(RemoteUpdata_Queue,RemoteUpdata_buf, ( TickType_t )0) == pdPASS)	//接收队列消息
        {
			RemoteUpDate(RemoteUpdata_buf);
            xQueueReceive(RemoteUpdata_Queue, RemoteUpdata_buf, ( TickType_t )0);
        }
		if((HAL_GetTick() - RemoteUpdata_tick)>=RemoteUpdataTimeOut)	//超时机制
		{
			next_pack = 0;
			firstdata_flag = 0;
//			free(RemoteUpdata_buf);		//释放内存
			xSemaphoreTake (RemoteUpdataMutex, 0);
			vTaskDelete(RemoteUpdata_Task_Handle);
		}
		vTaskDelay(100);
	}
}

void CreatRemoteUpdataTask(void)
{
	BaseType_t RemoteUpdata_TaskID;
	
	_myprintf("\r\n CreatRemoteUpdataTask");
	RemoteUpdata_TaskID = xTaskCreate (RemoteUpdata_Task, "RemoteUpdata", RemoteUpdata_StkSIZE, NULL, RemoteUpdata_TaskPrio, &RemoteUpdata_Task_Handle);
	if(pdPASS != RemoteUpdata_TaskID)
	{
        _myprintf("\r\n CreatRemoteUpdataTask creat error");
//        while(1);
    }
}
