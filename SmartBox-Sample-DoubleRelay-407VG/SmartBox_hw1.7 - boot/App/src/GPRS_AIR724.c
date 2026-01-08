#include "GPRS_AIR724.h"
#include "string.h"
#include "SysConfig.h"
#include "Usart.h"
#include "mystring.h"

BaseType_t GPRS_TaskID;
xTaskHandle pvCreatedTask_GPRS;

QueueHandle_t GPRS_CMD_Queue;
QueueHandle_t GPRS_RECV_Queue;

SemaphoreHandle_t GPRS_CMD_Semaphore;


uint8_t	GPRSSignal = 0;  //GPRS信号强度
const uint8_t memzero[30] = {0};
uint8_t 	IMEI[20] = {0}; 
uint8_t 	IMSI[20] = {0};
uint8_t 	ICCID[21] = {0};

/*=====================================================
 * 函数功能: 	GPRS接收数据队列发送 中断调用
 * 输入参数:	长度 + 内容
 * 输出参数: 
 * 返    回:	1 失败, 0 成功
=====================================================*/
uint8_t MSG_GPRSReceiveDataFromISR(uint8_t *buf, uint16_t len)
{
//	uint8_t *Data = NULL;
	//申请缓存
	uint8_t Data[len+2];

//	Data = mymallocFromISR(len+2);
//	if(Data == NULL)
//	{
//		_myprintf("\r\nmymalloc error, MSG_GPRSReceiveDataFromISR");
//		return 1;
//	}
	
	//复制消息
	Data[0] = len;
	Data[1] = len>>8;
	memcpy((void *)&Data[2],(const void *)buf,len);
	//发送消息

	if(xQueueSendFromISR(GPRS_RECV_Queue, (void *)&Data, NULL) != pdPASS)
	{
//		myfreeFromISR(Data);
		_myprintf("\r\nxQueueSend error, GPRS_RECEIVE_Queue");
		return 1;
	}

	return 0;
}

/*=====================================================
 * 函数功能: 	GPRS发送命令
 * 输入参数:	const char *cmd, uint16_t len, char *str, uint16_t Wtime
 * 输出参数: 
 * 返    回:	1 失败, 0 成功
=====================================================*/
uint8_t GPRSSendCMD(const char *cmd, uint16_t len, char *str, uint16_t Wtime)
{
	uint8_t *p;
	BaseType_t res;
	xSemaphoreTake(GPRS_CMD_Semaphore, ( TickType_t ) 0);	//获取信号量
	xQueueReceive( GPRS_CMD_Queue, (void *)&p, ( TickType_t ) 0);	//获取消息队列 清空队列
//	myprintf("\r\nGPRSSendCMD\r\n");
//	PrintWrite((uint8_t *)cmd,len);
	GPRSSend((uint8_t *)cmd,len);
	if(xQueueSend(GPRS_CMD_Queue, (void *)&str, 0) != pdPASS){		//发送需要检索的内容消息队列
		myprintf("\r\nxQueueSend error, GPRS_CMD_Queue");
		return 1;
	}
	res = xSemaphoreTake(GPRS_CMD_Semaphore, 100 * Wtime);
	if(res == pdTRUE){	//等待检索结果
		myprintf("\r\nGPRS_CMD_Semaphore OK, %X", res);
		return 0;
	}
	else
	{
		myprintf("\r\nGPRS_CMD_Semaphore NOK, %X", res);
		return 1;
	}
}



