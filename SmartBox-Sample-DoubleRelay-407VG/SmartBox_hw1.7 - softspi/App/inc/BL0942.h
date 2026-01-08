/****************************************
BL0942 Uart通信方式，
指定寄存器地址读写方式；
全电参数数据包通信代码
电能保存需要根据采用的EEPROM进行存储，代码暂未实现
******************************************/
#ifndef _BL0942_H
#define _BL0942_H

#include "main.h"

#define ELE_CIRCUIT_NUM			2	//电量计路数

#define DRIVER_USE_RTOS			1
#define DRIVER_USE_SPI			1	//spi工作模式 Mode1:CPOL=0,CPHA=1
#if DRIVER_USE_SPI
#define DRIVER_USE_HARD_SPI		1	//0:模拟SPI   1:硬件SPI
#endif

//电压电流采样使用方案，互感器or电阻分压
#define SAMPLE_USE_PT_CT		1	//互感器
#if SAMPLE_USE_PT_CT == 0
#define SAMPLE_USE_RD		1		//电阻分压
#endif

/*   SPI端口定义 */
#define BL0942_1_CS(x)			HAL_GPIO_WritePin(SPI1_CS1_GPIO_Port, SPI1_CS1_Pin, x)
#define BL0942_2_CS(x)			HAL_GPIO_WritePin(SPI1_CS2_GPIO_Port, SPI1_CS2_Pin, x)
#define SPI_SCLK(x)				HAL_GPIO_WritePin(SPI1_SCK_GPIO_Port, SPI1_SCK_Pin, x)
#define SPI_MOSI(x)				HAL_GPIO_WritePin(SPI1_MOSI_GPIO_Port, SPI1_MOSI_Pin, x)
#define SPI_MISO_IN				HAL_GPIO_ReadPin(SPI1_MISO_GPIO_Port,SPI1_MISO_Pin)
#ifndef bool
	typedef _Bool bool;
	#define FALSE 	0
	#define TRUE 	1
#endif

//电参数寄存器
#define Addr_I_WAVE				0x01	//电流通道波形			*
#define Addr_V_WAVE				0x02	//电压通道波形*
#define Addr_I_RMS				0x03	//电流有效值
#define Addr_V_RMS				0x04	//电压有效值
#define Addr_I_FAST_RMS			0x05	//电流快速有效值*
#define Addr_WATT				0x06	//有功功率
#define Addr_CF_CNT				0x07	//有功电能脉冲计数
#define	Addr_FREQ				0x08	//工频频率
#define Addr_STATUS				0x09	//状态
#define Addr_VERSION			0x0F	//版本
//用户操作寄存器
#define Addr_I_CHOS				0x11	//电流通道直流偏置校正
#define Addr_I_RMSOS			0x12	//电流通道有效值小信号校正
#define Addr_WA_CREEP			0x14	//有功功率防潜阈值
#define Addr_FAST_RMS_TH		0x15	//
#define Addr_FAST_RMS_CYC		0x16
#define Addr_FREQ_CYC			0x17
#define Addr_MASK				0x18
#define Addr_MODE				0x19
#define Addr_GAIN_CR			0x1A
#define Addr_SOFT_RESET			0x1C	//软复位
#define Addr_WRPROT				0x1D	//用户写保护设置

//注意 BL0940的读命令字节固定为0x58+ICAddr，写命令字节固定为0xA8+ICAddr；SOP10封装芯片的IC_Addr地址固定为0
//     BL0942 TSSOP14封装带地址选择管脚，需根据A1~A2地址选择管脚的电平配置命令字节，可以进行多机并联通信
#define BL0942_Addr_R 0x58
#define BL0942_Addr_w 0xA8
 
#if SAMPLE_USE_PT_CT == 1
#define Voltage_K				15124	// 73978*0.0249*1000/1.218/(20*5)		电压转换系数
#define Current_K				829004	// 305978*(3.3*1000)/1000/1.218			电流转换系数
#define Power_K					1959;	// 3537*(3.3*1000/1000)*0.0249*1000/1.218/1.218/20/5		功率转换系数
#define Energy_K				16815;	// 3600000*3537*(3.3*1000/1000)*0.0249*1000/1.218/1.218/20/5/1638.4/256		电能转换系数，电能脉冲计数，对应于1度电的脉冲计数

#endif
#if SAMPLE_USE_RD
//出厂校准芯片，增益控制1%以内，外围器件精度控制1%以内，采用同一系数，不用 EEPROM保存参数
// 电流采用1毫欧电阻采样，电压采用390K*5+0.51K进行分压，实际测试发现电阻存在偏差，进行微调
//BL0942评估版，立创直接贴片合金电阻(台湾厚声MS121WF100NT4E  )，实际测量比1毫欧偏小，约0.93毫欧

