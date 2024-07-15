/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "platform_stdlib.h"

/* common/api/wifi includes. */
#include "wifi_conf.h"
#include "wifi_simple_config.h"
extern enum sc_result simple_config_test(rtw_network_info_t *);
extern int init_test_data(char *custom_pin_code);
extern void deinit_test_data(void);
extern char simple_config_terminate;

/* acerpure includes */
#include "aws_acerpure.h"
AWS_OTA_STATE g_aws_ota = AWS_OTA_STATE_IDLE;
AWS_DAEMON_STATE g_aws_state = AWS_DAEMON_STATE_IDLE;
int is_ota_upgrade = 0;
char ota_url[255] = {0};

#include "acerpure_uart_atcmd/example_acerpure_uart_atcmd.h"
extern acerpure_device_state_t acerpure_device_state;
extern acerpure_device_monitor_t acerpure_device_monitor;
extern acerpure_wifi_test_t acerpure_wifi_test;
extern int device_state_report;
extern int g_start_wifi_pair;
extern int g_start_wifi_test;
ACERPURE_WIFI_STATE g_wifi_state = ACERPURE_WIFI_STATE_IDLE;

#include "flash_util.h"
extern struct user_wifi_conf g_wifi_conf;

#include "acer_startup.h"
#include "acer_mdns.h"

/*-----------------------------------------------------------*/

/**
 * @brief Application runtime entry point.
 */
int acer_startup_main( void )
{
    BaseType_t ret;
    /* Perform any hardware initialization that does not require the RTOS to be
     * running.  */
    prvMiscInitialization();

    /* Start the scheduler.  Initialization that requires the OS to be running,
     * including the Wi-Fi initialization, is performed in the RTOS daemon task
     * startup hook. */
    //vTaskStartScheduler();

    return 0;
}
/*-----------------------------------------------------------*/

static void prvMiscInitialization( void )
{
    /* FIX ME: Perform any hardware initializations, that don't require the RTOS to be
     * running, here.
     */
}
/*-----------------------------------------------------------*/