/*=====================================================
 * 函数功能: 	GPRS命令处理
 * 输入参数:	
 * 输出参数: 	
 * 返    回:	0 成功, 1 模块没反应, 2 初始化不通过
=====================================================*/
void GPRSDataPro(void)
{
	uint8_t *msgdata = NULL;
	uint8_t *cmddata = NULL;
	uint16_t GPRSRecviveLen = 0;
	uint8_t *GPRSRecviveBuf = NULL;
	uint16_t p;
	if(xQueueReceive(GPRS_RECV_Queue, (void *)&msgdata, ( TickType_t )100) != pdPASS)
	{
		return;
	}
	if(msgdata != NULL)
	{
		GPRSRecviveLen = msgdata[0] | (msgdata[1]<<8);
		GPRSRecviveBuf = msgdata+2;	
	}
	if(xQueueReceive( GPRS_CMD_Queue, (void *)&cmddata, ( TickType_t ) 0) == pdPASS)	//获取消息队列
	{
		p = StrFindString(GPRSRecviveBuf, GPRSRecviveLen, cmddata, strlen((const char *)cmddata));		
		if(p != 0xffff)
		{
			#if GPRSPrint
				myprintf("\r\nAT接收, len: %d",GPRSRecviveLen);
				PrintWrite(GPRSRecviveBuf, GPRSRecviveLen);
			#endif
			if(GPRSRecviveLen > GPRSCount)
			{
				memcpy((void *)GPRSBuf, (const void *)GPRSRecviveBuf, GPRSCount); 
			}
			else
			{
				memcpy((void *)GPRSBuf, (const void *)GPRSRecviveBuf, GPRSRecviveLen); 
			}
			xSemaphoreGive(GPRS_CMD_Semaphore);					
		}
		else
		{
			xQueueSend(GPRS_CMD_Queue, (void *)&cmddata, 0);		//当前数据包无需要内容，重新发送需要检索的内容消息队列
		}
	}
}

uint8_t GetSignal(uint8_t *data)
{
	uint8_t signal = 0;
	uint16_t p;
	p = GetNStr(GPRSBuf, GPRSCount, ':', 1);
	if(p != 0xffff){
		if(Str2Num(&GPRSBuf[p+2], 2, (uint32_t *)&signal) != 0){
			if(signal > 31){
				return 0;
			}
			#if GPRSPrint
				myprintf("\r\n信号质量: %d",signal);
			#endif
			return signal;
		}
	}
	return 0;
}

/*=====================================================
 * 函数功能: 	获取串号
 * 输入参数:	char *buf
 * 输出参数: 	char *str
 * 返    回:	
=====================================================*/
void GetSerialNumber(char *buf, char *str)
{
	uint16_t i,j;
	uint8_t tmp[20];

	memset(tmp,0,20);

	for(i=0;i<20;i++)
	{
	 	if((buf[i]>=0x30) && (buf[i]<=0x39))//是数字，开始取串号
		{  
			//字符转数字
			for(j=i;j<(20+i);j++)
			{
				if(CharToHex(buf[j])!= 0xff)
				{
					tmp[j-i] = buf[j];
				}
				else
				{
					break;
				}
				
			}
			memcpy((void *)str,(const void *)tmp,20);
			break;
		}
	}
}

/*=====================================================
 * 函数功能: 	检测GSM网络注册
 * 输入参数:	
 * 输出参数: 	
 * 返    回:	
=====================================================*/
uint8_t CheckCREG(uint8_t *buf, uint16_t len)
{
	uint16_t p;
	
	p = StrFindString(buf, len, (uint8_t *)"+CGREG", 5);
	if(p != 0xffff)
	{
		if((buf[p+9] == '1')|| (buf[p+9] == '5'))
		{
			return 0;
		}
	}
	return 1;
	
}
/*=====================================================
 * 函数功能: 	检测GPRS网络注册
 * 输入参数:	
 * 输出参数: 	
 * 返    回:	
=====================================================*/
uint8_t CheckCGREG(uint8_t *buf, uint16_t len)
{
	uint16_t p;
	
	p = StrFindString(buf, len, (uint8_t *)"+CGREG", 5);
	if(p != 0xffff)
	{
		if((buf[p+10] == '1')|| (buf[p+10] == '5'))
		{
			return 0;
		}
	}
	return 1;
	
}


