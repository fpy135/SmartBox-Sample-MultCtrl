#include "BL0942.h"
#include "SysConfig.h"
#include "usart.h"
#include "bsp_spi.h"
#include "MyFlash.h"
#include "StorageConfig.h"
#include "mystring.h"

CollectionData ElectricData[ELE_CIRCUIT_NUM];
uint32_t SaveEnergy[ELE_CIRCUIT_NUM];					//保存在flash中的已经产生的能量
Elect_StructDef BL0942_Elect[ELE_CIRCUIT_NUM];

FourBytes_Type BL0942_D;
AllFlame_TypeDef BL0942_AllFlame;

QueueHandle_t BL0942_RECEIVE_Queue;

extern void CheckSelf_Flag_Rw(uint8_t rw, uint8_t * checkflag);

void (*BL6523Send) (uint8_t *, uint16_t);

void delay5us(uint16_t time)
{
   uint16_t i=0;  
   while(time--)
   {
		i = 77;//77
		while (--i);    
   }
}

uint8_t BL0942_Check(uint8_t * check_data,uint8_t len)
{
    uint8_t res = 0;
    uint8_t i = 0;

    for(i = 0; i<len; i++)
    {
        res += check_data[i];
    }

    res &= 0xff;
    res = ~res;
    return res;
}

