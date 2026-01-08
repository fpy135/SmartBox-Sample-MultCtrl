/**
 ******************************************************************************
  * File Name          : LWIP.c
  * Description        : This file provides initialization code for LWIP
  *                      middleWare.
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
  
/* Includes ------------------------------------------------------------------*/
#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#if defined ( __CC_ARM )  /* MDK ARM Compiler */
#include "lwip/sio.h"
#endif /* MDK ARM Compiler */
#include "ethernetif.h"

/* USER CODE BEGIN 0 */
#include "usart.h"
#include "lwip/dns.h"
#include "SysConfig.h"
#include "socketserver.h"

/* USER CODE END 0 */
/* Private function prototypes -----------------------------------------------*/
/* ETH Variables initialization ----------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN 1 */
uint8_t ETH_Link_flag = 0;
/* USER CODE END 1 */
/* Semaphore to signal Ethernet Link state update */
osSemaphoreId Netif_LinkSemaphore = NULL;
/* Ethernet link thread Argument */
struct link_str link_arg;

/* Variables Initialization */
struct netif gnetif;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;

/* USER CODE BEGIN 2 */
/***********ip地址等网络打印***************/
void netPara_printf(void)
{
	myprintf("\r\n IP:%d.%d.%d.%d",  \
			   ((gnetif.ip_addr.addr)&0x000000ff),       \
			   (((gnetif.ip_addr.addr)&0x0000ff00)>>8),  \
			   (((gnetif.ip_addr.addr)&0x00ff0000)>>16), \
			   ((gnetif.ip_addr.addr)&0xff000000)>>24);
	myprintf("\r\n Netmask:%d.%d.%d.%d",  \
		   ((gnetif.netmask.addr)&0x000000ff),       \
		   (((gnetif.netmask.addr)&0x0000ff00)>>8),  \
		   (((gnetif.netmask.addr)&0x00ff0000)>>16), \
		   ((gnetif.netmask.addr)&0xff000000)>>24);
	myprintf("\r\n Gateway:%d.%d.%d.%d",  \
		   ((gnetif.gw.addr)&0x000000ff),       \
		   (((gnetif.gw.addr)&0x0000ff00)>>8),  \
		   (((gnetif.gw.addr)&0x00ff0000)>>16), \
		   ((gnetif.gw.addr)&0xff000000)>>24);
}

void ethernetif_notify_conn_changed(struct netif *netif)
{
    /* NOTE : This is function could be implemented in user file
              when the callback is needed,
    */
    if(netif_is_link_up(netif))
    {
        myprintf("\r\nnet link is up");
//	low_level_init(netif);
		/*
		{
			uint8_t * p;
			p = (uint8_t *)&DeviceIP;
			IP4_ADDR(&ipaddr,*p,*(p+1),*(p+2),*(p+3));//默认远端IP
			p = (uint8_t *)&DeviceNetmask;
			IP4_ADDR(&netmask,*p,*(p+1),*(p+2),*(p+3));//默认子网掩码
			p = (uint8_t *)&DeviceGw;
			IP4_ADDR(&gw,*p,*(p+1),*(p+2),*(p+3));//默认网关
			_myprintf("\r\n本程序使用静态IP地址");
		}
		netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input); */
        netif_set_up(netif);
#if LWIP_DHCP
		if(DHCP_Enable_Flag == 0x01)
        {
            int err;
            uint8_t dhcp_cnt = 0;
            ipaddr.addr = 0;
            gnetif.ip_addr.addr = 0;
            err = dhcp_start(&gnetif);      //开启dhcp
            if(err == ERR_OK)
                myprintf("\r\nlwip dhcp init success...");
            else
                myprintf("\r\nlwip dhcp init fail...");
            while(ip_addr_cmp(&(gnetif.ip_addr),&ipaddr))   //等待dhcp分配的ip有效
            {
                dhcp_cnt++;
                if(dhcp_cnt/4 >= 10)	//10s
                {
                    dhcp_stop(&gnetif); 		//关闭DHCP
                    //使用静态IP地址
					{
						uint8_t * p;
						p = (uint8_t *)&DeviceIP;
						IP4_ADDR(&(gnetif.ip_addr),*p,*(p+1),*(p+2),*(p+3));//默认远端IP
						p = (uint8_t *)&DeviceNetmask;
						IP4_ADDR(&(gnetif.netmask),*p,*(p+1),*(p+2),*(p+3));//默认子网掩码
						p = (uint8_t *)&DeviceGw;
						IP4_ADDR(&(gnetif.gw),*p,*(p+1),*(p+2),*(p+3));//默认网关
						myprintf("\r\nDHCP服务超时,使用静态IP地址!");
					}
                    break;
                }
                osDelay(250);
            }
        }
#endif
		netPara_printf();
		taskENTER_CRITICAL();
		ETH_Link_flag = 1;
		taskEXIT_CRITICAL();
    }
    else
    {
#if LWIP_DHCP
		if(DHCP_Enable_Flag == 0x01)
		{
			dhcp_stop(&gnetif); 		//关闭DHCP
		}
#endif
        myprintf("\r\n net link is down");
		{
			uint8_t netsta;
			netsta = ConnectOffLine;
			TcpConnectFlag_RW(1,&netsta);
		}
		taskENTER_CRITICAL();
		ETH_Link_flag = 0;
		taskEXIT_CRITICAL();
		LAN8720_Init();
//			low_level_init(netif);
//			netif->flags &= (~NETIF_FLAG_LINK_UP);
/*		netif_remove(netif); */
		vTaskDelay(100);
        netif_set_link_down(netif);
    }
}