/*=====================================================
 * 函数功能: 	GPRS air724 配置
 * 输入参数:	
 * 输出参数: 	
 * 返    回:	0 成功, 1 模块没反应, 2 初始化不通过
=====================================================*/
uint8_t GPRS_Config(void)
{
	
	uint8_t ReConfigCnt = 0;
	uint8_t GPRSErr = 0;	   // 0 成功, 1 模块没反应, 2 初始化不通过
	
	while(1)
	{
		ReConfigCnt ++;
		if(ReConfigCnt > ReConfigTime){
			return GPRSErr;	
		}
		vTaskDelay(1000);
//关回显
		#if GPRSPrint
			myprintf("\r\n发送: AT");
		#endif
		if(GPRSSendCMD("AT\r\n", 4, "OK", 10)){
			#if GPRSPrint
				myprintf("\r\nAT continue. ReConfigCnt: %d", ReConfigCnt);
			#endif
			GPRSErr = 1;
		 	continue;
		}
		
		GPRSErr = 2;
		vTaskDelay(1000);
		GPRSSendCMD("AT+CGMR\r\n", 9, "OK", 20);	//查看模块版本
//打开错误码
		#if GPRSPrint
			myprintf("\r\n发送: AT+CMEE=1");
		#endif
		GPRSSendCMD("AT+CMEE=1\r\n", 11, "OK", 10);

//CSQ信号
		#if GPRSPrint
			myprintf("\r\n发送: CSQ");
		#endif
		if(GPRSSendCMD("AT+CSQ\r\n", 8, "+CSQ", 10)){
			#if GPRSPrint
				myprintf("\r\nAT+CSQ continue. ReConfigCnt: %d", ReConfigCnt);
			#endif
			continue;
		}
		GPRSSignal = GetSignal(GPRSBuf);
		if(GPRSSignal < SignalLowValue){	//GPRS信号不好，不去连接网络
			#if GPRSPrint
				myprintf("\r\nGPRS信号不好，不去连接网络. ReConfigCnt: %d", ReConfigCnt);
			#endif
			continue;
		}
		vTaskDelay(1000);
//请求IMEI
		if(memcmp((const void *)IMEI, (const void *)memzero, sizeof(IMEI)) == 0){
			#if GPRSPrint
				myprintf("\r\n发送: AT+CGSN");
			#endif
			if(GPRSSendCMD("AT+CGSN\r\n", 9, "OK", 10)){
				#if GPRSPrint
					myprintf("\r\nAT+CGSN. ReConfigCnt: %d", ReConfigCnt);
				#endif
				continue;	
			}
			GetSerialNumber((char *)GPRSBuf, (char *)IMEI);
			#if GPRSPrint
				myprintf("\r\nIMEI %s",IMEI);
			#endif
		}
		vTaskDelay(1000);
//请求IMSI
		if(memcmp((const void *)IMSI, (const void *)memzero, sizeof(IMSI)) == 0){
			#if GPRSPrint
				myprintf("\r\n发送: AT+CIMI");
			#endif
			if(GPRSSendCMD("AT+CIMI\r\n", 9, "OK", 10))
			{
				#if GPRSPrint
					myprintf("\r\nAT+CIMI. ReConfigCnt: %d", ReConfigCnt);
				#endif
				continue;
			} 
			GetSerialNumber((char *)GPRSBuf, (char *)IMSI);
			#if GPRSPrint
				myprintf("\r\nIMSI %s",IMSI);
			#endif		
		}
		vTaskDelay(1000);
		
//请求ICCID
		if(memcmp((const void *)ICCID, (const void *)memzero, sizeof(ICCID)) == 0){
			#if GPRSPrint
				myprintf("\r\n发送: AT+ICCID");
			#endif
			if(GPRSSendCMD("AT+CCID\r\n", 9, "OK", 10)){
				#if GPRSPrint
					myprintf("\r\nAT+CCID. ReConfigCnt: %d", ReConfigCnt);
				#endif
				continue;
			}
			GetSerialNumber((char *)GPRSBuf, (char *)ICCID);
			#if GPRSPrint
				myprintf("\r\nICCID %s", ICCID);
			#endif
		}
		vTaskDelay(1000);
//GPRS 注册
		#if GPRSPrint
			myprintf("\r\n发送: AT+CGREG?");
		#endif
		if(GPRSSendCMD("AT+CGREG?\r\n", 11, "OK", 10))
		{
			#if GPRSPrint
				myprintf("\r\nAT+CGREG continue. ReConfigCnt: %d", ReConfigCnt);
			#endif
			continue;
		}
		if(CheckCGREG(GPRSBuf,GPRSCount)){	
			#if GPRSPrint
				myprintf("\r\nAT+CGREG bak not 1 or 5. ReConfigCnt: %d", ReConfigCnt);
			#endif				
			vTaskDelay(3000);
			continue;
		}

//GPRS附着
		#if GPRSPrint
			myprintf("\r\n发送: AT+CGATT?");
		#endif
		if(GPRSSendCMD("AT+CGATT?\r\n", 11, "+CGATT: 1", 50))
		{
			#if GPRSPrint
				myprintf("\r\nAT+CGATT continue. ReConfigCnt: %d", ReConfigCnt);
			#endif
			continue;
		}
		
//设置多链接
		#if GPRSPrint
			myprintf("\r\n发送: AT+CIPMUX=1");
		#endif
		if(GPRSSendCMD("AT+CIPMUX=1\r\n", 13, "OK", 10))
		{
			#if GPRSPrint
				myprintf("\r\nAT+CIPMUX=1 continue. ReConfigCnt: %d", ReConfigCnt);
			#endif
			continue;
		}	
		
//设置接入点APN
		#if GPRSPrint
			myprintf("\r\n发送: AT+CSTT=\"CMNET\" ");
		#endif
		if(GPRSSendCMD("AT+CSTT=\"CMNET\"\r\n", 17, "OK", 50))
		{
			#if GPRSPrint
				myprintf("\r\nAT+CSTT. ReConfigCnt: %d", ReConfigCnt);
			#endif
			continue;
		}	
		
//激活场景,获取IP
		#if GPRSPrint
			myprintf("\r\n发送: AT+CIICR");
		#endif
		if(GPRSSendCMD("AT+CIICR\r\n", 10, "OK", 30))
		{
			#if GPRSPrint
				myprintf("\r\nAT+CIICR. ReConfigCnt: %d", ReConfigCnt);
			#endif
			continue;
		}
		
//查看获取的IP
	#if GPRSPrint
		myprintf("\r\n发送: AT+CIFSR");
	#endif
	if(GPRSSendCMD("AT+CIFSR\r\n", 10, ".", 10))
	{
		#if GPRSPrint
			myprintf("\r\nAT+CIFSR. ReConfigCnt: %d", ReConfigCnt);
		#endif
		continue;
	}
#if GPRS_TTS_EN
		GPRSSendCMD("AT+CTTSPARAM=100,0,50,60,0\r\n", 28, "OK", 20);	//配置TTS 音量100模式0音高50速度50通道0
		if(ELE_Flag == 1){
			vTaskDelay(1000);
			ELE_Flag = 0;
			GPRSSendCMD("AT+CTTS=2,\"欢迎使用\"\r\n", 22, "OK", 20);
		}
#endif
		
//			GPRSSendCMD("AT+CIPSTATUS\r\n", 14, "OK", 100);
//显示IP头
		#if GPRSPrint
			myprintf("\r\n发送: AT+CIPHEAD=1");
		#endif
		if(GPRSSendCMD("AT+CIPHEAD=1\r\n", 14, "OK", 10))
		{
			#if GPRSPrint
				myprintf("\r\nAT+CIPHEAD. ReConfigCnt: %d", ReConfigCnt);
			#endif
			continue;
		}
//		GetLocationFromLBS();
		#if GPRSPrint
			myprintf("\r\nGPRS初始化完成");
		#endif
		return 0;
	}
}


