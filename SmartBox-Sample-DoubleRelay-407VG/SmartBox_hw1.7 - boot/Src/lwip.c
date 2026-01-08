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
void ethernetif_notify_conn_changed(struct netif *netif)
{
    /* NOTE : This is function could be implemented in user file
              when the callback is needed,
    */
    if(netif_is_link_up(netif))
    {
        myprintf("net liink is up\r\n");

        netif_set_up(netif);
#if LWIP_DHCP
        {
            int err;
            uint8_t dhcp_cnt = 0;
            ipaddr.addr = 0;
            gnetif.ip_addr.addr = 0;
            err = dhcp_start(&gnetif);      //开启dhcp
            if(err == ERR_OK)
                printf("lwip dhcp init success...\n\n");
            else
                printf("lwip dhcp init fail...\n\n");
            while(ip_addr_cmp(&(gnetif.ip_addr),&ipaddr))   //等待dhcp分配的ip有效
            {
                dhcp_cnt++;
                if(dhcp_cnt/4 >= 10)	//10s
                {
                    dhcp_stop(&gnetif); 		//关闭DHCP
                    //使用静态IP地址
                    IP4_ADDR(&(gnetif.ip_addr),192,168,1,105);//默认远端IP为:192.168.1.100
                    IP4_ADDR(&(gnetif.netmask),255,255,255,0);//默认子网掩码:255.255.255.0
                    IP4_ADDR(&(gnetif.gw),192,168,1,1);//默认网关:192.168.1.1
                    myprintf("DHCP服务超时,使用静态IP地址!\r\n");
//                    myprintf("静态IP地址........................%d.%d.%d.%d\r\n",192,168,1,100);
//                    myprintf("子网掩码..........................%d.%d.%d.%d\r\n",255,255,255,0);
//                    myprintf("默认网关..........................%d.%d.%d.%d\r\n",192,168,1,1);
                    break;
                }
                osDelay(250);
            }
            myprintf("本地动态IP地址是:%d.%d.%d.%d\n\n",  \
                   ((gnetif.ip_addr.addr)&0x000000ff),       \
                   (((gnetif.ip_addr.addr)&0x0000ff00)>>8),  \
                   (((gnetif.ip_addr.addr)&0x00ff0000)>>16), \
                   ((gnetif.ip_addr.addr)&0xff000000)>>24);
            myprintf("本地子网掩码地址是:%d.%d.%d.%d\n\n",  \
                   ((gnetif.netmask.addr)&0x000000ff),       \
                   (((gnetif.netmask.addr)&0x0000ff00)>>8),  \
                   (((gnetif.netmask.addr)&0x00ff0000)>>16), \
                   ((gnetif.netmask.addr)&0xff000000)>>24);
            myprintf("本地默认网关地址是:%d.%d.%d.%d\n\n",  \
                   ((gnetif.gw.addr)&0x000000ff),       \
                   (((gnetif.gw.addr)&0x0000ff00)>>8),  \
                   (((gnetif.gw.addr)&0x00ff0000)>>16), \
                   ((gnetif.gw.addr)&0xff000000)>>24);
        }
#endif
		taskENTER_CRITICAL();
		ETH_Link_flag = 1;
		taskEXIT_CRITICAL();
    }
    else
    {
#if LWIP_DHCP
        dhcp_stop(&gnetif); 		//关闭DHCP
#endif
        myprintf("\r\n net liink is down");
        netif_set_link_down(netif);
    }
}

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
  dhcp_start(&gnetif);

/* USER CODE BEGIN 3 */
    {
#if LWIP_DHCP
        int err;
        uint8_t dhcp_cnt = 0;

        _myprintf("本程序使用DHCP动态分配IP地址\n");

        err = dhcp_start(&gnetif);      //开启dhcp
        if(err == ERR_OK)
            _myprintf("lwip dhcp init success...\n\n");
        else
            _myprintf("lwip dhcp init fail...\n\n");
        while(ip_addr_cmp(&(gnetif.ip_addr),&ipaddr))   //等待dhcp分配的ip有效
		{
			dhcp_cnt++;
			if(dhcp_cnt/4 >= 10)	//10s
			{
				dhcp_stop(&gnetif); 		//关闭DHCP
				//使用静态IP地址
				IP4_ADDR(&(gnetif.ip_addr),192,168,1,105);//默认远端IP为:192.168.1.100
				IP4_ADDR(&(gnetif.netmask),255,255,255,0);//默认子网掩码:255.255.255.0
				IP4_ADDR(&(gnetif.gw),192,168,1,1);//默认网关:192.168.1.1
				myprintf("DHCP服务超时,使用静态IP地址!\r\n");
				break;
			}
            vTaskDelay(250);
//			HAL_Delay(250);
        }
#endif
        _myprintf("本地IP地址是:%d.%d.%d.%d\n\n",  \
               ((gnetif.ip_addr.addr)&0x000000ff),       \
               (((gnetif.ip_addr.addr)&0x0000ff00)>>8),  \
               (((gnetif.ip_addr.addr)&0x00ff0000)>>16), \
               ((gnetif.ip_addr.addr)&0xff000000)>>24);
        _myprintf("本地子网掩码地址是:%d.%d.%d.%d\n\n",  \
               ((gnetif.netmask.addr)&0x000000ff),       \
               (((gnetif.netmask.addr)&0x0000ff00)>>8),  \
               (((gnetif.netmask.addr)&0x00ff0000)>>16), \
               ((gnetif.netmask.addr)&0xff000000)>>24);
        _myprintf("本地默认网关地址是:%d.%d.%d.%d\n\n",  \
               ((gnetif.gw.addr)&0x000000ff),       \
               (((gnetif.gw.addr)&0x0000ff00)>>8),  \
               (((gnetif.gw.addr)&0x00ff0000)>>16), \
               ((gnetif.gw.addr)&0xff000000)>>24);
    }
    netif_set_link_callback(&gnetif,ethernetif_update_config);
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
