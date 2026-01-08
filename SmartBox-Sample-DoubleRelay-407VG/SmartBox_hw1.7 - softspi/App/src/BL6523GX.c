#include "BL6523GX.h"
#include "SysConfig.h"
#include "usart.h"
#include "MyFlash.h"
#include "StorageConfig.h"
#include "mystring.h"

ftoc VREF;
ftoc IREF;

QueueHandle_t BL6523GX_RECEIVE_Queue;
CollectionData ElectricData = {0};		/*将IRMT_S1结构体的值都初始化为0*/
CollectionData ElectricData2 = {0};		/*将IRMT_S1结构体的值都初始化为0*/
uint32_t SaveEnergy[2];					//保存在flash中的已经产生的能量
uint32_t U_k;
uint32_t I_k;
uint32_t Watt_k;

uint32_t U2_k;
uint32_t I2_k;
uint32_t Watt2_k;

uint32_t Who_Am_I[2];

void (*BL6523Send) (uint8_t *, uint16_t);
extern uint8_t Write_UserFlashAndSaveEnergy(uint32_t _ulFlashAddr, uint8_t *_ucpSrc, uint32_t _ulSize);

void BL6523GX_REF_Set(uint8_t *buf,uint16_t len)
{
	uint32_t vref,iref;
	uint16_t pp;
	uint16_t qq;
	pp = StrFindString(buf, len, (uint8_t *)"ele cal:", 8);
	if(pp != 0xffff)
	{
		qq = StrFindString(buf, len, (uint8_t *)",", 1);
		if(Str2Num(&buf[pp+8], qq-pp-8, &vref) == 0) return ;
		if(Str2Num(&buf[qq+1], len-qq-1, &iref) == 0) return ;
		taskENTER_CRITICAL();
		VREF.f = vref;
		IREF.f = (float)iref/1000;
		Write_UserFlashAndSaveEnergy(ENERGY_REF_ADDR,(uint8_t *)&VREF.c, 4);
		Write_UserFlashAndSaveEnergy(ENERGY_REF_ADDR+4,(uint8_t *)&IREF.c, 4);
		taskEXIT_CRITICAL();
		myprintf("\r\n校准参考电压:%dV,电流:%.3fA",vref,(float)iref/1000);
	}
}

uint8_t BL6523GX_Check(uint8_t * check_data)
{
	uint8_t res = 0;
	
	res = (check_data[0]+check_data[1]+check_data[2]+check_data[3]) & 0xff;
	res = ~res;
	return res;
}

/*******************************************************************************
** 函数名称: BL6526B_ReadData
** 功能描述：往里面传一个CMD = 0X00 + Address
** 函数说明：BL6526B_ReadData从BL6526B里面读数据出来 24bit数据
** 作　  者:
** 日　  期:
*******************************************************************************/
uint32_t BL6523GX_ReadData(uint8_t reg_addr)
{
    uint8_t byte[130];
	uint8_t tmp[2];
    uint32_t tmpdata = 0;
	uint8_t sum = 0;
	
	byte[0] = reg_addr;
	tmp[0] = 0x35;
	tmp[1] = reg_addr;
	//先往地址里面写一个字节 告诉BL6526B的哪个寄存器 
	BL6523Send(tmp,2);
//	delay5us(10);
	if(xQueueReceive(BL6523GX_RECEIVE_Queue, (void *)&byte[1], ( TickType_t )300) != pdPASS)
	{
		return 0;
	}
	sum = BL6523GX_Check(byte);
	if(sum == byte[4])
	{
		tmpdata |= (byte[3]<<16);
		tmpdata |= (byte[2]<<8);
		tmpdata |= byte[1];
		return tmpdata;
	}
	else
		return 0;
}

/*******************************************************************************
** 函数名称: BL6526B_WriteData
** 功能描述：往里面传一个CMD = 0X00 + Address
** 函数说明：BL6526B_WriteData
** 作　  者:
** 日　  期:
*******************************************************************************/
void BL6523GX_WriteData(uint8_t reg_addr,uint8_t byte1,uint8_t byte2,uint8_t byte3)
{
	uint8_t tmp[6];
	
	//先往地址里面写一个字节 告诉BL6526B的哪个寄存器
	tmp[0] = 0xCA;
	tmp[1] = reg_addr;
	tmp[2] = byte1;
	tmp[3] = byte2;
	tmp[4] = byte3;
	tmp[5] = BL6523GX_Check(&tmp[1]);
	BL6523Send(tmp,6);
}