void GPRS_Task(void * argument)
{
	GPRS_Config();
	while(1)
	{
//		GPRSDataPro();	//处理GPRS数据 等待消息队列阻塞100ms
		vTaskDelay(100);
	}
}

void CreatGRRSTask(void)
{
	_myprintf("\r\n CreatGRRSTask");
    GPRS_CMD_Queue = xQueueCreate( GPRS_QUEUE_LENGTH, 128 );
	if(NULL == GPRS_CMD_Queue)
	{
		_myprintf("\r\n GPRS_CMD_Queue error\r\n");
		while(1);
	}
    GPRS_RECV_Queue = xQueueCreate( GPRS_QUEUE_LENGTH, 128 );
	if(NULL == GPRS_RECV_Queue)
	{
		_myprintf("\r\n GPRS_RECV_Queue error\r\n");
		while(1);
	}
	GPRS_CMD_Semaphore = xSemaphoreCreateBinary();
	if(NULL == GPRS_CMD_Semaphore)
	{
		_myprintf("\r\n GPRS_CMD_Semaphore error\r\n");
		while(1);
	}
	GPRS_TaskID = xTaskCreate(GPRS_Task, "GPRS", GPRS_StkSIZE, NULL, GPRS_TaskPrio, &pvCreatedTask_GPRS);
    if(pdPASS != GPRS_TaskID)
    {
        _myprintf("\r\n GPRS_Task creat error");
        while(1);
    }
}
