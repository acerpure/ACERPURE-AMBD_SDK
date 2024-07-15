/* Standard includes. */
#include "string.h"
#include "stdio.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

// /* Demo includes. */
// #include "aws_demo_config.h"
#include "aws_application_version.h"
#include "aws_acerpure.h"

#include "flash_util.h"
extern struct user_aws_mqtt_conf g_aws_mqtt_conf;
extern void vStartAcerpureAWSDaemon( void );

#define AWS_MAX_DATA_LENGTH    1024

int acerpure_mqtt_task_alive = 0;
int acerpure_mqtt_parser_alive = 0;
int acerpure_mqtt_task_exit = 0;
int acerpure_mqtt_parser_exit = 0;
// int device_state_report = 0;
// int device_monitor_report = 0;
extern device_state_report = 0;
extern device_monitor_report = 0;
extern uint32_t acerpure_mqtt_timer;
extern struct user_aws_device_info g_aws_device_info;
extern struct user_aws_alert_conf g_aws_alert_conf;
#define MIN_ALERT_INTERVAL 10

#include <cJSON.h>
#define malloc      pvPortMalloc
#define free        vPortFree

extern char ota_url[255];
extern AWS_OTA_STATE g_aws_ota;
extern int is_ota_upgrade;
extern int g_need_update_version;
extern int g_need_update_mainboard_info;
// extern void OTAUpgrade(char *url);

#include "acerpure_uart_atcmd/example_acerpure_uart_atcmd.h"
// #include "example_acerpure_uart_atcmd.h"
extern acerpure_device_info_t acerpure_device_info;
extern acerpure_device_state_t acerpure_device_state;
extern acerpure_device_monitor_t acerpure_device_monitor;
extern int new_monitor_format;
extern int g_extended_model;
uint8_t aws_device_alert = 0;
int is_alert_interval_update = 0;
extern ACERPURE_WIFI_STATE g_wifi_state;