/*******************************************************************************
** 函数名称: Bl6526bState_Process
** 功能描述：
** 函数说明：设置电量采集芯片状态(正常,过流,过压,过零)函数
** 作　  者:
** 日　  期:
*******************************************************************************/
void Bl6523GXState_Process(BL6523GXStruct *bl6526bdata,CollectionData *electricdata)
{
    if (bl6526bdata->gain != 0x000033)
    {
        electricdata->Bl6526bState |= 0X01;
		#if BL6523GX_Printf
			myprintf("\n******************BL6523GX通讯异常********************\n");
		#endif
		return;
    }
//    if (bl6526bdata->status & 0x28)
//    {
//        electricdata->Bl6526bState = 0X04;
//    }
//    else if (bl6526bdata->status & 0x08)
//    {
//        electricdata->Bl6526bState = 0X02;
//    }
	if (bl6526bdata->status & 0x20)
    {
        electricdata->Bl6526bState |= 0X02;
    }
    else
    {
        electricdata->Bl6526bState = 0X00;
    }

    switch(electricdata->Bl6526bState)
    {
    case 0x00:		//正常
		#if BL6523GX_Printf
        myprintf("\n******************BL6523GX正常********************\n");
		#endif

        break;

    case 0x02:		//过压
		#if BL6523GX_Printf
        myprintf("\n******************BL6523GX电流过压********************\n");
		#endif
        break;

    case 0x04:		//过流
		#if BL6523GX_Printf
        myprintf("\n******************BL6523GX电流过流********************\n");
		#endif
        break;

    case 0x08:		//同时过压过流
		#if BL6523GX_Printf
        myprintf("\n******************BL6523GX电流过压过流********************\n");
		#endif

        break;
    }

}

uint8_t BL6523GX_CheckSelf(uint8_t channle)
{
	uint8_t ret = 0;
	uint32_t wd = 0;
	
	if(channle == 1)
	{
		BL6523Send = UART8Write;
	} 
	else if(channle == 2)
	{
		BL6523Send = UART2Write;
	} 
	
	wd = (BL6523GX_ReadData(BL6523G_GAIN));			/* 增益寄存器*/
	if(wd == 0x000033)
	{
		ret = 0;
	}
	else
	{
		ret = 1;
	}
	return ret;
}

void BL6523GX_Init(uint8_t channle)
{
	if(channle == 1)
	{
		BL6523Send = UART8Write;
		BL6523GX_1_NRST_LOW();
		HAL_Delay(100);
		BL6523GX_1_NRST_HIGH();
		HAL_Delay(300);
	} 
	else if(channle == 2)
	{
		BL6523Send = UART2Write;
		BL6523GX_2_NRST_LOW();
		HAL_Delay(100);
		BL6523GX_2_NRST_HIGH();
		HAL_Delay(300);
	}
	
	Who_Am_I[channle-1] = BL6523GX_ReadData(BL6523G_WHO_AM_I);
	if(Who_Am_I[channle-1] == BL6523GX_D78)
		myprintf("\r\n ele channle:%d is BL6523GX_D78\r\n", channle);
	else if(Who_Am_I[channle-1] == BL6523GX_D70)
		myprintf("\r\n ele channle:%d is BL6523GX_D70\r\n", channle);
	else if(Who_Am_I[channle-1] == BL6523GX_D75)
		myprintf("\r\n ele channle:%d is BL6523GX_D75\r\n", channle);
	
	BL6523GX_WriteData((BL6523G_WRPROT ),0x55,0x00,0x00);	// 8bits 关闭写保护
	HAL_Delay(1);
	if(Who_Am_I[channle-1] == BL6523GX_D78)
		BL6523GX_WriteData((BL6523G_MODE ),0x01,0x02,0x00);		//12bits 使用高通滤波器，能量累加模式采用绝对值累加
	else if(Who_Am_I[channle-1] == BL6523GX_D70)
		BL6523GX_WriteData((BL6523G_MODE ),0x01,0x02,0x00);		//12bits 使用高通滤波器，能量累加模式采用绝对值累加
	else if(Who_Am_I[channle-1] == BL6523GX_D75)
		BL6523GX_WriteData((BL6523G_MODE ),0x01,0x00,0x20);		//12bits 使用高通滤波器，能量累加模式采用绝对值累加
															//选择B相功率累加，使用功率阈值比较方式防潜动，关闭定时防潜方式
	HAL_Delay(1);
    BL6523GX_WriteData((BL6523G_GAIN ),0x33,0x00,0x00);		//12bits 电流增益8倍，电压1倍增益
	HAL_Delay(1);
    BL6523GX_WriteData((BL6523G_MASK ),0x20,0x00,0x00);		//12bits 中断屏蔽寄存器，过压，，
	HAL_Delay(1);
//    BL6523GX_WriteData((BL6523G_V_PKLVL ),0xD3,0x03,0x00);	//12bits 250V左右会过压
//	BL6523GX_WriteData((BL6523G_V_PKLVL ),0x0E,0x03,0x00);	//12bits 200V左右会过压
	HAL_Delay(1);
//    BL6523GX_WriteData((BL6523G_I_PKLVL ),0xEA,0x03,0x00);	//12bits 5A电流左右会过流
	BL6523GX_WriteData((BL6523G_WRPROT ),0x00,0x00,0x00);	// 8bits 开启写保护
	HAL_Delay(1);
}