#if LWIP_DHCP

xTaskHandle DHCPTaskHandle;

//删除DHCP任务
void lwip_dhcp_delete(void)
{
//    dhcp_stop(&gnetif); 		//关闭DHCP
    vTaskDelete(DHCPTaskHandle);	//删除DHCP任务
}

//DHCP处理任务
void lwip_dhcp_task(void *pdata)
{
	int err;
	uint8_t dhcp_cnt = 0;
	uint32_t ip = 0;

	myprintf("\r\n本程序使用DHCP动态分配IP地址");

	err = dhcp_start(&gnetif);      //开启dhcp
	if(err == ERR_OK)
		myprintf("\r\nlwip dhcp init success...");
	else
		myprintf("\r\nlwip dhcp init fail...");
	myprintf("\r\n正在查找DHCP服务器,请稍等...........");   
	while(1)   //等待dhcp分配的ip有效
	{
		myprintf("\r\n正在获取地址...");
		ip = gnetif.ip_addr.addr;
		if(ip != 0)
		{
			myprintf("\r\n获取到ip地址...");
			break;
		}
		dhcp_cnt++;
		if(dhcp_cnt/4 >= 30)	//30s
		{
			myprintf("\r\nDHCP服务超时,使用静态IP地址!");
			//使用静态IP地址
			{
				uint8_t * p;
				p = (uint8_t *)&DeviceIP;
				IP4_ADDR(&(gnetif.ip_addr),*p,*(p+1),*(p+2),*(p+3));//默认远端IP
				p = (uint8_t *)&DeviceNetmask;
				IP4_ADDR(&(gnetif.netmask),*p,*(p+1),*(p+2),*(p+3));//默认子网掩码
				p = (uint8_t *)&DeviceGw;
				IP4_ADDR(&(gnetif.gw),*p,*(p+1),*(p+2),*(p+3));//默认网关
			}
			break;
		}
		osDelay(250);
	}
	netPara_printf();
	{
		uint32_t regvalue = 0;
		taskENTER_CRITICAL();
		HAL_ETH_ReadPHYRegister(&heth, PHY_BSR, &regvalue);
		if((regvalue & PHY_LINKED_STATUS) == PHY_LINKED_STATUS)
			ETH_Link_flag = 1;
		else
			ETH_Link_flag = 0;
		taskEXIT_CRITICAL();
	}
	lwip_dhcp_delete();	//删除DHCP任务 
}

void lwip_dhcp_creat(void)
{
    BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */

	myprintf("\r\n Creat LwipDHCP_Task");

    xReturn = xTaskCreate(lwip_dhcp_task, "DHCP", DHCP_StkSIZE, NULL, DHCP_TaskPrio, &DHCPTaskHandle);
    if(pdPASS != xReturn)
    {
        myprintf("\r\n LwipDHCP_Task creat error");
        while(1);
    }
}

#endif 


/* USER CODE END 2 */

/**
  * LwIP initialization function
  */