static void prvDaemonTask( void * pvParameters )
{
    vTaskDelay(1000);
    for (;;)
    {
        // protection! promise wifi pair will be execute!
        if (g_start_wifi_pair)
        {
            g_aws_state = AWS_DAEMON_STATE_PAIRING;
            g_start_wifi_pair = 0;
        }

        // protection! promise wifi test will be execute!
        if (g_start_wifi_test)
        {
            g_aws_state = AWS_DAEMON_STATE_TEST;
            g_start_wifi_test = 0;
        }

        switch (g_aws_state)
        {
            case AWS_DAEMON_STATE_IDLE:
            {
                configPRINTF( ( "Entry AWS_DAEMON_STATE_IDLE\r\n" ) );

                g_wifi_state = ACERPURE_WIFI_STATE_IDLE;

                if (0 == prvWifiConnect())
                {
                    g_aws_state = AWS_DAEMON_STATE_PAIRED;
                }
                else
                {
                    if (g_start_wifi_pair)
                    {
                        // start wifi pair ASAP
                        continue;
                    }
                    vTaskDelay(5000); // retry connect every 5 seconds
                }

                break;
            }

            case AWS_DAEMON_STATE_PAIRING:
            {
                // exit mqtt task if running
                // acerpure_mqtt_parser_exit = 1;
                // acerpure_mqtt_task_exit = 1;

                // while (acerpure_mqtt_parser_alive || acerpure_mqtt_task_alive)
                // {
                //     configPRINTF( ( "Wait MQTT task exit for pairing\r\n" ) );
                //     vTaskDelay(1000);
                // }

                configPRINTF( ( "Start WIFI and AWS IOT pairing\r\n" ) );
                g_wifi_state = ACERPURE_WIFI_STATE_PAIRING;

                // pairing with mobile APP
                // enter AP mode
                char *custom_pin_code = NULL;
                enum sc_result ret = SC_ERROR;
                simple_config_terminate = 0;

                if (init_test_data(custom_pin_code) == 0)
                {
                    ret = simple_config_test(NULL);
                    deinit_test_data();
                    print_simple_config_result(ret);
                }

                if (ret == SC_SUCCESS)
                {

                }
                else
                {
                    configPRINTF( ( "Connect to WIFI failed! enter IDLE state\r\n" ) );

                    g_aws_state = AWS_DAEMON_STATE_IDLE;
                }

                vTaskDelay(1000);

                break;
            }

            case AWS_DAEMON_STATE_PAIRED:
            {
                configPRINTF( ( "Entry AWS_DAEMON_STATE_PAIRED\r\n" ) );
                g_wifi_state = ACERPURE_WIFI_STATE_PAIRED;

                // acerpure_mqtt_parser_exit = 0;
                // acerpure_mqtt_task_exit = 0;
                // if (Acerpure_RunDaemon() == 0)
                if (acer_mdns() == 0 && Acerpure_RunDaemon() == 0)
                    g_aws_state = AWS_DAEMON_STATE_CONNECTED;
                else
                    g_aws_state = AWS_DAEMON_STATE_IDLE;

                vTaskDelay(1000);
                break;
            }

            case AWS_DAEMON_STATE_CONNECTED:
            {
                // AWS shadow connected, do nothing
                // if (is_ota_upgrade)
                // {
                //     // wait mqtt task exit
                //     acerpure_mqtt_parser_exit = 1;
                //     acerpure_mqtt_task_exit = 1;
                //     while (acerpure_mqtt_parser_alive || acerpure_mqtt_task_alive)
                //     {
                //         configPRINTF( ( "Wait MQTT task exit for OTA upgrading\r\n" ) );
                //         vTaskDelay(1000);
                //     }

                //     is_ota_upgrade = 0;
                //     OTAUpgrade(ota_url);
                // }

                // if (!acerpure_mqtt_parser_alive && !acerpure_mqtt_task_alive)
                // {
                //     configPRINTF( ( "acerpure mqtt task exit! return to idle state\r\n" ) );
                //     g_aws_state = AWS_DAEMON_STATE_IDLE;
                // }

                vTaskDelay(1000);

                break;
            }

            case AWS_DAEMON_STATE_TEST:
            {
                configPRINTF( ( "Start WIFI testing\r\n" ) );

                // change wifi state to idle for wifi testing
                g_wifi_state = ACERPURE_WIFI_STATE_IDLE;

                acerpure_wifi_state_t wifi_state;
                memset(&wifi_state, 0, sizeof(acerpure_wifi_state_t));
                wifi_state.connection = AWS_DAEMON_STATE_IDLE;
                // wifi_state.ota = 0; // dont care
                wifi_state.ota = AWS_OTA_STATE_IDLE; // dont care

                if (0 == prvConnectAP(acerpure_wifi_test.ssid, acerpure_wifi_test.password))
                {
                    wifi_state.connection = (unsigned char)ACERPURE_WIFI_STATE_PAIRED;
                    configPRINTF( ( "WIFI test successfully\r\n" ) );
                }
                else
                {
                    wifi_state.connection = AWS_DAEMON_STATE_IDLE;
                    configPRINTF( ( "WIFI test failed\r\n" ) );
                }

                acerpure_atcmd_send(ACERPURE_ATCMD_REPORT_WIFI_STATE,
                                    (unsigned char *)&wifi_state, sizeof(wifi_state));

                g_aws_state = AWS_DAEMON_STATE_IDLE;
                break;
            }
        }
    }

    configPRINTF( ( "Acer daemon task exit\r\n" ) );
    vTaskDelete(NULL);
}
/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void )
{
    configPRINTF( ( "Start Acer daemon task!\r\n") );
    eraseFastconnectData();
    #include "acer_init.h"
    strcpy(g_wifi_conf.setting.ssid, acerWIFI_SSID);
    strcpy(g_wifi_conf.setting.password, acerWIFI_PASSWORD);
    // g_wifi_conf.setting.security_type = RTW_SECURITY_WPA2_WPA3_MIXED;
    strcpy(acerpure_wifi_test.ssid, acerWIFI_SSID);
    strcpy(acerpure_wifi_test.password, acerWIFI_PASSWORD);
    g_start_wifi_test = 1;

    BaseType_t xReturned;
    xReturned = xTaskCreate( prvDaemonTask,               /* The function that implements the task. */
                             "DaemonTask",                           /* Human readable name for the task. */
                             configMINIMAL_STACK_SIZE * 4, /* Size of the stack to allocate for the task, in words not bytes! */
                             NULL,                                /* The task parameter is not used. */
                             tskIDLE_PRIORITY+5,                    /* Runs at the lowest priority. */
                             NULL );                 /* The handle is stored so the created task can be deleted again at the end of the demo. */
    configASSERT(xReturned == pdPASS)
}
/*-----------------------------------------------------------*/

int prvConnectAP(const char *ssid, const char *password)
{
    #include <lwip_netconf.h>
    if(wifi_connect(ssid, RTW_SECURITY_WPA2_WPA3_MIXED, password, strlen(ssid), strlen(password), -1, NULL) == RTW_SUCCESS)
		LwIP_DHCP(0, DHCP_START);
    else return -1;

    printf("\n\r[WLAN0] Show Wi-Fi information\n");
	rtw_wifi_setting_t setting;
	wifi_get_setting(WLAN0_NAME,&setting);
    if(wifi_get_setting(WLAN0_NAME,&setting) != -1) {
        setting.security_type = rltk_get_security_mode_full(WLAN0_NAME);
    }
	wifi_show_setting(WLAN0_NAME,&setting);
    // restore_wifi_info_to_flash();
    // SC_connect_to_candidate_AP(g_wifi_conf);
    return 0;
}

int prvWifiConnect( void )
{
    configPRINTF( ( "Wi-Fi information: ssid[%s], password[%s]\r\n",
                    g_wifi_conf.setting.ssid, g_wifi_conf.setting.password ) );

    return prvConnectAP((const char *)g_wifi_conf.setting.ssid, (const char *)g_wifi_conf.setting.password);
}
/*-----------------------------------------------------------*/

int eraseFastconnectData(void){
    #include "flash_api.h"
    #include "device_lock.h"
	flash_t flash;
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_erase_sector(&flash, FAST_RECONNECT_DATA);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	return 0;
}