void BL6523GX_Triming(uint8_t channle)
{
	BL6523GXStruct bl6526bdata = {0};
    float v_k,ia_k,ib_k,watt_k;
	uint8_t i = 0;
	
	if(channle == 1)
	{
		BL6523Send = UART8Write;
	} 
	else if(channle == 2)
	{
		BL6523Send = UART2Write;
	}
	for(i = 0;i<(CAL_CNT+1);i++)
	{
		if(i>=1)
		{
			bl6526bdata.v_rms += (BL6523GX_ReadData(BL6523G_V_RMS));			/* 电压有效值寄存器*/
			bl6526bdata.ia_rms += (BL6523GX_ReadData(BL6523G_IA_RMS));		/* 电流有效值寄存器*/
			bl6526bdata.ib_rms += (BL6523GX_ReadData(BL6523G_IB_RMS));		/* 电流有效值寄存器*/
			bl6526bdata.watt += (BL6523GX_ReadData(BL6523G_B_WATT));			/* 平均有功功率寄存器*/
		}
		vTaskDelay(400);
	}
	bl6526bdata.v_rms /= CAL_CNT;
	bl6526bdata.ia_rms /= CAL_CNT;
	bl6526bdata.ib_rms /= CAL_CNT;
	bl6526bdata.watt /= CAL_CNT;
	if(bl6526bdata.watt >= 0x800000)
		bl6526bdata.watt -= 0x800000;
	
	v_k = bl6526bdata.v_rms / VREF.f;								/* 12335.58是实际测试中220V 每v的寄存器值 */
	ib_k = bl6526bdata.ib_rms / IREF.f;								/* 555914.9是实际测试中1A 每a的寄存器值 */
	
	watt_k = bl6526bdata.watt / (VREF.f*IREF.f);					//默认是230V 0.135A
	if(channle == 1)
	{
//		U_k.f = v_k;
//		I_k.f = ib_k;
//		
//		Watt_k.f = watt_k;
		
		U_k = bl6526bdata.v_rms;
		I_k = bl6526bdata.ib_rms;
		Watt_k = bl6526bdata.watt;
		
//		Write_UserMem_Flash(ENERGY_PARA_ADDR,(uint8_t *)&U_k, 4);
//		Write_UserMem_Flash(ENERGY_PARA_ADDR+4,(uint8_t *)&I_k, 4);
//		Write_UserMem_Flash(ENERGY_PARA_ADDR+8,(uint8_t *)&Watt_k, 4);
	}
	else if(channle == 2)
	{
//		U2_k.f = v_k;
//		I2_k.f = ib_k;
//		
//		Watt2_k.f = watt_k;
		
		U2_k = bl6526bdata.v_rms;
		I2_k = bl6526bdata.ib_rms;
		Watt2_k = bl6526bdata.watt;
		
//		Write_UserMem_Flash(ENERGY_PARA_ADDR+12,(uint8_t *)&U2_k, 4);
//		Write_UserMem_Flash(ENERGY_PARA_ADDR+12+4,(uint8_t *)&I2_k, 4);
//		Write_UserMem_Flash(ENERGY_PARA_ADDR+12+8,(uint8_t *)&Watt2_k, 4);
	}
	
	myprintf("--------------------------------------\r\n");			/*分割*/
	
    myprintf("BL6523G1_V_RMS = %d\r\n", bl6526bdata.v_rms);
    myprintf("BL6523G1_I_RMS = %d\r\n", bl6526bdata.ib_rms);
	myprintf("BL6523G1_WATT = %d\r\n", bl6526bdata.watt);
	myprintf("BL6523G1_VAHR = %d\r\n", bl6526bdata.vahr);
	myprintf("v_k = %2f\r\n", v_k);
	myprintf("ia_k = %2f\r\n", ib_k);
	myprintf("ib_k = %2f\r\n", ib_k);
	myprintf("watt_k = %2f\r\n", watt_k);
	myprintf("--------------------------------------\r\n");			/*分割*/
}