void MX_LWIP_Init(void)
{
  /* Initilialize the LwIP stack with RTOS */
  tcpip_init( NULL, NULL );

  /* IP addresses initialization with DHCP (IPv4) */
  ipaddr.addr = 0;
  netmask.addr = 0;
  gw.addr = 0;

  /* add the network interface (IPv4/IPv6) with RTOS */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  /* Registers the default network interface */
  netif_set_default(&gnetif);

  if (netif_is_link_up(&gnetif))
  {
    /* When the netif is fully configured this function must be called */
    netif_set_up(&gnetif);
  }
  else
  {
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
  }

  /* Set the link callback function, this function is called on change of link status*/
  netif_set_link_callback(&gnetif, ethernetif_update_config);

  /* create a binary semaphore used for informing ethernetif of frame reception */
  osSemaphoreDef(Netif_SEM);
  Netif_LinkSemaphore = osSemaphoreCreate(osSemaphore(Netif_SEM) , 1 );

  link_arg.netif = &gnetif;
  link_arg.semaphore = Netif_LinkSemaphore;
  /* Create the Ethernet link handler thread */
/* USER CODE BEGIN OS_THREAD_DEF_CREATE_CMSIS_RTOS_V1 */
  osThreadDef(LinkThr, ethernetif_set_link, osPriorityBelowNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate (osThread(LinkThr), &link_arg);
/* USER CODE END OS_THREAD_DEF_CREATE_CMSIS_RTOS_V1 */

  /* Start DHCP negotiation for a network interface (IPv4) */
//  dhcp_start(&gnetif);

/* USER CODE BEGIN 3 */
    {
#if LWIP_DHCP
		if(DHCP_Enable_Flag == 0x01)
		{
			lwip_dhcp_creat();
		}
		else
		{
			uint8_t * p;
			p = (uint8_t *)&DeviceIP;
			IP4_ADDR(&(gnetif.ip_addr),*p,*(p+1),*(p+2),*(p+3));//默认远端IP
			p = (uint8_t *)&DeviceNetmask;
			IP4_ADDR(&(gnetif.netmask),*p,*(p+1),*(p+2),*(p+3));//默认子网掩码
			p = (uint8_t *)&DeviceGw;
			IP4_ADDR(&(gnetif.gw),*p,*(p+1),*(p+2),*(p+3));//默认网关
			myprintf("\r\n本程序使用静态IP地址");

			{
				uint32_t regvalue = 0;
				taskENTER_CRITICAL();
				HAL_ETH_ReadPHYRegister(&heth, PHY_BSR, &regvalue);
				if((regvalue & PHY_LINKED_STATUS) == PHY_LINKED_STATUS)
					ETH_Link_flag = 1;
				else
					ETH_Link_flag = 0;
				taskEXIT_CRITICAL();
			}
		}
//        int err;
//        uint8_t dhcp_cnt = 0;

//        _myprintf("\r\n本程序使用DHCP动态分配IP地址");

////        err = dhcp_start(&gnetif);      //开启dhcp
//        if(err == ERR_OK)
//            _myprintf("\r\nlwip dhcp init success...");
//        else
//            _myprintf("\r\nlwip dhcp init fail...");
//        while(ip_addr_cmp(&(gnetif.ip_addr),&ipaddr))   //等待dhcp分配的ip有效
//		{
//			dhcp_cnt++;
//			if(dhcp_cnt/4 >= 30)	//10s
//			{
//				dhcp_stop(&gnetif); 		//关闭DHCP
//				//使用静态IP地址
//				IP4_ADDR(&(gnetif.ip_addr),192,168,1,105);//默认远端IP为:192.168.1.100
//				IP4_ADDR(&(gnetif.netmask),255,255,255,0);//默认子网掩码:255.255.255.0
//				IP4_ADDR(&(gnetif.gw),192,168,1,1);//默认网关:192.168.1.1
//				myprintf("\r\nDHCP服务超时,使用静态IP地址!");
//				break;
//			}
//            osDelay(250);
////			HAL_Delay(250);
//        }
#else
		{
			uint8_t * p;
			p = (uint8_t *)&DeviceIP;
			IP4_ADDR(&(gnetif.ip_addr),*p,*(p+1),*(p+2),*(p+3));//默认远端IP
			p = (uint8_t *)&DeviceNetmask;
			IP4_ADDR(&(gnetif.netmask),*p,*(p+1),*(p+2),*(p+3));//默认子网掩码
			p = (uint8_t *)&DeviceGw;
			IP4_ADDR(&(gnetif.gw),*p,*(p+1),*(p+2),*(p+3));//默认网关
			myprintf("\r\n本程序使用静态IP地址");

			{
				uint32_t regvalue = 0;
				taskENTER_CRITICAL();
				HAL_ETH_ReadPHYRegister(&heth, PHY_BSR, &regvalue);
				if((regvalue & PHY_LINKED_STATUS) == PHY_LINKED_STATUS)
					ETH_Link_flag = 1;
				else
					ETH_Link_flag = 0;
				taskEXIT_CRITICAL();
			}
		}
#endif
        netPara_printf();
    }
/* USER CODE END 3 */
}

#ifdef USE_OBSOLETE_USER_CODE_SECTION_4
/* Kept to help code migration. (See new 4_1, 4_2... sections) */
/* Avoid to use this user section which will become obsolete. */
/* USER CODE BEGIN 4 */
/* USER CODE END 4 */
#endif

#if defined ( __CC_ARM )  /* MDK ARM Compiler */
/**
 * Opens a serial device for communication.
 *
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
sio_fd_t sio_open(u8_t devnum)
{
  sio_fd_t sd;

/* USER CODE BEGIN 7 */
  sd = 0; // dummy code
/* USER CODE END 7 */
	
  return sd;
}

/**
 * Sends a single character to the serial device.
 *
 * @param c character to send
 * @param fd serial device handle
 *
 * @note This function will block until the character can be sent.
 */
void sio_send(u8_t c, sio_fd_t fd)
{
/* USER CODE BEGIN 8 */
/* USER CODE END 8 */
}

/**
 * Reads from the serial device.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
 *
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */
u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

/* USER CODE BEGIN 9 */
  recved_bytes = 0; // dummy code
/* USER CODE END 9 */	
  return recved_bytes;
}

/**
 * Tries to read from the serial device. Same as sio_read but returns
 * immediately if no data is available and never blocks.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received
 */
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

/* USER CODE BEGIN 10 */
  recved_bytes = 0; // dummy code
/* USER CODE END 10 */	
  return recved_bytes;
}
#endif /* MDK ARM Compiler */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