//#define Power_K					5798;	// 3537*1毫欧*0.51K*1000/(1.218*1.218)/(390K*5+0.51K)	功率转换系数
//#define Current_K				23362;	// 305978*1毫欧/1.218										电流转换系数
//#define Voltage_K				15883;	// 73989*0.51K*1000/1.218/(390K*5+0.51K)					电压转换系数
//#define Energy_K				4976;	// 3600000*Power_K/16384/256								电能转换系数，电能脉冲计数，对应于1度电的脉冲计数

#define Current_K				233629;	// 305978*1毫欧/1.218										电流转换系数
#define Voltage_K				15883	// 73989*0.51K*1000/1.218/(390K*5+0.51K)					电压转换系数
#define Power_K					580;	// 3537*1毫欧*0.51K*1000/(1.218*1.218)/(390K*5+0.51K)		功率转换系数
#define Energy_K				4978;	// 3600000*Power_K/1638.4/256								电能转换系数，电能脉冲计数，对应于1度电的脉冲计数

//采用美隆0.001毫欧贴片合金电阻，实际比1毫欧偏大，约1.023毫欧
/*
#define Power_K					6378;	// 功率转换系数
#define Current_K				25699;	// 电流转换系数
#define Voltage_K				15883; 	// 电压转换系数
#define Energy_K				5474;   //
*/
#endif

typedef union
{
    uint8_t		uByte[4];
    uint16_t 	uInt[2];
    uint32_t 	uLongs;
} FourBytes_Type;

typedef union
{
    uint8_t		uByte[2];
    uint16_t 	uInt;
} TwoByte_Type;
//全电参数数据包格式
typedef struct
{
    uint8_t	head;
    uint8_t I_RMS[3];
    uint8_t V_RMS[3];
    uint8_t I_FAST_RMS[3];
    uint8_t WATT[3];
    uint8_t CF_CNT[3];
    uint8_t FREQ[3];
    uint8_t STATUS[3];
    uint8_t	CHKSUM;
} AllFlame_StructDef;

typedef union
{
    uint8_t B8[23];
    AllFlame_StructDef Flame;
} AllFlame_TypeDef;
//BL0942/0940的电参数定义
typedef struct
{
    uint32_t		Energy_kwh;			//实际用电量，千瓦小时 XXXXXX.XXX 0.001kwh
    uint32_t		Fir_CF_CNT;			//上一次的CF_CNT寄存器读数
    uint32_t		Mid_CF_CNT;			//电能累积换算的中间变量
    uint32_t		Power_Value;		//unit:	0.1W	XXXX.X
    uint32_t		Current_Value;		//unit:	0.001A	XX.XXX
    uint32_t		Voltage_Value;		//unit: 0.001V		XXX.X
    uint16_t		Freq;				//unit: 0.01Hz	XX.XX
    uint16_t		F_Current_Value;
} Elect_StructDef;

typedef struct
{
    uint32_t Current;//当前电流  单位：mA   传输值/1000为实际值
    uint32_t Voltage;//当前电压  单位：0.1V   传输值/10为实际值
    uint32_t Power;//当前功率（视在功率）单位： 0.1W  传输值/1000为实际值
    uint32_t Energy;//能量（视在能量） 单位： 0.01KW/h 传输值/100为实际值
    uint8_t Bl6526bState;//电量采集芯片的状态（过零、过流和过压三种状态） bit0：通讯故障  bit1：电压过压  bit2：电流过流 bit3:灯头异常
    uint8_t RelayState;//继电器状态 开/关 两种状态
	uint8_t LedPwm;		//pwm调光值
}CollectionData; //stm32f103采集板 主要数据结构体

extern Elect_StructDef BL0942_Elect[ELE_CIRCUIT_NUM];
extern CollectionData ElectricData[ELE_CIRCUIT_NUM];
extern uint32_t SaveEnergy[ELE_CIRCUIT_NUM];					//保存在flash中的已经产生的能量

void Delay_MS(uint16_t Count);
void Uart_Init(void);
void Delay_100uS(uint16_t Count);
void BL0942_Init(void);
bool BL0942_Uart1_R(uint8_t ICAddr,uint8_t cmd);
void BL0942_Uart1_W(uint8_t ICAddr,uint8_t cmd);
uint8_t BL0942_ReadEData(uint8_t channle, CollectionData *electricdata);
void BL0942_ConfigInit(uint8_t channle);
void BL0942_ReadEDataAll(uint8_t channle, CollectionData *electricdata);

void BL0942_REF_Set(uint8_t *buf,uint16_t len);
uint8_t BL0942_CheckSelf(uint8_t channle);

#endif