uint8_t BL6523GX_ProcessTask(uint8_t channle, CollectionData *electricdata)
{
	BL6523GXStruct bl6526bdata;
    float v,ia,ib,pa,pb,e,watt_k ,v_k,ia_k,ib_k;
    float sum = 0.0;
    uint8_t x = 0;
    v=ia=ib=pa=pb=e=watt_k=0.0;

	if(channle == 1)
	{
		BL6523Send = UART8Write;
		v_k = U_k / VREF.f;
		ia_k = I_k / IREF.f;
		ib_k = I_k / IREF.f;
		
		watt_k = Watt_k / (VREF.f*IREF.f);
	} 
	else if(channle == 2)
	{
		BL6523Send = UART2Write;
		v_k = U2_k / VREF.f;
		ia_k = I2_k / IREF.f;
		ib_k = I2_k / IREF.f;
		
		watt_k = Watt2_k / (VREF.f*IREF.f);
		
	} 
//    BL6523GX_WriteData((BL6523G_WRPROT ),0x00,0x00,0x55);		// 8bits 关闭写保护
//    BL6523GX_WriteData((BL6523G_MODE ),0x00,0x00,0x01);		//12bits 使用高通滤波器，能量累加模式采用绝对值累加
//																	//使用功率阈值比较方式防潜动，关闭定时防潜方式
//    BL6523GX_WriteData((BL6523G_GAIN ),0x00,0x00,0x03);		//12bits 电流增益8倍，电压1倍增益
//    BL6523GX_WriteData((BL6523G_MASK ),0x00,0x00,0x28);		//12bits 中断屏蔽寄存器，过压，过流中断，
//    BL6523GX_WriteData((BL6523G_V_PKLVL ),0x00,0x03,0xEA);		//12bits 240V左右会过压
////    BL6523GX_WriteData((BL6523G_I_PKLVL | 0x40),0x00,0x03,0xE8);		//12bits 5A电流左右会过流
	
    bl6526bdata.gain = (BL6523GX_ReadData(BL6523G_GAIN));			/* 增益寄存器*/
	if(bl6526bdata.gain != 0x000033)
	{
		Bl6523GXState_Process(&bl6526bdata,electricdata);
		return 1;
	}

    bl6526bdata.status = (BL6523GX_ReadData(BL6523G_STATUS));		/* 中断状态寄存器*/

    bl6526bdata.wrprot = (BL6523GX_ReadData(BL6523G_WRPROT));		/* 写保护设置寄存器*/

    bl6526bdata.mode = (BL6523GX_ReadData(BL6523G_MODE));			/* 工作模式寄存器*/

    bl6526bdata.mask = (BL6523GX_ReadData(BL6523G_MASK));			/* 中断屏蔽寄存器*/

    bl6526bdata.v_pklvl = (BL6523GX_ReadData(BL6523G_V_PKLVL));		/* 电压峰值门限寄存器*/

    bl6526bdata.v_rms = (BL6523GX_ReadData(BL6523G_V_RMS));			/* 电压有效值寄存器*/

    bl6526bdata.ia_rms = (BL6523GX_ReadData(BL6523G_IA_RMS));			/* 电流有效值寄存器*/

    bl6526bdata.ib_rms = (BL6523GX_ReadData(BL6523G_IB_RMS));			/* 电流有效值寄存器*/

//    bl6526bdata.v_wave = (BL6523GX_ReadData(BL6523G_V_WAVE));		/* 电压波形寄存器*/
//    myprintf("BL6523G1_V_WAVE = %d\r\n", bl6526bdata.v_wave);

//    bl6526bdata.i_wave = (BL6523GX_ReadData(BL6523G_I_WAVE));		/* 电流波形寄存器*/
//    myprintf("BL6523G1_I_WAVE = %d\r\n", bl6526bdata.i_wave);

//    bl6526bdata.v_peak = (BL6523GX_ReadData(BL6523G_V_PEAK));		/* 电压瞬态峰值寄存器*/
//    myprintf("BL6523G1_V_PEAK = %d\r\n", bl6526bdata.v_peak);

//    bl6526bdata.i_peak = (BL6523GX_ReadData(BL6523G_I_PEAK));		/* 电流瞬态峰值寄存器*/
//    myprintf("BL6523G1_I_PEAK = %d\r\n", bl6526bdata.i_peak);

    bl6526bdata.watt = (BL6523GX_ReadData(BL6523G_B_WATT));			/* 平均有功功率寄存器*/
	
    bl6526bdata.cf_div = (BL6523GX_ReadData(BL6523G_WA_CFDIV));			/* 平均有功功率寄存器*/
	bl6526bdata.cf_cnt = (BL6523GX_ReadData(BL6523G_COUNTER_CF));			/* CF脉冲个数*/
	bl6526bdata.vahr = (BL6523GX_ReadData(BL6523G_WATTHR));				/* 视在能量寄存器*/

//    bl6526bdata.va = (BL6523GX_ReadData(BL6523G_VA));				/* 平均视在功率寄存器*/
//    myprintf("BL6523G1_VA = %d\r\n", bl6526bdata.va);

//    bl6526bdata.pf = (BL6523GX_ReadData(BL6523G_PF));				/* 功率因子寄存器*/
//    for(x = 1; x <= 23; x++)
//    {
//        sum += ((float)pow(2,x-24)) * (bl6526bdata.pf>>(x-1) & (0x01));
//    }
//    myprintf("BL6523G1_PF = %f\r\n", sum);


//	if(Who_Am_I[channle-1] == BL6523GX_D78)
//		bl6526bdata.vahr = (BL6523GX_ReadData(BL6523G_WATTHR));			/* 视在能量寄存器*/
//	else if(Who_Am_I[channle-1] == BL6523GX_D75)
//		bl6526bdata.vahr = (BL6523GX_ReadData(BL6523G_COUNTER_CF));			/* 视在能量寄存器*/
//	bl6526bdata.vahr = (BL6523GX_ReadData(BL6523G_COUNTER_CF));			/* 视在能量寄存器*/
#if 1
    v = bl6526bdata.v_rms / v_k;								/* 12335.58是实际测试中220V 每v的寄存器值 */
    ia = bl6526bdata.ia_rms / ia_k;								/* 555914.9是实际测试中1A 每a的寄存器值 */
    ib = bl6526bdata.ib_rms / ib_k;								/* 555914.9是实际测试中1A 每a的寄存器值 */
    pa = v * ia;
	pb = v * ib;
	
	if(bl6526bdata.watt >= 0x800000)
		bl6526bdata.watt -= 0x800000;
	pb = bl6526bdata.watt / watt_k;
	
//	watt_k = bl6526bdata.watt/(pb);
	
//	e = bl6526bdata.vahr / (watt_k*17578.125);					//17578.125为手册上介绍
	
	if(Who_Am_I[channle-1] == BL6523GX_D78)
		e = (bl6526bdata.cf_cnt*16384) / (watt_k*17578.125);					//17578.125为手册上介绍
	else if(Who_Am_I[channle-1] == BL6523GX_D70)
		e = bl6526bdata.cf_cnt / (watt_k*17578.125);					//17578.125为手册上介绍
	else if(Who_Am_I[channle-1] == BL6523GX_D75)
		e = bl6526bdata.cf_cnt / (watt_k*17578.125);					//17578.125为手册上介绍
#else
	v = bl6526bdata.v_rms / 25;								/* 12335.58是实际测试中220V 每v的寄存器值 */
    ia = bl6526bdata.ia_rms / 496941.176;								/* 555914.9是实际测试中1A 每ma的寄存器值 */
    ib = bl6526bdata.ib_rms / 496941.176;								/* 555914.9是实际测试中1A 每ma的寄存器值 */
    pa = v * ia;
	pb = v * ib;
	
	watt_k = bl6526bdata.watt/(223.98*0.135);
	
//    e = bl6526bdata.vahr / 20253000.0;
	e = bl6526bdata.vahr / 5443487.1371294;
	e = bl6526bdata.vahr / (watt_k*17578.125);
#endif

//    if (((uint32_t)(ia * 1000)) < 10)								//电流小于0.01A 则将其设置为0
//        electricdata->Current = 0;
//    else
//        electricdata->Current = (uint32_t)(ia * 1000);
	
	if (((uint32_t)(ib * 1000)) < 10)								//电流小于0.01A 则将其设置为0
        electricdata->Current = 0;
    else
        electricdata->Current = (uint32_t)(ib * 1000);

    if (((uint32_t)(v * 1000)) < 1000)								//电压小于1V 则将其设置为0
        electricdata->Voltage = 0;
    else
        electricdata->Voltage = (uint32_t)(v * 1000);

//    if (((uint32_t)(pa * 1000)) < 1000)								//功率小于1W 则将其设置为0
//        electricdata->Power = 0;
//    else
//        electricdata->Power = (uint32_t)(pa * 10);
//	
//	if (((uint32_t)(pb * 1000)) < 1000)								//功率小于1W 则将其设置为0
//        electricdata->Power = 0;
//    else
//        electricdata->Power = (uint32_t)(pb * 10);
	
	electricdata->Power = (uint32_t)(pb * 1);
    electricdata->Energy = (uint32_t)(e * 100);

#if BL6523GX_Printf
    myprintf("\r\n------------------------\r\n");			/*分割*/
    myprintf("BL6523G1_STATUS = 0X%02X\r\n", bl6526bdata.status);
    myprintf("BL6523G1_WRPROT = 0x%02X\r\n", bl6526bdata.wrprot);
    myprintf("BL6523G1_MODE = 0x%02X\r\n", bl6526bdata.mode);
    myprintf("BL6523G1_GAIN = 0x%02X\r\n", bl6526bdata.gain);
    myprintf("BL6523G1_MASK = 0x%02X\r\n", bl6526bdata.mask);
    myprintf("BL6523G1V_PKLVL = 0x%02X\r\n", bl6526bdata.v_pklvl);
    myprintf("BL6523G1_I_PKLVL = 0x%02X\r\n", bl6526bdata.i_pklvl);
    myprintf("BL6523G1_V_RMS = %d\r\n", bl6526bdata.v_rms);
    myprintf("BL6523G1_I_RMS = %d\r\n", bl6526bdata.ib_rms);
	myprintf("BL6523G_WA_CFDIV = %d\r\n", bl6526bdata.cf_div);
	myprintf("BL6523G1_WATT = %d\r\n", bl6526bdata.watt);
	myprintf("BL6523G_COUNTER_CF = %d\r\n", bl6526bdata.cf_cnt);
	myprintf("BL6523G1_VAHR = %d\r\n", bl6526bdata.vahr);
//    myprintf("V1 = %d mV,I1 = %d mA,P = %d mW\n",electricdata->Voltage,electricdata->Current,(uint32_t)(pb));
	myprintf("V%d = %dV,A = %dmA,P = %.2fW,E= %d° e = %3f° \n",channle,electricdata->Voltage/1000,electricdata->Current,pb,electricdata->Energy,e);
    myprintf("------------------------\r\n");			/*分割*/
#else
	myprintf("\r\nV%d = %dV,A = %dmA,P = %.2fW,E= %d° e = %3f°",channle,electricdata->Voltage/1000,electricdata->Current,pb,electricdata->Energy,e);

#endif
	Bl6523GXState_Process(&bl6526bdata,electricdata);								/*设置电量采集芯片状态(正常,过流,过压,过零)*/
	return 0;
}

