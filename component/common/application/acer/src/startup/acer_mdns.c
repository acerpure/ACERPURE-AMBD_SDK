#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>

#include <mDNS/mDNS.h>

#include <lwip_netconf.h>
#include <lwip/netif.h>
extern struct netif xnetif[];

static void acer_mdns_thread(void *param)
{
	/* To avoid gcc warnings */
	( void ) param;
	
	DNSServiceRef dnsServiceRef = NULL;
	TXTRecordRef txtRecord;
	unsigned char txt_buf[100];	// use fixed buffer for text record to prevent malloc/free

	// Delay to wait for IP by DHCP
	vTaskDelay(5000);

	printf("\nmDNS Init\n");
	if(mDNSResponderInit() == 0) {
		printf("mDNS Register service: ameba.local\n");
		TXTRecordCreate(&txtRecord, sizeof(txt_buf), txt_buf);
		// TXTRecordSetValue(&txtRecord, "text1", strlen("text1_content"), "text1_content");
		// TXTRecordSetValue(&txtRecord, "text2", strlen("text2_content"), "text2_content");
		dnsServiceRef = mDNSRegisterService("ameba", "_service1._tcp", "local", 5000, &txtRecord);
		TXTRecordDeallocate(&txtRecord);

		// if(dnsServiceRef)
		// 	mDNSDeregisterService(dnsServiceRef);
		// printf("mDNS Deinit\n");
		// mDNSResponderDeinit();
	}

	vTaskDelete(NULL);
}

//===TEST
#include "timers.h"
TaskHandle_t xTargetTaskHandle = NULL;
void vTargetTask(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		xTaskCreate(acer_mdns_thread, ((const char*)"acer_mdns_thread_5"), 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
    }
}
void vTimerCallback(TimerHandle_t xTimer)
{
    static int wakeCount = 0;
    wakeCount++;
    if (wakeCount < 5) xTaskNotifyGive(xTargetTaskHandle);
    else xTimerStop(xTimer, 0);
}
void createAndStartTimer(void)
{
    TimerHandle_t xTimer = xTimerCreate("WakeUpTimer", pdMS_TO_TICKS(20000), pdTRUE, (void *)0, vTimerCallback);
    if (xTimer != NULL) xTimerStart(xTimer, 0);
}
//===TEST

int acer_mdns(void)
{
	if(xTaskCreate(acer_mdns_thread, ((const char*)"acer_mdns_thread"), 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\n\r%s xTaskCreate(init_thread) failed", __FUNCTION__);
		return -1;
	}
	// if(xTaskCreate(vTargetTask, "TargetTask", 1024, NULL, tskIDLE_PRIORITY+9, &xTargetTaskHandle) != pdPASS) {
	// 	printf("\n\r%s xTaskCreate(init_thread) failed", __FUNCTION__);
	// 	return -1;
	// }
	// else
    // 	createAndStartTimer();
	return 0;
}