#if DRIVER_USE_SPI
/*******************************************************************************
** 函数名称: SPI_Configuration
** 功能描述：
** 函数说明：SPI初始化
** 作　  者: EE.Fan
** 日　  期: 
*******************************************************************************/
void SPI_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	/*Configure GPIO pin : SPI1_CS_Pin */
    GPIO_InitStruct.Pin = SPI1_CS1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SPI1_CS1_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = SPI1_CS2_Pin;
    HAL_GPIO_Init(SPI1_CS2_GPIO_Port, &GPIO_InitStruct);
	
	 /*Configure GPIO pin : PtPin */
	GPIO_InitStruct.Pin = SPI1_MISO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(SPI1_MISO_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : PtPin */
	GPIO_InitStruct.Pin = SPI1_MOSI_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(SPI1_MOSI_GPIO_Port, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = SPI1_SCK_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(SPI1_SCK_GPIO_Port, &GPIO_InitStruct);
	
    BL0942_1_CS(1);
    BL0942_2_CS(1);
	SPI_SCLK(0);
	SPI_MOSI(0);
	
	#if DRIVER_USE_HARD_SPI
		MX_SPI1_Init();
//		SPI_SetSpeed(&hspi1,SPI_BAUDRATEPRESCALER_256); //设置时钟,高速模式
//		SPI_ReadWriteByte(&hspi1,0Xff);                           //启动传输
		__HAL_SPI_ENABLE(&hspi1);                    //使能SPI1
	
		SPI_ReadWriteByte(&hspi1,0Xff);                           //启动传输
	#else
	#endif
	
}

#if DRIVER_USE_HARD_SPI
void SPI_SendByte(uint8_t byte)
{
	SPI_ReadWriteByte(&hspi1, byte);
}

uint8_t SPI_ReceiveByte(void)
{
	return SPI_ReadWriteByte(&hspi1, 0x00);
}

#else
/*******************************************************************************
** 函数名称: SPI_SendByte
** 功能描述：
** 函数说明：发送一个字节到BL6526B
** 作　  者: EE.Fan
** 日　  期:
*******************************************************************************/
void SPI_SendByte(uint8_t byte)
{
    uint8_t i;

//	SPI_SCLK(0);
    for(i=0; i<8; i++)
    {
		SPI_SCLK(1);
		delay5us(1);
		if(byte&0x80)
			SPI_MOSI(1);
		else
			SPI_MOSI(0);
		byte<<=1;
        SPI_SCLK(0); //时钟拉高 产生上升沿 一个bit位就发走了
		delay5us(1);
    }

}

/*******************************************************************************
** 函数名称: SPI3_ReceiveByte
** 功能描述：
** 函数说明：接收到一个字节
** 作　  者: EE.Fan
** 日　  期:
*******************************************************************************/
uint8_t SPI_ReceiveByte(void)
{
    uint8_t i,byte = 0;
//    SPI_SCLK(0);
    for(i=0; i<8; i++)
    {
        byte<<=1;
        SPI_SCLK(1);
		delay5us(1);
        if(SPI_MISO_IN)
            byte |= 0x01;
        else
            byte &= ~0x01;
        SPI_SCLK(0);
		delay5us(1);
    }
    return byte;
}
#endif

/*******************************************************************************
** 函数名称: BL0942_ReadData
** 功能描述：往里面传一个CMD = 0X00 + Address
** 函数说明：BL0942_ReadData从BL0942B里面读数据出来 24bit数据
** 作　  者: EE.Fan
** 日　  期:
*******************************************************************************/
uint32_t BL0942_ReadData(uint8_t reg_addr)
{
    uint8_t *tmp;
    uint32_t tmpdata = 0;
    uint8_t sum = 0;
    uint8_t i = 0;

    tmp = pvPortMalloc(130);
    tmp[0] = BL0942_Addr_R;
    tmp[1] = reg_addr;
    //先往地址里面写一个字节 告诉BL0942的哪个寄存器
    SPI_SendByte(BL0942_Addr_R);
    SPI_SendByte(reg_addr);
    for(i = 0; i < 4; i++)
        tmp[2+i] = SPI_ReceiveByte();
    sum = BL0942_Check(tmp,5);
    if(sum == tmp[5])
    {
        tmpdata |= (tmp[2]<<16);
        tmpdata |= (tmp[3]<<8);
        tmpdata |= tmp[4];
    }
    vPortFree(tmp);
    return tmpdata;
}

/*******************************************************************************
** 函数名称: BL0942_WriteData
** 功能描述：往里面传一个CMD = 0X00 + Address
** 函数说明：BL0942_WriteData
** 作　  者: EE.Fan
** 日　  期:
*******************************************************************************/
void BL0942_WriteData(uint8_t reg_addr,uint8_t *byte)
{
    uint8_t tmp[6];
    uint8_t i = 0;

    //先往地址里面写一个字节 告诉BL0942的哪个寄存器
    tmp[0] = BL0942_Addr_w;
    tmp[1] = reg_addr;
    tmp[2] = byte[0];
    tmp[3] = byte[1];
    tmp[4] = byte[2];
    tmp[5] = BL0942_Check(tmp,5);
    for(i = 0; i < 6; i++)
        SPI_SendByte(tmp[i]);
}

uint32_t BL0942_ReadAllData(uint8_t reg_addr)
{
    uint8_t *tmp;
    uint32_t ret = 0;
    uint8_t i = 0;
    uint8_t sum = 0;

    tmp = pvPortMalloc(130);
    tmp[0] = BL0942_Addr_R;
    tmp[1] = reg_addr;
    //先往地址里面写一个字节 告诉BL0942的哪个寄存器
    SPI_SendByte(BL0942_Addr_R);
    SPI_SendByte(reg_addr);
//	tmp[1] = 0x55;
    for(i = 0; i < 23; i++)
        tmp[1+i] = SPI_ReceiveByte();
    sum = BL0942_Check(tmp,23);
    if(sum == tmp[23])
    {
        StrCopy(BL0942_AllFlame.B8,&tmp[1],23);
        ret = 1;
    }
    vPortFree(tmp);
    return ret;
}

#else
/*******************************************************************************
** 函数名称: BL0942_ReadData
** 功能描述：往里面传一个CMD = 0X00 + Address
** 函数说明：BL0942_ReadData从BL0942B里面读数据出来 24bit数据
** 作　  者: EE.Fan
** 日　  期:
*******************************************************************************/
uint32_t BL0942_ReadData(uint8_t reg_addr)
{
    uint8_t *tmp;
    uint32_t tmpdata = 0;
    uint8_t sum = 0;

    tmp = pvPortMalloc(130);
    tmp[0] = BL0942_Addr_R;
    tmp[1] = reg_addr;
    //先往地址里面写一个字节 告诉BL0942的哪个寄存器
    BL6523Send(tmp,2);
    if(xQueueReceive(BL0942_RECEIVE_Queue, (void *)&tmp[2], ( TickType_t )300) != pdPASS)
    {
        goto fail_ret;
    }
    sum = BL0942_Check(tmp,5);
    if(sum == tmp[5])
    {
        tmpdata |= (tmp[4]<<16);
        tmpdata |= (tmp[3]<<8);
        tmpdata |= tmp[2];
    }
fail_ret:
    vPortFree(tmp);
    return tmpdata;
}

uint32_t BL0942_ReadAllData(uint8_t reg_addr)
{
    uint8_t *tmp;
    uint32_t ret = 0;
    uint8_t i = 0;
    uint8_t sum = 0;

    tmp = pvPortMalloc(130);
    tmp[0] = BL0942_Addr_R;
    tmp[1] = reg_addr;
    //先往地址里面写一个字节 告诉BL0942的哪个寄存器
    BL6523Send(tmp,2);
//	tmp[1] = 0x55;
    if(xQueueReceive(BL0942_RECEIVE_Queue, (void *)&tmp[1], ( TickType_t )300) != pdPASS)
    {
        ret = 0;
        goto fail_ret;
    }
    sum = BL0942_Check(tmp,23);
    if(sum == tmp[23])
    {
        StrCopy(BL0942_AllFlame.B8,&tmp[1],23);
        ret = 1;
    }
fail_ret:
    vPortFree(tmp);
    return ret;
}

/*******************************************************************************
** 函数名称: BL0942_WriteData
** 功能描述：往里面传一个CMD = 0X00 + Address
** 函数说明：BL0942_WriteData
** 作　  者: EE.Fan
** 日　  期:
*******************************************************************************/
void BL0942_WriteData(uint8_t reg_addr,uint8_t *byte)
{
    uint8_t tmp[6];

    //先往地址里面写一个字节 告诉BL0942的哪个寄存器
    tmp[0] = BL0942_Addr_w;
    tmp[1] = reg_addr;
    tmp[2] = byte[2];
    tmp[3] = byte[1];
    tmp[4] = byte[0];
    tmp[5] = BL0942_Check(tmp,5);
    BL6523Send(tmp,6);
}

#endif

void Delay_MS(uint16_t Count)
{
    uint16_t i,j;
    for(i=0; i<Count; i++)
    {
        for(j=0; j<540; j++)	//10800,10mS,内部16MHz时钟时;20mS at 8MHz Clock  ,540=1mS at 8MHz
        {
            __nop();
        }
    }
}

void Delay_100uS(uint16_t Count)
{
    uint16_t i,j;
    for(i=0; i<Count; i++)
    {
        for(j=0; j<54; j++)	// at 8MHz Clock
        {
            __nop();
        }
    }
}


/**********************************************
	*Function:			Uart_Init
	*Description: 	Uart接口的初始化；4800，N，8,1
	*Parameter: 		在Uart中断中进行数据字节接收，
	*Return:
	*Author:				Eric.Huang
	*Data:					2018.09.10
***********************************************/
void BL0942_Init(void)
{
#if DRIVER_USE_SPI
	SPI_Configuration();
#endif
	for(uint8_t i=0; i<ELE_CIRCUIT_NUM; i++)
	{
		BL0942_ConfigInit(i+1);
	}
}


/**********************************************
*Function:			BL0942_Uart_ReadEData
*Description:		BL0942读电参数函数
*Parameter:			指定寄存器地址读取方式
*Return:				转换为实际电参数数据，放到对应发送数组中
*Author:				Eric.Huang
*Data:					2020.04.19
*Note:				  如果使用BL0942多机通信时，需要注意切换器件地址进行通讯时延时时间要20mS以上
***********************************************/
uint8_t BL0942_ReadEData(uint8_t channle, CollectionData *electricdata)
{
    bool sign_WATT=(bool)0;					//有功功率的符号位，0表示正功，1表示负功
    FourBytes_Type	tmp_D;
	Elect_StructDef *ele_tmp;
    if(channle == 1)
    {
		ele_tmp = &BL0942_Elect[0];
#if DRIVER_USE_SPI
        BL0942_1_CS(0);
#else
        BL6523Send = UART1Write;
#endif
    }
#if ELE_CIRCUIT_NUM >= 2
    else if(channle == 2)
    {
		ele_tmp = &BL0942_Elect[1];
	#if DRIVER_USE_SPI
        BL0942_2_CS(0);
	#else
        BL6523Send = UART6Write;
	#endif
    }
#endif
	
    if(BL0942_ReadData(Addr_WA_CREEP) != 0x0000A0)	/* 增益寄存器*/
    {
        return 1;
    }
	
    tmp_D.uLongs = BL0942_ReadData(Addr_I_RMS);	//IA_RMS
    tmp_D.uLongs = tmp_D.uLongs*1000 / Current_K;
	ele_tmp->Current_Value = tmp_D.uLongs;

    tmp_D.uLongs = BL0942_ReadData(Addr_I_FAST_RMS);	//I_FAST_RMS
    tmp_D.uLongs = tmp_D.uLongs*1000 / Current_K;
	ele_tmp->F_Current_Value = tmp_D.uLongs;

    tmp_D.uLongs = BL0942_ReadData(Addr_V_RMS);	//V_RMS
    tmp_D.uLongs = tmp_D.uLongs*1000/Voltage_K;
    ele_tmp->Voltage_Value = tmp_D.uLongs;

    tmp_D.uLongs = BL0942_ReadData(Addr_WATT);	//WATT
    if(tmp_D.uByte[2] > 0x7F )								//有功功率寄存器bit[23]为符号位，负功时补码转换
    {
        tmp_D.uByte[0] = (~tmp_D.uByte[0])+1;
        tmp_D.uByte[1] = ~tmp_D.uByte[1];
        tmp_D.uByte[2] = ~tmp_D.uByte[2];
        sign_WATT=(bool)1;
    }
    tmp_D.uLongs = tmp_D.uLongs*1000/Power_K;
    ele_tmp->Power_Value=tmp_D.uLongs;

    tmp_D.uLongs = BL0942_ReadData(Addr_FREQ);		//工频
    tmp_D.uLongs=100000000/tmp_D.uLongs;			//转换为实际频率
    ele_tmp->Freq=tmp_D.uLongs;

    tmp_D.uLongs = BL0942_ReadData(Addr_CF_CNT);	//电能脉冲计数
    if (tmp_D.uLongs>=ele_tmp->Fir_CF_CNT)
    {
        ele_tmp->Mid_CF_CNT=ele_tmp->Mid_CF_CNT+(tmp_D.uLongs-ele_tmp->Fir_CF_CNT)*1000;
    }
    else
    {
        ele_tmp->Mid_CF_CNT=ele_tmp->Mid_CF_CNT+(tmp_D.uLongs+(0xFFFFFF-ele_tmp->Fir_CF_CNT))*1000;
    }

    ele_tmp->Fir_CF_CNT=tmp_D.uLongs;
    ele_tmp->Energy_kwh=ele_tmp->Energy_kwh+ele_tmp->Mid_CF_CNT/Energy_K;
    ele_tmp->Mid_CF_CNT=ele_tmp->Mid_CF_CNT%Energy_K;

	if(ele_tmp->Current_Value < 10)				//电流小于0.01A 则将其设置为0
		electricdata->Current = 0;
	else
		electricdata->Current = ele_tmp->Current_Value;
	if(ele_tmp->Voltage_Value < 5000)			//电压小于5V 则将其设置为0
		electricdata->Voltage = 0;
	else
		electricdata->Voltage = ele_tmp->Voltage_Value;
	if(ele_tmp->Power_Value < 1000)				//功率小于1W 则将其设置为0
		electricdata->Power = 0;
	else
		electricdata->Power = ele_tmp->Power_Value;
	electricdata->Energy = ele_tmp->Energy_kwh/10;
	myprintf("\r\nV%d = %.3fV,A = %dmA,P = %.3fW,E = %.3f°",channle,(float)electricdata->Voltage/1000,\
				electricdata->Current,(float)electricdata->Power/1000,(float)ele_tmp->Energy_kwh/1000);

#if DRIVER_USE_SPI
    if(channle == 1)
    {
        BL0942_1_CS(1);
    }

	#if ELE_CIRCUIT_NUM >= 2
    else if(channle == 2)
    {
        BL0942_2_CS(1);
    }
	#endif
#endif
	return 0;
}

//如果需要配置用户区寄存器，需要先打开写保护
void BL0942_ConfigInit(uint8_t channle)
{
    Elect_StructDef *ele_tmp;
    if(channle == 1)
    {
		ele_tmp = &BL0942_Elect[0];
#if DRIVER_USE_SPI
        BL0942_1_CS(0);
#else
        BL6523Send = UART1Write;
#endif
    }
#if ELE_CIRCUIT_NUM >= 2
    else if(channle == 2)
    {
		ele_tmp = &BL0942_Elect[1];
	#if DRIVER_USE_SPI
        BL0942_2_CS(0);
	#else
        BL6523Send = UART6Write;
	#endif
    }
#endif
    BL0942_D.uByte[1]=0x00;
    BL0942_D.uByte[2]=0x00;
    BL0942_D.uByte[3]=0x55;
    BL0942_WriteData(Addr_WRPROT,&BL0942_D.uByte[1]);		//USR_WRPROT 开用户区写保护
	
    BL0942_D.uByte[1]=0x5A;
    BL0942_D.uByte[2]=0x5A;
    BL0942_D.uByte[3]=0x5A;
    BL0942_WriteData(Addr_SOFT_RESET,&BL0942_D.uByte[1]);		//软件复位
	
	vTaskDelay(100);
	
	
    BL0942_D.uByte[1]=0x00;
    BL0942_D.uByte[2]=0x00;
    BL0942_D.uByte[3]=0x55;
    BL0942_WriteData(Addr_WRPROT,&BL0942_D.uByte[1]);		//USR_WRPROT 开用户区写保护

    //1398/3.0517578125/8≈57，WA_CREEP=0x39；
    BL0942_D.uByte[1]=0x00;
    BL0942_D.uByte[2]=0x00;
    BL0942_D.uByte[3]=0xA0;
    BL0942_WriteData(Addr_WA_CREEP,&BL0942_D.uByte[1]);//设置有功功率防潜寄存器，用于噪声功率切除，参考设计对应1瓦功率；1W以下的数据切除到0

    BL0942_D.uByte[1]=0x00;
    BL0942_D.uByte[2]=0x00;
    BL0942_D.uByte[3]=0x00;
    BL0942_WriteData(Addr_WRPROT,&BL0942_D.uByte[1]);//USR_WRPROT			关用户区写保护
	
    //演示程序不带电量存储功能，
	
    #if ELE_CIRCUIT_NUM >= 1
    BL0942_Elect[0].Energy_kwh=0;		//实际用电量，千瓦小时 XXXXXX.XXX
    BL0942_Elect[0].Fir_CF_CNT=0;		//上一次的CF_CNT寄存器读数
    BL0942_Elect[0].Mid_CF_CNT=0;		//电能累积换算的中间变量
	#endif
	#if ELE_CIRCUIT_NUM >=2
	BL0942_Elect[1].Energy_kwh=0;		//实际用电量，千瓦小时 XXXXXX.XXX
    BL0942_Elect[1].Fir_CF_CNT=0;		//上一次的CF_CNT寄存器读数
    BL0942_Elect[1].Mid_CF_CNT=0;		//电能累积换算的中间变量
	#endif
	
	if(BL0942_ReadData(Addr_WA_CREEP) != 0x0000A0)	/* 增益寄存器*/
    {
		uint8_t check_flag = 0;
		
		CheckSelf_Flag_Rw(0, &check_flag);
		if(channle == 1)
		{
			check_flag |= 0x01;
			myprintf("\r\n电量计一 ERROR！！！");
		}
#if ELE_CIRCUIT_NUM >= 2
		else if(channle == 2)
		{
			check_flag |= 0x02;
			myprintf("\r\n电量计二 ERROR！！！");
		}
#endif
		CheckSelf_Flag_Rw(1, &check_flag);
    }
    else
	{
		if(channle == 1)
			myprintf("\r\n电量计一 SUCCESS！！！");
#if ELE_CIRCUIT_NUM >= 2
		else if(channle == 2)
			myprintf("\r\n电量计二 SUCCESS！！！");
#endif
	}
	
#if DRIVER_USE_SPI
    if(channle == 1)
    {
        BL0942_1_CS(1);
    }
    #if ELE_CIRCUIT_NUM >= 2
    else if(channle == 2)
    {
        BL0942_2_CS(1);
    }
	#endif
#endif
}

/**********************************************
*Function:			BL0942_Uart_ReadEDataAll
*Description:		BL0942读电参数函数
*Parameter:			使用全电参数数据包读取方式
*Return:			转换为实际电参数数据，放到对应发送数组中
*Author:			EE.Fan
*Data:				2022.03.19
*Note:				如果使用BL0942多机通信时，需要注意切换器件地址进行通讯时延时时间要20mS以上
***********************************************/
void BL0942_ReadEDataAll(uint8_t channle, CollectionData *electricdata)
{
    FourBytes_Type	tmp_D;
    bool sign_WATT=(bool)0;					//有功功率的符号位，0表示正功，1表示负功
	bool flg_Comm;

    Elect_StructDef *ele_tmp;
    if(channle == 1)
    {
		ele_tmp = &BL0942_Elect[0];
#if DRIVER_USE_SPI
        BL0942_1_CS(0);
#else
        BL6523Send = UART1Write;
#endif
    }
#if ELE_CIRCUIT_NUM >= 2
    else if(channle == 2)
    {
		ele_tmp = &BL0942_Elect[1];
	#if DRIVER_USE_SPI
        BL0942_2_CS(0);
	#else
        BL6523Send = UART6Write;
	#endif
    }
#endif

    flg_Comm = BL0942_ReadAllData(0xAA);
    if (flg_Comm==TRUE)
    {
        tmp_D.uByte[0]=0x00;
        tmp_D.uByte[1]=BL0942_AllFlame.Flame.I_RMS[2];
        tmp_D.uByte[2]=BL0942_AllFlame.Flame.I_RMS[1];
        tmp_D.uByte[3]=BL0942_AllFlame.Flame.I_RMS[0];				//注意在不同编译器下编译的大小端对齐问题！！！
        tmp_D.uLongs=tmp_D.uLongs*1000/Current_K;
        ele_tmp->Current_Value=tmp_D.uLongs;						//电流有效值

        tmp_D.uByte[0]=0x00;
        tmp_D.uByte[1]=BL0942_AllFlame.Flame.I_FAST_RMS[2];
        tmp_D.uByte[2]=BL0942_AllFlame.Flame.I_FAST_RMS[1];
        tmp_D.uByte[3]=BL0942_AllFlame.Flame.I_FAST_RMS[0];				//注意在不同编译器下编译的大小端对齐问题！！！
        tmp_D.uLongs=tmp_D.uLongs*1000/Current_K;
        ele_tmp->F_Current_Value=tmp_D.uLongs;						//快速电流有效值

        tmp_D.uByte[0]=0x00;
        tmp_D.uByte[1]=BL0942_AllFlame.Flame.V_RMS[2];
        tmp_D.uByte[2]=BL0942_AllFlame.Flame.V_RMS[1];
        tmp_D.uByte[3]=BL0942_AllFlame.Flame.V_RMS[0];
        tmp_D.uLongs=tmp_D.uLongs*1000/Voltage_K;
        ele_tmp->Voltage_Value=tmp_D.uInt[1];				//电压有效值

        tmp_D.uByte[0]=0x00;
        tmp_D.uByte[1]=BL0942_AllFlame.Flame.WATT[2];
        tmp_D.uByte[2]=BL0942_AllFlame.Flame.WATT[1];
        tmp_D.uByte[3]=BL0942_AllFlame.Flame.WATT[0];		//A_WATT
        if(tmp_D.uByte[1]>0x7F )								//有功功率寄存器bit[23]为符号位，补码转换
        {
            tmp_D.uByte[1]=~tmp_D.uByte[1];
            tmp_D.uByte[2]=~tmp_D.uByte[2];
            tmp_D.uByte[3]=~tmp_D.uByte[3]+1;
            sign_WATT=(bool)1;
        }
        tmp_D.uLongs=tmp_D.uLongs*1000/Power_K;
        ele_tmp->Power_Value=tmp_D.uLongs;

        tmp_D.uByte[0]=0x00;
        tmp_D.uByte[1]=BL0942_AllFlame.Flame.FREQ[2];
        tmp_D.uByte[2]=BL0942_AllFlame.Flame.FREQ[1];
        tmp_D.uByte[3]=BL0942_AllFlame.Flame.FREQ[0];				//FREQ
        tmp_D.uLongs=100000000/tmp_D.uLongs;			//转换为实际频率
        ele_tmp->Freq=tmp_D.uLongs;

        tmp_D.uByte[0]=0x00;
        tmp_D.uByte[1]=BL0942_AllFlame.Flame.CF_CNT[2];
        tmp_D.uByte[2]=BL0942_AllFlame.Flame.CF_CNT[1];
        tmp_D.uByte[3]=BL0942_AllFlame.Flame.CF_CNT[0];				//电能脉冲计数

        if (tmp_D.uLongs>=ele_tmp->Fir_CF_CNT)
        {
            ele_tmp->Mid_CF_CNT=ele_tmp->Mid_CF_CNT+(tmp_D.uLongs-ele_tmp->Fir_CF_CNT)*1000;
        }
        else if(ele_tmp->Fir_CF_CNT>0xFFFF00)			//判断是否是累积溢出，翻零重计
        {
            ele_tmp->Mid_CF_CNT=ele_tmp->Mid_CF_CNT+(tmp_D.uLongs+(0xFFFFFF-ele_tmp->Fir_CF_CNT))*1000;
        }
		ele_tmp->Fir_CF_CNT=tmp_D.uLongs;
		ele_tmp->Energy_kwh=ele_tmp->Energy_kwh+ele_tmp->Mid_CF_CNT/Energy_K;
		ele_tmp->Mid_CF_CNT=ele_tmp->Mid_CF_CNT%Energy_K;
		
		if(ele_tmp->Current_Value < 10)				//电流小于0.01A 则将其设置为0
			electricdata->Current = 0;
		else
			electricdata->Current = ele_tmp->Current_Value;
		if(ele_tmp->Voltage_Value < 1000)			//电压小于1V 则将其设置为0
			electricdata->Voltage = 0;
		else
			electricdata->Voltage = ele_tmp->Voltage_Value;
		if(ele_tmp->Power_Value < 1000)				//功率小于1W 则将其设置为0
			electricdata->Power = 0;
		else
			electricdata->Power = ele_tmp->Power_Value;
		electricdata->Energy = ele_tmp->Energy_kwh/10;
		myprintf("\r\nV%d = %dV,A = %dmA,P = %.2fW,E= %d° e = %3f°",channle,electricdata->Voltage,\
					electricdata->Current,electricdata->Power,electricdata->Energy,electricdata->Energy);
    }
	
#if DRIVER_USE_SPI
    if(channle == 1)
    {
        BL0942_1_CS(1);
    }
    #if ELE_CIRCUIT_NUM >= 2
    else if(channle == 2)
    {
        BL0942_2_CS(1);
    }
	#endif
#endif
}

uint8_t BL0942_CheckSelf(uint8_t channle)
{
    uint8_t ret = 0;
    uint32_t wd = 0;

    if(channle == 1)
    {
#if DRIVER_USE_SPI
        BL0942_1_CS(0);
#else
        BL6523Send = UART1Write;
#endif
    }
#if ELE_CIRCUIT_NUM >= 2
    else if(channle == 2)
    {
#if DRIVER_USE_SPI
        BL0942_2_CS(0);
#else
        BL6523Send = UART6Write;
#endif
    }
#endif
	
    wd = BL0942_ReadData(Addr_WA_CREEP);			/* 增益寄存器*/
    if(wd == 0x0000A0)
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }
#if DRIVER_USE_SPI
    if(channle == 1)
    {
        BL0942_1_CS(1);
    }
    #if ELE_CIRCUIT_NUM >= 2
    else if(channle == 2)
    {
        BL0942_2_CS(1);
    }
	#endif
#endif
    return ret;
}