// value mapping
static const char * const OTAState[3] = { "Stop", "Upgrading", "Error" };
static const char * const Power[2] = { "Off", "On" };
static const char * const AirPurifierSpeed[5] = { "Smart", "1", "2", "3", "Turbo" };
static const char * const AirCirculatorSpeed[12] = { "Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "N/A" };
static const char * const AirPurifierRotate[2] = { "Off", "On" };
static const char * const AirCirculatorRotate[2] = { "Off", "On" };
static const char * const AirDetectMode[3] = { "Off", "PM2.5", "PM1.0" };
static const char * const KidMode[2] = { "Off", "On" };
static const char * const SleepMode[2] = { "Off", "On" };
static const char * const ShutdownTimer[14] = { "Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "0.5" };
static const char * const FilterState[2] = { "Un-installed", "Installed" };
static const char * const PMGASLevel[4] = { "N/A", "Good", "Normal", "Bad" };
static const char * const FilterLevel[2] = { "Off", "On" };
static const char * const MyFavorIcon[2] = { "Off", "On" };
static const char * const UV[3] = { "Off", "On", "N/A" };
static const char * const AlertNotification[8] = { "FilterEndOfLifeAlert",      \
                                                   "FilterEndOfLifeAlert5",     \
                                                   "FilterEndOfLifeAlert10",    \
                                                   "FilterEndOfLifeAlert20",    \
                                                   "FilterInstallFaultAlert",   \
                                                   "AQIWarnAlert",              \
                                                   "GasWarnAlert",              \
                                                   "CO2WarnAlert" };

uint32_t filter_health_alert_timer = 0;
uint32_t filter_health_5_alert_timer = 0;
uint32_t filter_health_10_alert_timer = 0;
uint32_t filter_health_20_alert_timer = 0;
uint32_t filter_install_alert_timer = 0;
uint32_t aqi_alert_timer = 0;
uint32_t gas_alert_timer = 0;
uint32_t co2_alert_timer = 0;

// uint8_t alert_test = 0;
// uint8_t alert_test = 99;

// send save-monitor message timer, send once per 10 mins
uint32_t monitor_timer = 0;
#define MONITOR_PERIOD 600   // 600 seconds

#define commfwREPORT_FORMAT             \
    "{"                                 \
    "\"type\":\"set-device\","          \
    "\"field\":\"commfw\","             \
    "\"value\":\"v%d.%d.%d\""           \
    "}"

#define mainfwREPORT_FORMAT             \
    "{"                                 \
    "\"type\":\"set-device\","          \
    "\"field\":\"mainfw\","             \
    "\"value\":\"%s\""                  \
    "}"

#define deviceStateREPORT_FORMAT        \
    "{"                                 \
    "\"type\":\"save-state\","          \
    "\"data\":{"                        \
    "\"%s\":{"                          \
    "\"Model\":\"%s\","                 \
    "\"MainFWVersion\":\"%s\","         \
    "\"ModuleFWVersion\":\"%s\","       \
    "\"OTAUrl\":\"%s\","                \
    "\"OTAState\":\"%s\","              \
    "\"Power\":\"%s\","                 \
    "\"AirPurifierSpeed\":\"%s\","      \
    "\"AirCirculatorSpeed\":\"%s\","    \
    "\"AirPurifierRotate\":\"%s\","     \
    "\"AirCirculatorRotate\":\"%s\","   \
    "\"AirDetectMode\":\"%s\","         \
    "\"KidMode\":\"%s\","               \
    "\"SleepMode\":\"%s\","             \
    "\"ShutdownTimer\":\"%s\","         \
    "\"FilterState\":\"%s\","           \
    "\"FilterHealth\":\"%d\","          \
    "\"FilterIndicator\":\"%s\","       \
    "\"UV\":\"%s\","                    \
    "\"PM2.5State\":\"%s\","            \
    "\"PM1.0State\":\"%s\","            \
    "\"GASState\":\"%s\","              \
    "\"CO2State\":\"%s\","              \
    "\"MyFavorIcon\":\"%s\","           \
    "\"PM2.5\":\"%d\","                 \
    "\"PM1.0\":\"%d\","                 \
    "\"GAS\":\"%d\","                   \
    "\"CO2\":\"%d\","                   \
    "\"FilterHealthAlertInterval\":\"%lu\","     \
    "\"FilterHealth5AlertInterval\":\"%lu\","    \
    "\"FilterHealth10AlertInterval\":\"%lu\","   \
    "\"FilterHealth20AlertInterval\":\"%lu\","   \
    "\"FilterInstallAlertInterval\":\"%lu\","    \
    "\"AQIAlertInterval\":\"%lu\","              \
    "\"GASAlertInterval\":\"%lu\","              \
    "\"CO2AlertInterval\":\"%lu\""               \
    "}"                                 \
    "}"                                 \
    "}"

#define monitorValueREPORT_FORMAT       \
    "{"                                 \
    "\"type\":\"save-monitor\","        \
    "\"data\":{"                        \
    "\"PM2.5\":\"%d\","                 \
    "\"PM1.0\":\"%d\","                 \
    "\"GAS\":\"%d\","                    \
    "\"CO2\":\"%d\""                    \
    "}"                                 \
    "}"

#define alertNotificationREPORT_FORMAT  \
    "{"                                 \
    "\"type\":\"alert\","               \
    "\"alert\":\"%s\""                  \
    "}"

/*
接收自遠端的命令
*/
static void prvAWSActionParser(char *received_json, int length)
{
    cJSON_Hooks memoryHook;

    memoryHook.malloc_fn = malloc;
    memoryHook.free_fn = free;
    cJSON_InitHooks(&memoryHook);

    int is_multiple_actions = 0;
    acerpuer_action_t action;
    memset(&action, 0, sizeof(action));
    unsigned char multiple_actions[((ACERPURE_ACTION_MAX - 1) * 2)];
    memset(multiple_actions, 0, sizeof(multiple_actions));

    int update_flash_alert_interval = 0;
    cJSON *parsed_json = cJSON_Parse(received_json);
    if (parsed_json)
    {
        cJSON *acerpure_json = cJSON_GetObjectItem(parsed_json, "Acerpure-0");
        if (acerpure_json)
        {
            // check alert configuration
            cJSON *alert_interval = cJSON_GetObjectItem(acerpure_json, "FilterHealthAlertInterval");
            if (alert_interval)
            {
                g_aws_alert_conf.filter_health = atoi(alert_interval->valuestring);
                update_flash_alert_interval = 1;
            }
            alert_interval = cJSON_GetObjectItem(acerpure_json, "FilterHealth5AlertInterval");
            if (alert_interval)
            {
                g_aws_alert_conf.filter_health_5 = atoi(alert_interval->valuestring);
                update_flash_alert_interval = 1;
            }
            alert_interval = cJSON_GetObjectItem(acerpure_json, "FilterHealth10AlertInterval");
            if (alert_interval)
            {
                g_aws_alert_conf.filter_health_10 = atoi(alert_interval->valuestring);
                update_flash_alert_interval = 1;
            }
            alert_interval = cJSON_GetObjectItem(acerpure_json, "FilterHealth20AlertInterval");
            if (alert_interval)
            {
                g_aws_alert_conf.filter_health_20 = atoi(alert_interval->valuestring);
                update_flash_alert_interval = 1;
            }
            alert_interval = cJSON_GetObjectItem(acerpure_json, "FilterInstallAlertInterval");
            if (alert_interval)
            {
                g_aws_alert_conf.filter_install = atoi(alert_interval->valuestring);
                update_flash_alert_interval = 1;
            }
            alert_interval = cJSON_GetObjectItem(acerpure_json, "AQIAlertInterval");
            if (alert_interval)
            {
                g_aws_alert_conf.aqi = atoi(alert_interval->valuestring);
                update_flash_alert_interval = 1;
            }
            alert_interval = cJSON_GetObjectItem(acerpure_json, "GASAlertInterval");
            if (alert_interval)
            {
                g_aws_alert_conf.gas = atoi(alert_interval->valuestring);
                update_flash_alert_interval = 1;
            }
            alert_interval = cJSON_GetObjectItem(acerpure_json, "CO2AlertInterval");
            if (alert_interval)
            {
                g_aws_alert_conf.co2 = atoi(alert_interval->valuestring);
                update_flash_alert_interval = 1;
            }

            if (update_flash_alert_interval)
            {
                update_flash_alert_interval = 0;

                // protection
                if (g_aws_alert_conf.filter_health < MIN_ALERT_INTERVAL)
                {
                    g_aws_alert_conf.filter_health = MIN_ALERT_INTERVAL;
                }
                if (g_aws_alert_conf.filter_health_5 < MIN_ALERT_INTERVAL)
                {
                    g_aws_alert_conf.filter_health_5 = MIN_ALERT_INTERVAL;
                }
                if (g_aws_alert_conf.filter_health_10 < MIN_ALERT_INTERVAL)
                {
                    g_aws_alert_conf.filter_health_10 = MIN_ALERT_INTERVAL;
                }
                if (g_aws_alert_conf.filter_health_20 < MIN_ALERT_INTERVAL)
                {
                    g_aws_alert_conf.filter_health_20 = MIN_ALERT_INTERVAL;
                }
                if (g_aws_alert_conf.filter_install < MIN_ALERT_INTERVAL)
                {
                    g_aws_alert_conf.filter_install = MIN_ALERT_INTERVAL;
                }
                if (g_aws_alert_conf.aqi < MIN_ALERT_INTERVAL)
                {
                    g_aws_alert_conf.aqi = MIN_ALERT_INTERVAL;
                }
                if (g_aws_alert_conf.gas < MIN_ALERT_INTERVAL)
                {
                    g_aws_alert_conf.gas = MIN_ALERT_INTERVAL;
                }
                if (g_aws_alert_conf.co2 < MIN_ALERT_INTERVAL)
                {
                    g_aws_alert_conf.co2 = MIN_ALERT_INTERVAL;
                }
                // user_data_partition_ops(USER_PARTITION_AWS_ALERT, USER_PARTITION_WRITE,
                //             (u8 *)&g_aws_alert_conf, sizeof(struct user_aws_alert_conf));

                is_alert_interval_update = 1; // notify using new alert interval
            }

            // ota
            cJSON *ota_url_json = cJSON_GetObjectItem(acerpure_json, "OTAUrl");
            cJSON *ota_state_json = cJSON_GetObjectItem(acerpure_json, "OTAState");
            if (ota_url_json && ota_state_json)
            {
                if (!strcmp(ota_state_json->valuestring, "Start"))
                {
                    strncpy(ota_url, ota_url_json->valuestring, sizeof(ota_url));
                    cJSON_Delete(parsed_json);
                    // OTAUpgrade(ota_url);
                    is_ota_upgrade = 1;
                    return;
                }
            }

            // check if multiplee actions first
            // MyFavorIcon is old method, keep it for compatible
            // use PkType instead
            cJSON *action_json = cJSON_GetObjectItem(acerpure_json, "MyFavorIcon");
            if (action_json)
            {
                is_multiple_actions = 1;
                memset(&action, 0, sizeof(action));
                action.action = ACERPURE_ACTION_MY_FAVORITE;
                if (!strcmp(action_json->valuestring, "On"))
                {
                    action.param = 0x01;
                    configPRINTF( ( "acerpure my favorite actions received !!!\r\n" ) );
                }
                else
                {
                    action.param = 0x00;
                    configPRINTF( ( "acerpure schedule actions received !!!\r\n" ) );
                }

                // prvAddMultipleActions(&action, multiple_actions);
            }

            // new method! it will override the previous setting if exists
            action_json = cJSON_GetObjectItem(acerpure_json, "PkType");
            if (action_json)
            {
                is_multiple_actions = 1;
                memset(&action, 0, sizeof(action));
                action.action = ACERPURE_ACTION_MY_FAVORITE;
                if (!strcmp(action_json->valuestring, "MyFavor"))
                {
                    action.param = 0x01;
                    configPRINTF( ( "acerpure my favorite actions received !!!\r\n" ) );
                }
                else
                {
                    action.param = 0x00;
                    configPRINTF( ( "acerpure schedule actions received !!!\r\n" ) );
                }

                // prvAddMultipleActions(&action, multiple_actions);
            }

            // power
            action_json = cJSON_GetObjectItem(acerpure_json, "Power");
            if (action_json)
            {
                memset(&action, 0, sizeof(action));
                action.action = ACERPURE_ACTION_POWER;
                if (!strcmp(action_json->valuestring, "On"))
                {
                    action.param = 0x01;
                }
                else if (!strcmp(action_json->valuestring, "Off"))
                {
                    action.param = 0x00;
                }

                if (is_multiple_actions)
                {
                    // prvAddMultipleActions(&action, multiple_actions);
                    configPRINTF( ( "prvAddMultipleActions !!!\r\n" ) );
                }
                else
                {
                    acerpure_atcmd_send(ACERPURE_ATCMD_DEVICE_ACTION,
                                    (u8 *)&action, sizeof(action));

                    configPRINTF( ( "acerpure action [Power -> %d] param[%s -> %d]\r\n",
                                    ACERPURE_ACTION_POWER, action_json->valuestring, action.param ) );
                }
            }

            // air speed
            action_json = cJSON_GetObjectItem(acerpure_json, "AirPurifierSpeed");
            if (action_json)
            {
                memset(&action, 0, sizeof(action));
                action.action = ACERPURE_ACTION_AIR_PUR_SPEED;
                if (!strcmp(action_json->valuestring, "Smart"))
                {
                    action.param = 0x00;
                }
                else if (!strcmp(action_json->valuestring, "1"))
                {
                    action.param = 0x01;
                }
                else if (!strcmp(action_json->valuestring, "2"))
                {
                    action.param = 0x02;
                }
                else if (!strcmp(action_json->valuestring, "3"))
                {
                    action.param = 0x03;
                }
                else if (!strcmp(action_json->valuestring, "Turbo"))
                {
                    action.param = 0x04;
                }

                if (is_multiple_actions)
                {
                    // prvAddMultipleActions(&action, multiple_actions);
                    configPRINTF( ( "prvAddMultipleActions !!!\r\n" ) );
                }
                else
                {
                    acerpure_atcmd_send(ACERPURE_ATCMD_DEVICE_ACTION,
                                        (u8 *)&action, sizeof(action));

                    configPRINTF( ( "acerpure action [AirPurifierSpeed -> %d] param[%s -> %d]\r\n",
                                    ACERPURE_ACTION_AIR_PUR_SPEED, action_json->valuestring, action.param ) );
                }
            }

            action_json = cJSON_GetObjectItem(acerpure_json, "AirCirculatorSpeed");
            if (action_json)
            {
                memset(&action, 0, sizeof(action));
                action.action = ACERPURE_ACTION_AIR_CIR_SPEED;
                if (!strcmp(action_json->valuestring, "Off"))
                {
                    action.param = 0x00;
                }
                else if (!strcmp(action_json->valuestring, "1"))
                {
                    action.param = 0x01;
                }
                else if (!strcmp(action_json->valuestring, "2"))
                {
                    action.param = 0x02;
                }
                else if (!strcmp(action_json->valuestring, "3"))
                {
                    action.param = 0x03;
                }
                else if (!strcmp(action_json->valuestring, "4"))
                {
                    action.param = 0x04;
                }
                else if (!strcmp(action_json->valuestring, "5"))
                {
                    action.param = 0x05;
                }
                else if (!strcmp(action_json->valuestring, "6"))
                {
                    action.param = 0x06;
                }
                else if (!strcmp(action_json->valuestring, "7"))
                {
                    action.param = 0x07;
                }
                else if (!strcmp(action_json->valuestring, "8"))
                {
                    action.param = 0x08;
                }
                else if (!strcmp(action_json->valuestring, "9"))
                {
                    action.param = 0x09;
                }
                else if (!strcmp(action_json->valuestring, "10"))
                {
                    action.param = 0x0a;
                }
                else if (!strcmp(action_json->valuestring, "N/A"))
                {
                    action.param = 0xff;
                }

                if (is_multiple_actions)
                {
                    // prvAddMultipleActions(&action, multiple_actions);
                    configPRINTF( ( "prvAddMultipleActions !!!\r\n" ) );
                }
                else
                {
                    acerpure_atcmd_send(ACERPURE_ATCMD_DEVICE_ACTION,
                                        (u8 *)&action, sizeof(action));

                    configPRINTF( ( "acerpure action [AirCirculatorSpeed -> %d] param[%s -> %d]\r\n",
                                    ACERPURE_ACTION_AIR_CIR_SPEED, action_json->valuestring, action.param ) );
                }
            }

            // air rotate
            action_json = cJSON_GetObjectItem(acerpure_json, "AirPurifierRotate");
            if (action_json)
            {
                memset(&action, 0, sizeof(action));
                action.action = ACERPURE_ACTION_AIR_PUR_ROTATE;
                if (!strcmp(action_json->valuestring, "On"))
                {
                    action.param = 0x01;
                }
                else if (!strcmp(action_json->valuestring, "Off"))
                {
                    action.param = 0x00;
                }

                if (is_multiple_actions)
                {
                    // prvAddMultipleActions(&action, multiple_actions);
                    configPRINTF( ( "prvAddMultipleActions !!!\r\n" ) );
                }
                else
                {
                    acerpure_atcmd_send(ACERPURE_ATCMD_DEVICE_ACTION,
                                        (u8 *)&action, sizeof(action));

                    configPRINTF( ( "acerpure action [AirPurifierRotate -> %d] param[%s -> %d]\r\n",
                                    ACERPURE_ACTION_AIR_PUR_ROTATE, action_json->valuestring, action.param ) );
                }
            }

            action_json = cJSON_GetObjectItem(acerpure_json, "AirCirculatorRotate");
            if (action_json)
            {
                memset(&action, 0, sizeof(action));
                action.action = ACERPURE_ACTION_AIR_CIR_ROTATE;
                if (!strcmp(action_json->valuestring, "On"))
                {
                    action.param = 0x01;
                }
                else if (!strcmp(action_json->valuestring, "Off"))
                {
                    action.param = 0x00;
                }

                if (is_multiple_actions)
                {
                    // prvAddMultipleActions(&action, multiple_actions);
                    configPRINTF( ( "prvAddMultipleActions !!!\r\n" ) );
                }
                else
                {
                    acerpure_atcmd_send(ACERPURE_ATCMD_DEVICE_ACTION,
                                        (u8 *)&action, sizeof(action));

                    configPRINTF( ( "acerpure action [AirCirculatorRotate -> %d] param[%s -> %d]\r\n",
                                    ACERPURE_ACTION_AIR_CIR_ROTATE, action_json->valuestring, action.param ) );
                }
            }

            // air detect mode
            action_json = cJSON_GetObjectItem(acerpure_json, "AirDetectMode");
            if (action_json)
            {
                memset(&action, 0, sizeof(action));
                action.action = ACERPURE_ACTION_AIR_DETECT_MODE;
                if (!strcmp(action_json->valuestring, "Off"))
                {
                    action.param = 0x00;
                }
                else if (!strcmp(action_json->valuestring, "PM2.5"))
                {
                    action.param = 0x01;
                }
                else if (!strcmp(action_json->valuestring, "PM1.0"))
                {
                    action.param = 0x02;
                }

                if (is_multiple_actions)
                {
                    // prvAddMultipleActions(&action, multiple_actions);
                    configPRINTF( ( "prvAddMultipleActions !!!\r\n" ) );
                }
                else
                {
                    acerpure_atcmd_send(ACERPURE_ATCMD_DEVICE_ACTION,
                                        (u8 *)&action, sizeof(action));

                    configPRINTF( ( "acerpure action [AirDetectMode -> %d] param[%s -> %d]\r\n",
                                    ACERPURE_ACTION_AIR_DETECT_MODE, action_json->valuestring, action.param ) );
                }
            }

            // kid mode
            action_json = cJSON_GetObjectItem(acerpure_json, "KidMode");
            if (action_json)
            {
                memset(&action, 0, sizeof(action));
                action.action = ACERPURE_ACTION_KID_MODE;
                if (!strcmp(action_json->valuestring, "On"))
                {
                    action.param = 0x01;
                }
                else if (!strcmp(action_json->valuestring, "Off"))
                {
                    action.param = 0x00;
                }

                if (is_multiple_actions)
                {
                    // prvAddMultipleActions(&action, multiple_actions);
                    configPRINTF( ( "prvAddMultipleActions !!!\r\n" ) );
                }
                else
                {
                    acerpure_atcmd_send(ACERPURE_ATCMD_DEVICE_ACTION,
                                        (u8 *)&action, sizeof(action));

                    configPRINTF( ( "acerpure action [KidMode -> %d] param[%s -> %d]\r\n",
                                    ACERPURE_ACTION_KID_MODE, action_json->valuestring, action.param ) );
                }
            }

            // sleep mode
            action_json = cJSON_GetObjectItem(acerpure_json, "SleepMode");
            if (action_json)
            {
                memset(&action, 0, sizeof(action));
                action.action = ACERPURE_ACTION_SLEEP_MODE;
                if (!strcmp(action_json->valuestring, "On"))
                {
                    // alert_test = 1;
                    action.param = 0x01;
                }
                else if (!strcmp(action_json->valuestring, "Off"))
                {
                    // alert_test = 0;
                    action.param = 0x00;
                }

                if (is_multiple_actions)
                {
                    // prvAddMultipleActions(&action, multiple_actions);
                    configPRINTF( ( "prvAddMultipleActions !!!\r\n" ) );
                }
                else
                {
                    acerpure_atcmd_send(ACERPURE_ATCMD_DEVICE_ACTION,
                                        (u8 *)&action, sizeof(action));

                    configPRINTF( ( "acerpure action [SleepMode -> %d] param[%s -> %d]\r\n",
                                    ACERPURE_ACTION_SLEEP_MODE, action_json->valuestring, action.param ) );
                }
            }

            // shutdown timer
            action_json = cJSON_GetObjectItem(acerpure_json, "ShutdownTimer");
            if (action_json)
            {
                memset(&action, 0, sizeof(action));
                action.action = ACERPURE_ACTION_SHUTDOWN_TIMER;
                if (!strcmp(action_json->valuestring, "Off"))
                {
                    action.param = 0x00;
                }
                else if (!strcmp(action_json->valuestring, "1"))
                {
                    action.param = 0x01;
                }
                else if (!strcmp(action_json->valuestring, "2"))
                {
                    action.param = 0x02;
                }
                else if (!strcmp(action_json->valuestring, "3"))
                {
                    action.param = 0x03;
                }
                else if (!strcmp(action_json->valuestring, "4"))
                {
                    action.param = 0x04;
                }
                else if (!strcmp(action_json->valuestring, "5"))
                {
                    action.param = 0x05;
                }
                else if (!strcmp(action_json->valuestring, "6"))
                {
                    action.param = 0x06;
                }
                else if (!strcmp(action_json->valuestring, "7"))
                {
                    action.param = 0x07;
                }
                else if (!strcmp(action_json->valuestring, "8"))
                {
                    action.param = 0x08;
                }
                else if (!strcmp(action_json->valuestring, "9"))
                {
                    action.param = 0x09;
                }
                else if (!strcmp(action_json->valuestring, "10"))
                {
                    action.param = 0x0a;
                }
                else if (!strcmp(action_json->valuestring, "11"))
                {
                    action.param = 0x0b;
                }
                else if (!strcmp(action_json->valuestring, "12"))
                {
                    action.param = 0x0c;
                }
                else if (!strcmp(action_json->valuestring, "0.5"))
                {
                    action.param = 0xf1;
                }

                if (is_multiple_actions)
                {
                    // prvAddMultipleActions(&action, multiple_actions);
                    configPRINTF( ( "prvAddMultipleActions !!!\r\n" ) );
                }
                else
                {
                    acerpure_atcmd_send(ACERPURE_ATCMD_DEVICE_ACTION,
                                        (u8 *)&action, sizeof(action));

                    configPRINTF( ( "acerpure action [ShutdownTimer -> %d] param[%s -> %d]\r\n",
                                    ACERPURE_ACTION_SHUTDOWN_TIMER, action_json->valuestring, action.param ) );
                }
            }

            // UV
            action_json = cJSON_GetObjectItem(acerpure_json, "UV");
            if (action_json)
            {
                memset(&action, 0, sizeof(action));
                action.action = ACERPURE_ACTION_UV;
                if (!strcmp(action_json->valuestring, "On"))
                {
                    action.param = 0x01;
                }
                else if (!strcmp(action_json->valuestring, "Off"))
                {
                    action.param = 0x00;
                }

                if (is_multiple_actions)
                {
                    // prvAddMultipleActions(&action, multiple_actions);
                    configPRINTF( ( "prvAddMultipleActions !!!\r\n" ) );
                }
                else
                {
                    acerpure_atcmd_send(ACERPURE_ATCMD_DEVICE_ACTION,
                                        (u8 *)&action, sizeof(action));

                    configPRINTF( ( "acerpure action [UV -> %d] param[%s -> %d]\r\n",
                                    ACERPURE_ACTION_UV, action_json->valuestring, action.param ) );
                }
            }

            // send my favorite actions if my favorite json received
            if (is_multiple_actions)
            {
                int action_num = 0;
                if (!g_extended_model)
                {
                    action_num = ACERPURE_ACTION_MY_FAVORITE;
                }
                else
                {
                    action_num = ACERPURE_ACTION_MAX - 1;
                }
                acerpure_atcmd_send(ACERPURE_ATCMD_MULTIPLE_ACTIONS,
                                    (u8 *)multiple_actions, (action_num * 2));

                configPRINTF( ( "==== acerpure multiple actions start ====\r\n" ) );
                for (int i = 0; i < (action_num * 2); i++)
                {
                    printf("%02X ", *(multiple_actions + i));
                }
                printf("\r\n");
                configPRINTF( ( "==== acerpure multiple actions end ====\r\n" ) );
            }
        }
        else
        {
            configPRINTF( ( "Action parser: not acerpure format '%s', length: %d\r\n",
                        received_json, length ) );
        }

        cJSON_Delete(parsed_json);
    }
    else
    {
        configPRINTF( ( "Action parser: Invalid json '%s', length: %d\r\n",
                        received_json, length ) );
    }
}


/*
    Acer TEST
*/
// *******************************
#define democonfigMQTT_TIMEOUT 60

#include <platform_stdlib.h>
#include <httpd/httpd.h>


void http_json_res(struct httpd_conn *conn, char *body)
{

	char *user_agent = NULL;

	// test log to show brief header parsing
	httpd_conn_dump_header(conn);

	// test log to show extra User-Agent header field
	if(httpd_request_get_header_field(conn, "User-Agent", &user_agent) != -1) {
		printf("\nUser-Agent=[%s]\n", user_agent);
		httpd_free(user_agent);
	}

	if(httpd_request_is_method(conn, "GET")) {
		httpd_response_write_header_start(conn, "200 OK", "application/json", strlen(body));
		httpd_response_write_header(conn, "Connection", "close");
		httpd_response_write_header_finish(conn);
		httpd_response_write_data(conn, (uint8_t*)body, strlen(body));
        free(body);
	}
	else
        httpd_response_method_not_allowed(conn, NULL);
	httpd_conn_close(conn);
}


void hp_cb(struct httpd_conn *conn)
{
	char *user_agent = NULL;

	// test log to show brief header parsing
	httpd_conn_dump_header(conn);

	// test log to show extra User-Agent header field
	if(httpd_request_get_header_field(conn, "User-Agent", &user_agent) != -1) {
		printf("\nUser-Agent=[%s]\n", user_agent);
		httpd_free(user_agent);
	}

	// GET homepage
	if(httpd_request_is_method(conn, "GET")) {
		char *body = \
"<HTML><BODY>" \
"It Works<BR>" \
"<BR>" \
"</BODY></HTML>";

		// write HTTP response
		httpd_response_write_header_start(conn, "200 OK", "text/html", strlen(body));
		httpd_response_write_header(conn, "Connection", "close");
		httpd_response_write_header_finish(conn);
		httpd_response_write_data(conn, (uint8_t*)body, strlen(body));
        free(body);
	}
	else {
		// HTTP/1.1 405 Method Not Allowed
		httpd_response_method_not_allowed(conn, NULL);
	}

	httpd_conn_close(conn);
}

void appversion_cb(struct httpd_conn *conn)
{
    char cDataBuffer[ AWS_MAX_DATA_LENGTH ];

    uint32_t ulReportLength;
    ulReportLength = snprintf( cDataBuffer,
                                AWS_MAX_DATA_LENGTH,
                                commfwREPORT_FORMAT,
                                APP_VERSION_MAJOR,
                                APP_VERSION_MINOR,
                                APP_VERSION_BUILD
                                );
    http_json_res(conn, cDataBuffer);
}

void mainfw_cb(struct httpd_conn *conn)
{
    char cDataBuffer[ AWS_MAX_DATA_LENGTH ];
    uint32_t ulReportLength;
    ulReportLength = snprintf( cDataBuffer,
                                AWS_MAX_DATA_LENGTH,
                                mainfwREPORT_FORMAT,
                                g_aws_device_info.main_fw
                                );
	http_json_res(conn, cDataBuffer);
}

void devicestate_cb(struct httpd_conn *conn)
{
    // ======TEST======
    // AWS_OTA_STATE g_aws_ota = AWS_OTA_STATE_IDLE;
    // #define ota_url "test"
    uint32_t acerpure_mqtt_timer = MONITOR_PERIOD;
    // ======TEST======
    char cDataBuffer[ AWS_MAX_DATA_LENGTH ];
    // char app_topic[32] = {0};
    // snprintf(app_topic, sizeof(app_topic), pubAPP_TOPIC_NAME, g_aws_mqtt_conf.client_id);

    // check state and mapping to string
    // protection! mapping index
    if (acerpure_device_state.power > 1)
    {
        acerpure_device_state.power = 1;
    }

    if (acerpure_device_state.purifier_speed > 4)
    {
        acerpure_device_state.purifier_speed = 4;
    }

    if (acerpure_device_state.circulator_speed == 0xff ||  // N/A
        acerpure_device_state.circulator_speed > 11)
    {
        acerpure_device_state.circulator_speed = 11;
    }

    if (acerpure_device_state.purifier_rotate > 1)
    {
        acerpure_device_state.purifier_rotate = 1;
    }

    if (acerpure_device_state.circulator_rotate > 1)
    {
        acerpure_device_state.circulator_rotate = 1;
    }

    if (acerpure_device_state.air_detect_mode > 2)
    {
        acerpure_device_state.air_detect_mode = 2;
    }

    if (acerpure_device_state.kid_mode > 1)
    {
        acerpure_device_state.kid_mode = 1;
    }

    if (acerpure_device_state.sleep_mode > 1)
    {
        acerpure_device_state.sleep_mode = 1;
    }

    if (acerpure_device_state.shutdown_timer == 0xf1 || // 0.5 hr
        acerpure_device_state.shutdown_timer > 13)
    {
        acerpure_device_state.shutdown_timer = 13;
    }

    if (acerpure_device_state.filter_install > 1)
    {
        acerpure_device_state.filter_install = 1;
    }

    if (acerpure_device_state.filter_health_value > 100)
    {
        acerpure_device_state.filter_health_value = 100;
    }

    // new
    if (acerpure_device_state.filter_health_level > 1)
    {
        acerpure_device_state.filter_health_level = 1;
    }
    // old
    if (acerpure_device_monitor.filter_health_level > 1)
    {
        acerpure_device_monitor.filter_health_level = 1;
    }

    if (acerpure_device_state.uv == 0xff) // N/A
    {
        acerpure_device_state.uv = 2; // mapping to "N/A"
    }
    else if (acerpure_device_state.uv > 1) // protection
    {
        acerpure_device_state.uv = 1;
    }

    if (acerpure_device_monitor.pm25_level > 3)
    {
        acerpure_device_monitor.pm25_level = 3;
    }

    if (acerpure_device_monitor.pm10_level > 3)
    {
        acerpure_device_monitor.pm10_level = 3;
    }

    if (acerpure_device_monitor.gas_level > 3)
    {
        acerpure_device_monitor.gas_level = 3;
    }

    if (acerpure_device_monitor.co2_level > 3)
    {
        acerpure_device_monitor.co2_level = 3;
    }

    if (acerpure_device_state.my_favor_icon > 1)
    {
        acerpure_device_state.my_favor_icon = 1;
    }

    uint32_t ulReportLength;

    if (new_monitor_format)
    {
        ulReportLength = snprintf( cDataBuffer,
                                AWS_MAX_DATA_LENGTH,
                                deviceStateREPORT_FORMAT,
                                "Acerpure-0",
                                g_aws_device_info.model_name,
                                g_aws_device_info.main_fw,
                                g_aws_device_info.comm_fw,
                                ota_url,
                                OTAState[g_aws_ota],
                                Power[acerpure_device_state.power],
                                AirPurifierSpeed[acerpure_device_state.purifier_speed],
                                AirCirculatorSpeed[acerpure_device_state.circulator_speed],
                                AirPurifierRotate[acerpure_device_state.purifier_rotate],
                                AirCirculatorRotate[acerpure_device_state.circulator_rotate],
                                AirDetectMode[acerpure_device_state.air_detect_mode],
                                KidMode[acerpure_device_state.kid_mode],
                                SleepMode[acerpure_device_state.sleep_mode],
                                ShutdownTimer[acerpure_device_state.shutdown_timer],
                                FilterState[acerpure_device_state.filter_install],
                                acerpure_device_state.filter_health_value,
                                FilterLevel[acerpure_device_state.filter_health_level],
                                UV[acerpure_device_state.uv],
                                PMGASLevel[acerpure_device_monitor.pm25_level],
                                PMGASLevel[acerpure_device_monitor.pm10_level],
                                PMGASLevel[acerpure_device_monitor.gas_level],
                                PMGASLevel[acerpure_device_monitor.co2_level],
                                MyFavorIcon[acerpure_device_state.my_favor_icon],
                                acerpure_device_monitor.pm25_value,
                                acerpure_device_monitor.pm10_value,
                                acerpure_device_monitor.gas_value,
                                acerpure_device_monitor.co2_value,
                                g_aws_alert_conf.filter_health,
                                g_aws_alert_conf.filter_health_5,
                                g_aws_alert_conf.filter_health_10,
                                g_aws_alert_conf.filter_health_20,
                                g_aws_alert_conf.filter_install,
                                g_aws_alert_conf.aqi,
                                g_aws_alert_conf.gas,
                                g_aws_alert_conf.co2
                                 );
    }
    else
    {
        ulReportLength = snprintf( cDataBuffer,
                                AWS_MAX_DATA_LENGTH,
                                deviceStateREPORT_FORMAT,
                                "Acerpure-0",
                                g_aws_device_info.model_name,
                                g_aws_device_info.main_fw,
                                g_aws_device_info.comm_fw,
                                ota_url,
                                OTAState[g_aws_ota],
                                Power[acerpure_device_state.power],
                                AirPurifierSpeed[acerpure_device_state.purifier_speed],
                                AirCirculatorSpeed[acerpure_device_state.circulator_speed],
                                AirPurifierRotate[acerpure_device_state.purifier_rotate],
                                AirCirculatorRotate[acerpure_device_state.circulator_rotate],
                                AirDetectMode[acerpure_device_state.air_detect_mode],
                                KidMode[acerpure_device_state.kid_mode],
                                SleepMode[acerpure_device_state.sleep_mode],
                                ShutdownTimer[acerpure_device_state.shutdown_timer],
                                FilterState[acerpure_device_state.filter_install],
                                acerpure_device_monitor.filter_health_value,
                                FilterLevel[acerpure_device_monitor.filter_health_level],
                                PMGASLevel[acerpure_device_monitor.pm25_level],
                                PMGASLevel[acerpure_device_monitor.pm10_level],
                                PMGASLevel[acerpure_device_monitor.gas_level],
                                MyFavorIcon[acerpure_device_state.my_favor_icon],
                                acerpure_device_monitor.pm25_value,
                                acerpure_device_monitor.pm10_value,
                                acerpure_device_monitor.gas_value,
                                g_aws_alert_conf.filter_health,
                                g_aws_alert_conf.filter_health_5,
                                g_aws_alert_conf.filter_health_10,
                                g_aws_alert_conf.filter_health_20,
                                g_aws_alert_conf.filter_install,
                                g_aws_alert_conf.aqi,
                                g_aws_alert_conf.gas
                                 );
    }
	http_json_res(conn, cDataBuffer);
}

void devicemonitor_cb(struct httpd_conn *conn)
{
    // ======TEST======
    // AWS_OTA_STATE g_aws_ota = AWS_OTA_STATE_IDLE;
    // #define ota_url "test"
    uint32_t acerpure_mqtt_timer = MONITOR_PERIOD;
    // ======TEST======

    char cDataBuffer[ AWS_MAX_DATA_LENGTH ];

    // modify save-monitor frequency...send once per 10 mins
    if (acerpure_mqtt_timer >= monitor_timer)
    {
        monitor_timer = acerpure_mqtt_timer + MONITOR_PERIOD;
        uint32_t ulReportLength;
        ulReportLength = snprintf( cDataBuffer,
                                    AWS_MAX_DATA_LENGTH,
                                    monitorValueREPORT_FORMAT,
                                    acerpure_device_monitor.pm25_value,
                                    acerpure_device_monitor.pm10_value,
                                    acerpure_device_monitor.gas_value,
                                    acerpure_device_monitor.co2_value
                                    );
    }
	http_json_res(conn, cDataBuffer);
}

void action_cb(struct httpd_conn *conn)
{
    // ======TEST======
    // AWS_OTA_STATE g_aws_ota = AWS_OTA_STATE_IDLE;
    // #define ota_url "test"
    // ======TEST======
    char cDataBuffer[ AWS_MAX_DATA_LENGTH ];

	if(httpd_request_is_method(conn, "POST")) {
		size_t read_size;
		size_t content_len = conn->request.content_len;
		uint8_t *body = (uint8_t *) malloc(content_len + 1);

		if(body) {
			// read HTTP body
			memset(body, 0, content_len + 1);
			read_size = httpd_request_read_data(conn, body, content_len);

            prvAWSActionParser(body, content_len);

			// write HTTP response
			httpd_response_write_header_start(conn, "200 OK", "application/json", 0);
			httpd_response_write_header(conn, "Connection", "close");
			httpd_response_write_header_finish(conn);
			httpd_response_write_data(conn, body, strlen((char const*)body));
			free(body);
		}
		else {
			httpd_response_internal_server_error(conn, NULL);
		}
	}
	else {
		httpd_response_method_not_allowed(conn, NULL);
	}
	httpd_conn_close(conn);
	// http_json_res(conn, cDataBuffer);
}



static void acer_test_httpd_thread(void *param)
{
	/* To avoid gcc warnings */
	( void ) param;
	httpd_reg_page_callback("/", hp_cb);
	httpd_reg_page_callback("/appversion", appversion_cb);
    httpd_reg_page_callback("/mainfw", mainfw_cb);
    httpd_reg_page_callback("/devicestate", devicestate_cb);
    httpd_reg_page_callback("/devicemonitor", devicemonitor_cb);
    httpd_reg_page_callback("/action", action_cb);
	if(httpd_start(80, 5, 4096, HTTPD_THREAD_SINGLE, HTTPD_SECURE_NONE) != 0) {
		printf("ERROR: httpd_start");
		httpd_clear_page_callbacks();
	}
	vTaskDelete(NULL);
}

int acer_test_httpd(void)
{
    uint8_t alert_test = 99;
	if(xTaskCreate(acer_test_httpd_thread, ((const char*)"acer_test_httpd_thread"), 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS){
        printf("\n\r%s xTaskCreate(acer_test_httpd_thread) failed", __FUNCTION__);
        return -1;
    }
    return 0;
}



/**
 * @brief Runs daemon in the system.
 */
int Acerpure_RunDaemon( void )
{
    return acer_test_httpd();
}
