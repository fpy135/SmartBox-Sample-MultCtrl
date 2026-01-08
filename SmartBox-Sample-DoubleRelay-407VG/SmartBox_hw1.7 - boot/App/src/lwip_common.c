#include "lwip_common.h"
#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/mem.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

//lwip DHCP任务
//设置任务优先级
#define LWIP_DHCP_TASK_PRIO       		osPriorityNormal
//设置任务堆栈大小
#define LWIP_DHCP_STK_SIZE  		    128

osThreadId DHCPTaskHandle;

//删除DHCP任务
void lwip_comm_dhcp_delete(void)
{
//    dhcp_stop(&gnetif); 		//关闭DHCP
    osThreadTerminate(DHCPTaskHandle);	//删除DHCP任务
}

void LwipDHCPTask()
{
//	ethernetif_set_link();
}

void lwip_comm_creat(void)
{
    osThreadDef(dhcpTask, LwipDHCPTask, LWIP_DHCP_TASK_PRIO, NULL, LWIP_DHCP_STK_SIZE);
    DHCPTaskHandle = osThreadCreate(osThread(dhcpTask), NULL);
}

