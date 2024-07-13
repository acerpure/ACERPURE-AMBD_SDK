/* Standard includes. */
#include "string.h"
#include "stdio.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

// /* MQTT includes. */
// #include "aws_mqtt_agent.h"

// /* Credentials includes. */
// #include "aws_clientcredential.h"

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
#define pubAWS_TOPIC_NAME "incomming"
#define pubAPP_TOPIC_NAME "@%s"         // @clientID
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


// /**
//  * @brief Implements the task that connects to and then publishes messages to the
//  * MQTT broker.
//  *
//  * Messages are published while device state have been updated.
//  *
//  * @param[in] pvParameters Parameters passed while creating the task. Unused in our
//  * case.
//  */
// static void prvMQTTConnectAndPublishTask( void * pvParameters );

// /**
//  * @brief Creates an MQTT client and then connects to the MQTT broker.
//  *
//  * The MQTT broker end point is set by clientcredentialMQTT_BROKER_ENDPOINT.
//  *
//  * @return pdPASS if everything is successful, pdFAIL otherwise.
//  */
// static BaseType_t prvCreateClientAndConnectToBroker( void );

// /**
//  * @brief The callback registered with the MQTT client to get notified when
//  * data is received from the broker.
//  *
//  * @param[in] pvUserData User data as supplied while registering the callback.
//  * @param[in] pxCallbackParams Data received from the broker.
//  *
//  * @return Indicates whether or not we take the ownership of the buffer containing
//  * the MQTT message. We never take the ownership and always return eMQTTFalse.
//  */
// static MQTTBool_t prvMQTTCallback( void * pvUserData,
//                                    const MQTTPublishData_t * const pxCallbackParams );

// /**
//  * @brief Subscribes to the client id topic.
//  *
//  * @return pdPASS if subscribe operation is successful, pdFALSE otherwise.
//  */
// static BaseType_t prvSubscribe( void );

// /*-----------------------------------------------------------*/

// /**
//  * @brief The FreeRTOS message buffer that is used to send data from the callback
//  * function (see prvMQTTCallback() above) to the task that parse the message and
//  * send the corresponding AT command to mainboard
//  */
// static MessageBufferHandle_t xAWSMessageBuffer = NULL;

// /**
//  * @ brief The handle of the MQTT client object used by the MQTT acerpure aws daemon.
//  */
// static MQTTAgentHandle_t xMQTTHandle = NULL;

/*-----------------------------------------------------------*/
// static BaseType_t prvCreateClientAndConnectToBroker( void )
// {
//     MQTTAgentReturnCode_t xReturned;
//     BaseType_t xReturn = pdFAIL;
//     MQTTAgentConnectParams_t xConnectParameters =
//     {
//         (const char *)g_aws_mqtt_conf.mqtt_broker_endpoint,   /* The URL of the MQTT broker to connect to. */
//         democonfigMQTT_AGENT_CONNECT_FLAGS,   /* Connection flags. */
//         pdFALSE,                              /* Deprecated. */
//         clientcredentialMQTT_BROKER_PORT,     /* Port number on which the MQTT broker is listening. Can be overridden by ALPN connection flag. */
//         (uint8_t *)g_aws_mqtt_conf.client_id,   /* Client Identifier of the MQTT client. It should be unique per broker. */
//         0,                                    /* The length of the client Id, filled in later as not const. */
//         pdFALSE,                              /* Deprecated. */
//         NULL,                                 /* User data supplied to the callback. Can be NULL. */
//         NULL,                                 /* Callback used to report various events. Can be NULL. */
//         NULL,                                 /* Certificate used for secure connection. Can be NULL. */
//         0                                     /* Size of certificate used for secure connection. */
//     };

//     /* Check this function has not already been executed. */
//     configASSERT( xMQTTHandle == NULL );

//     /* The MQTT client object must be created before it can be used.  The
//      * maximum number of MQTT client objects that can exist simultaneously
//      * is set by mqttconfigMAX_BROKERS. */
//     xReturned = MQTT_AGENT_Create( &xMQTTHandle );

//     if( xReturned == eMQTTAgentSuccess )
//     {
//         /* Fill in the MQTTAgentConnectParams_t member that is not const,
//          * and therefore could not be set in the initializer (where
//          * xConnectParameters is declared in this function). */
//         xConnectParameters.usClientIdLength = ( uint16_t ) strlen( ( const char * ) g_aws_mqtt_conf.client_id );

//         /* Connect to the broker. */
//         configPRINTF( ( "MQTT acerpure attempting to connect to %s.\r\n", g_aws_mqtt_conf.mqtt_broker_endpoint ) );
//         xReturned = MQTT_AGENT_Connect( xMQTTHandle,
//                                         &xConnectParameters,
//                                         democonfigMQTT_ACERPURE_TLS_NEGOTIATION_TIMEOUT );

//         if( xReturned != eMQTTAgentSuccess )
//         {
//             /* Could not connect, so delete the MQTT client. */
//             ( void ) MQTT_AGENT_Delete( xMQTTHandle );
//             configPRINTF( ( "ERROR:  MQTT acerpure failed to connect.\r\n" ) );
//         }
//         else
//         {
//             configPRINTF( ( "MQTT acerpure connected.\r\n" ) );
//             xReturn = pdPASS;
//         }
//     }

//     return xReturn;
// }

// static void prvAddMultipleActions(acerpuer_action_t *action, unsigned char *multiple_actions)
// {
//     int multiple_actions_offset = 0;

//     if (ACERPURE_ACTION_MY_FAVORITE == action->action)  // 1st in multiple actions
//     {
//         multiple_actions_offset = 0;
//     }
//     else
//     {
//         multiple_actions_offset = (action->action - 1) * 2 + 2; // +2 for my favorite action offset
//     }

//     // extended model, extended action?
//     if (g_extended_model && action->action > ACERPURE_ACTION_MY_FAVORITE)
//     {
//         multiple_actions_offset = (action->action - ACERPURE_ACTION_MY_FAVORITE - 1) * 2 + (ACERPURE_ACTION_MY_FAVORITE * 2);
//     }

//     if (multiple_actions_offset < ((ACERPURE_ACTION_MAX - 1) * 2)) // protect
//     {
//         multiple_actions[multiple_actions_offset] = action->action;
//         multiple_actions[multiple_actions_offset + 1] = action->param;
//     }
// }

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

// static void prvMessageParserTask( void * pvParameters )
// {
//     acerpure_mqtt_parser_alive = 1;

//     MQTTAgentPublishParams_t xPublishParameters;
//     MQTTAgentReturnCode_t xReturned;
//     char cDataBuffer[ AWS_MAX_DATA_LENGTH ];
//     size_t xBytesReceived;

//     /* Remove compiler warnings about unused parameters. */
//     ( void ) pvParameters;

//     /* Check this task has not already been created. */
//     configASSERT( xMQTTHandle != NULL );
//     configASSERT( xAWSMessageBuffer != NULL );

//     while (!acerpure_mqtt_parser_exit)
//     {
//         memset( cDataBuffer, 0x00, sizeof( cDataBuffer ) );
//         xBytesReceived = xMessageBufferReceive( xAWSMessageBuffer,
//                                                 cDataBuffer,
//                                                 sizeof( cDataBuffer ),
//                                                 pdMS_TO_TICKS(3000) );

//         if (xBytesReceived > 0)
//         {
//             configPRINTF( ( "Message from topic: '%s', length: %d\r\n",
//                             cDataBuffer, strlen(cDataBuffer) ) );

//             prvAWSActionParser(cDataBuffer, strlen(cDataBuffer));
//         }
//     }

//     acerpure_mqtt_parser_alive = 0;
//     configPRINTF( ( "MQTT acerpure parser finished.\r\n" ) );

//     vTaskDelete( NULL );
// }
/*-----------------------------------------------------------*/

// static BaseType_t prvSubscribe( void )
// {
//     MQTTAgentReturnCode_t xReturned;
//     BaseType_t xReturn;
//     MQTTAgentSubscribeParams_t xSubscribeParams;

//     /* Setup subscribe parameters to subscribe to client id topic. */
//     xSubscribeParams.pucTopic = (const uint8_t *)g_aws_mqtt_conf.client_id;
//     xSubscribeParams.pvPublishCallbackContext = NULL;
//     xSubscribeParams.pxPublishCallback = prvMQTTCallback;
//     xSubscribeParams.usTopicLength = ( uint16_t ) strlen( ( const char * ) g_aws_mqtt_conf.client_id );
//     xSubscribeParams.xQoS = eMQTTQoS1;

//     /* Subscribe to the topic. */
//     xReturned = MQTT_AGENT_Subscribe( xMQTTHandle,
//                                       &xSubscribeParams,
//                                       democonfigMQTT_TIMEOUT );

//     if( xReturned == eMQTTAgentSuccess )
//     {
//         configPRINTF( ( "MQTT acerpure subscribed to %s\r\n", g_aws_mqtt_conf.client_id ) );
//         xReturn = pdPASS;
//     }
//     else
//     {
//         configPRINTF( ( "ERROR:  MQTT acerpure could not subscribe to %s\r\n", g_aws_mqtt_conf.client_id ) );
//         xReturn = pdFAIL;
//     }

//     return xReturn;
// }
/*-----------------------------------------------------------*/

// static MQTTBool_t prvMQTTCallback( void * pvUserData,
//                                    const MQTTPublishData_t * const pxPublishParameters )
// {
//     char cBuffer[ AWS_MAX_DATA_LENGTH ];
//     uint32_t ulBytesToCopy = ( AWS_MAX_DATA_LENGTH - 1 );

//     /* Remove warnings about the unused parameters. */
//     ( void ) pvUserData;

//     /* Don't expect the callback to be invoked for any other topics. */
//     configASSERT( ( size_t ) ( pxPublishParameters->usTopicLength ) == strlen( ( const char * ) g_aws_mqtt_conf.client_id ) );
//     configASSERT( memcmp( pxPublishParameters->pucTopic, g_aws_mqtt_conf.client_id, ( size_t ) ( pxPublishParameters->usTopicLength ) ) == 0 );

//     /* THe ulBytesToCopy has already been initialized to ensure it does not copy
//      * more bytes than will fit in the buffer.  Now check it does not copy more
//      * bytes than are available. */
//     if( pxPublishParameters->ulDataLength <= ulBytesToCopy )
//     {
//         ulBytesToCopy = pxPublishParameters->ulDataLength;

//         /* Set the buffer to zero and copy the data into the buffer to ensure
//          * there is a NULL terminator and the buffer can be accessed as a
//          * string. */
//         memset( cBuffer, 0x00, sizeof( cBuffer ) );
//         memcpy( cBuffer, pxPublishParameters->pvData, ( size_t ) ulBytesToCopy );

//         ( void ) xMessageBufferSend( xAWSMessageBuffer, cBuffer, ( size_t ) ulBytesToCopy + ( size_t ) 1,  0);
//     }
//     else
//     {
//         configPRINTF( ( "[WARN]: Dropping received message as it does not fit in the buffer.\r\n" ) );
//     }

//     /* The data was copied into the FreeRTOS message buffer, so the buffer
//      * containing the data is no longer required.  Returning eMQTTFalse tells the
//      * MQTT agent that the ownership of the buffer containing the message lies with
//      * the agent and it is responsible for freeing the buffer. */
//     return eMQTTFalse;
// }

int publish_count = 0;
int publish_error = 0;
// static void prvPublishMessage(const uint8_t *topic, const char *message, uint32_t length)
// {
//     /* Check this function is not being called before the MQTT client object has
//      * been created. */
//     configASSERT( xMQTTHandle != NULL );

//     /* Setup the publish parameters. */
//     MQTTAgentPublishParams_t xPublishParameters;
//     MQTTAgentReturnCode_t xReturned;

//     memset( &( xPublishParameters ), 0x00, sizeof( xPublishParameters ) );
//     xPublishParameters.pucTopic = topic;
//     xPublishParameters.pvData = message;
//     xPublishParameters.usTopicLength = ( uint16_t ) strlen( ( const char * ) topic );
//     xPublishParameters.ulDataLength = length;
//     xPublishParameters.xQoS = eMQTTQoS1;

//     /* Publish the message. */
//     xReturned = MQTT_AGENT_Publish( xMQTTHandle,
//                                     &( xPublishParameters ),
//                                     democonfigMQTT_TIMEOUT );

//     if( xReturned == eMQTTAgentSuccess )
//     {
//         configPRINTF( ( "Message successfully published '%s' to '%s', length: %d\r\n",
//                         message, xPublishParameters.pucTopic, length ) );
//         // reset
//         publish_error = 0;
//     }
//     else
//     {
//         configPRINTF( ( "ERROR:  Message failed to publish '%s' to '%s'\r\n",
//                         message, xPublishParameters.pucTopic ) );
//         publish_error++;
//     }

//     publish_count++;
//     configPRINTF( ( "Message publish times: %d, error count: %d\r\n",
//                         publish_count, publish_error ) );

//     /* Remove compiler warnings in case configPRINTF() is not defined. */
//     ( void ) xReturned;
// }

// static void prvPublishCommFWMessage(void)
// {
//     char cDataBuffer[ AWS_MAX_DATA_LENGTH ];

//     uint32_t ulReportLength;
//     ulReportLength = snprintf( cDataBuffer,
//                                 AWS_MAX_DATA_LENGTH,
//                                 commfwREPORT_FORMAT,
//                                 APP_VERSION_MAJOR,
//                                 APP_VERSION_MINOR,
//                                 APP_VERSION_BUILD
//                                 );

//     // prvPublishMessage(pubAWS_TOPIC_NAME, cDataBuffer, ulReportLength);
// }

// static void prvPublishMainFWMessage(void)
// {
//     char cDataBuffer[ AWS_MAX_DATA_LENGTH ];

//     uint32_t ulReportLength;
//     ulReportLength = snprintf( cDataBuffer,
//                                 AWS_MAX_DATA_LENGTH,
//                                 mainfwREPORT_FORMAT,
//                                 g_aws_device_info.main_fw
//                                 );

//     // prvPublishMessage(pubAWS_TOPIC_NAME, cDataBuffer, ulReportLength);
// }

// static void prvPublishDeviceStateMessage(void)
// {
//     char cDataBuffer[ AWS_MAX_DATA_LENGTH ];
//     char app_topic[32] = {0};
//     // snprintf(app_topic, sizeof(app_topic), pubAPP_TOPIC_NAME, g_aws_mqtt_conf.client_id);

//     // check state and mapping to string
//     // protection! mapping index
//     if (acerpure_device_state.power > 1)
//     {
//         acerpure_device_state.power = 1;
//     }

//     if (acerpure_device_state.purifier_speed > 4)
//     {
//         acerpure_device_state.purifier_speed = 4;
//     }

//     if (acerpure_device_state.circulator_speed == 0xff ||  // N/A
//         acerpure_device_state.circulator_speed > 11)
//     {
//         acerpure_device_state.circulator_speed = 11;
//     }

//     if (acerpure_device_state.purifier_rotate > 1)
//     {
//         acerpure_device_state.purifier_rotate = 1;
//     }

//     if (acerpure_device_state.circulator_rotate > 1)
//     {
//         acerpure_device_state.circulator_rotate = 1;
//     }

//     if (acerpure_device_state.air_detect_mode > 2)
//     {
//         acerpure_device_state.air_detect_mode = 2;
//     }

//     if (acerpure_device_state.kid_mode > 1)
//     {
//         acerpure_device_state.kid_mode = 1;
//     }

//     if (acerpure_device_state.sleep_mode > 1)
//     {
//         acerpure_device_state.sleep_mode = 1;
//     }

//     if (acerpure_device_state.shutdown_timer == 0xf1 || // 0.5 hr
//         acerpure_device_state.shutdown_timer > 13)
//     {
//         acerpure_device_state.shutdown_timer = 13;
//     }

//     if (acerpure_device_state.filter_install > 1)
//     {
//         acerpure_device_state.filter_install = 1;
//     }

//     if (acerpure_device_state.filter_health_value > 100)
//     {
//         acerpure_device_state.filter_health_value = 100;
//     }

//     // new
//     if (acerpure_device_state.filter_health_level > 1)
//     {
//         acerpure_device_state.filter_health_level = 1;
//     }
//     // old
//     if (acerpure_device_monitor.filter_health_level > 1)
//     {
//         acerpure_device_monitor.filter_health_level = 1;
//     }

//     if (acerpure_device_state.uv == 0xff) // N/A
//     {
//         acerpure_device_state.uv = 2; // mapping to "N/A"
//     }
//     else if (acerpure_device_state.uv > 1) // protection
//     {
//         acerpure_device_state.uv = 1;
//     }

//     if (acerpure_device_monitor.pm25_level > 3)
//     {
//         acerpure_device_monitor.pm25_level = 3;
//     }

//     if (acerpure_device_monitor.pm10_level > 3)
//     {
//         acerpure_device_monitor.pm10_level = 3;
//     }

//     if (acerpure_device_monitor.gas_level > 3)
//     {
//         acerpure_device_monitor.gas_level = 3;
//     }

//     if (acerpure_device_monitor.co2_level > 3)
//     {
//         acerpure_device_monitor.co2_level = 3;
//     }

//     if (acerpure_device_state.my_favor_icon > 1)
//     {
//         acerpure_device_state.my_favor_icon = 1;
//     }

//     uint32_t ulReportLength;

//     if (new_monitor_format)
//     {
//         ulReportLength = snprintf( cDataBuffer,
//                                 AWS_MAX_DATA_LENGTH,
//                                 deviceStateREPORT_FORMAT,
//                                 "Acerpure-0",
//                                 g_aws_device_info.model_name,
//                                 g_aws_device_info.main_fw,
//                                 g_aws_device_info.comm_fw,
//                                 ota_url,
//                                 OTAState[g_aws_ota],
//                                 Power[acerpure_device_state.power],
//                                 AirPurifierSpeed[acerpure_device_state.purifier_speed],
//                                 AirCirculatorSpeed[acerpure_device_state.circulator_speed],
//                                 AirPurifierRotate[acerpure_device_state.purifier_rotate],
//                                 AirCirculatorRotate[acerpure_device_state.circulator_rotate],
//                                 AirDetectMode[acerpure_device_state.air_detect_mode],
//                                 KidMode[acerpure_device_state.kid_mode],
//                                 SleepMode[acerpure_device_state.sleep_mode],
//                                 ShutdownTimer[acerpure_device_state.shutdown_timer],
//                                 FilterState[acerpure_device_state.filter_install],
//                                 acerpure_device_state.filter_health_value,
//                                 FilterLevel[acerpure_device_state.filter_health_level],
//                                 UV[acerpure_device_state.uv],
//                                 PMGASLevel[acerpure_device_monitor.pm25_level],
//                                 PMGASLevel[acerpure_device_monitor.pm10_level],
//                                 PMGASLevel[acerpure_device_monitor.gas_level],
//                                 PMGASLevel[acerpure_device_monitor.co2_level],
//                                 MyFavorIcon[acerpure_device_state.my_favor_icon],
//                                 acerpure_device_monitor.pm25_value,
//                                 acerpure_device_monitor.pm10_value,
//                                 acerpure_device_monitor.gas_value,
//                                 acerpure_device_monitor.co2_value,
//                                 g_aws_alert_conf.filter_health,
//                                 g_aws_alert_conf.filter_health_5,
//                                 g_aws_alert_conf.filter_health_10,
//                                 g_aws_alert_conf.filter_health_20,
//                                 g_aws_alert_conf.filter_install,
//                                 g_aws_alert_conf.aqi,
//                                 g_aws_alert_conf.gas,
//                                 g_aws_alert_conf.co2
//                                  );
//     }
//     else
//     {
//         ulReportLength = snprintf( cDataBuffer,
//                                 AWS_MAX_DATA_LENGTH,
//                                 deviceStateREPORT_FORMAT,
//                                 "Acerpure-0",
//                                 g_aws_device_info.model_name,
//                                 g_aws_device_info.main_fw,
//                                 g_aws_device_info.comm_fw,
//                                 ota_url,
//                                 OTAState[g_aws_ota],
//                                 Power[acerpure_device_state.power],
//                                 AirPurifierSpeed[acerpure_device_state.purifier_speed],
//                                 AirCirculatorSpeed[acerpure_device_state.circulator_speed],
//                                 AirPurifierRotate[acerpure_device_state.purifier_rotate],
//                                 AirCirculatorRotate[acerpure_device_state.circulator_rotate],
//                                 AirDetectMode[acerpure_device_state.air_detect_mode],
//                                 KidMode[acerpure_device_state.kid_mode],
//                                 SleepMode[acerpure_device_state.sleep_mode],
//                                 ShutdownTimer[acerpure_device_state.shutdown_timer],
//                                 FilterState[acerpure_device_state.filter_install],
//                                 acerpure_device_monitor.filter_health_value,
//                                 FilterLevel[acerpure_device_monitor.filter_health_level],
//                                 PMGASLevel[acerpure_device_monitor.pm25_level],
//                                 PMGASLevel[acerpure_device_monitor.pm10_level],
//                                 PMGASLevel[acerpure_device_monitor.gas_level],
//                                 MyFavorIcon[acerpure_device_state.my_favor_icon],
//                                 acerpure_device_monitor.pm25_value,
//                                 acerpure_device_monitor.pm10_value,
//                                 acerpure_device_monitor.gas_value,
//                                 g_aws_alert_conf.filter_health,
//                                 g_aws_alert_conf.filter_health_5,
//                                 g_aws_alert_conf.filter_health_10,
//                                 g_aws_alert_conf.filter_health_20,
//                                 g_aws_alert_conf.filter_install,
//                                 g_aws_alert_conf.aqi,
//                                 g_aws_alert_conf.gas
//                                  );
//     }

//     // publish two topic
//     // prvPublishMessage(pubAWS_TOPIC_NAME, cDataBuffer, ulReportLength);

//     //// prvPublishMessage(app_topic, cDataBuffer, ulReportLength);
// }

// static void prvPublishDeviceMonitorMessage(void)
// {
//     char cDataBuffer[ AWS_MAX_DATA_LENGTH ];

//     // modify save-monitor frequency...send once per 10 mins
//     if (acerpure_mqtt_timer >= monitor_timer)
//     {
//         monitor_timer = acerpure_mqtt_timer + MONITOR_PERIOD;

//         uint32_t ulReportLength;
//         ulReportLength = snprintf( cDataBuffer,
//                                     AWS_MAX_DATA_LENGTH,
//                                     monitorValueREPORT_FORMAT,
//                                     acerpure_device_monitor.pm25_value,
//                                     acerpure_device_monitor.pm10_value,
//                                     acerpure_device_monitor.gas_value,
//                                     acerpure_device_monitor.co2_value
//                                     );

//         // prvPublishMessage(pubAWS_TOPIC_NAME, cDataBuffer, ulReportLength);
//     }
// }

// static void prvUpdatePublishTimer(AWS_ALERT alert)
// {
//     switch (alert)
//     {
//         case AWS_ALERT_FILTER_HEALTH:
//         {
//             filter_health_alert_timer = acerpure_mqtt_timer + g_aws_alert_conf.filter_health;
//             break;
//         }
//         case AWS_ALERT_FILTER_HEALTH_5:
//         {
//             filter_health_5_alert_timer = acerpure_mqtt_timer + g_aws_alert_conf.filter_health_5;
//             break;
//         }
//         case AWS_ALERT_FILTER_HEALTH_10:
//         {
//             filter_health_10_alert_timer = acerpure_mqtt_timer + g_aws_alert_conf.filter_health_10;
//             break;
//         }
//         case AWS_ALERT_FILTER_HEALTH_20:
//         {
//             filter_health_20_alert_timer = acerpure_mqtt_timer + g_aws_alert_conf.filter_health_20;
//             break;
//         }
//         case AWS_ALERT_FILTER_INSTALL:
//         {
//             filter_install_alert_timer = acerpure_mqtt_timer + g_aws_alert_conf.filter_install;
//             break;
//         }
//         case AWS_ALERT_AQI:
//         {
//             aqi_alert_timer = acerpure_mqtt_timer + g_aws_alert_conf.aqi;
//             break;
//         }
//         case AWS_ALERT_GAS:
//         {
//             gas_alert_timer = acerpure_mqtt_timer + g_aws_alert_conf.gas;
//             break;
//         }
//         case AWS_ALERT_CO2:
//         {
//             co2_alert_timer = acerpure_mqtt_timer + g_aws_alert_conf.co2;
//             break;
//         }
//         default:
//         {
//             configPRINTF( ( "invalid alert type. should not be this case\r\n" ) );
//             break;
//         }
//     }
// }

static void prvPublishAlertNotificationMessage(AWS_ALERT alert)
{
    char cDataBuffer[ AWS_MAX_DATA_LENGTH ];

    uint32_t ulReportLength;
    ulReportLength = snprintf( cDataBuffer,
                                AWS_MAX_DATA_LENGTH,
                                alertNotificationREPORT_FORMAT,
                                AlertNotification[alert]
                                );

    // prvPublishMessage(pubAWS_TOPIC_NAME, cDataBuffer, ulReportLength);

    // update for the next publish
    // prvUpdatePublishTimer(alert);
}

// static void prvCheckAlertNotification()
// {
//     // TickType_t current_tick = xTaskGetTickCount();
//     // configPRINTF( ( "device alert publish tick: %d!\r\n", current_tick ) );

//     // check if alert interval update
//     if (is_alert_interval_update)
//     {
//         is_alert_interval_update = 0;
//         prvUpdatePublishTimer(AWS_ALERT_FILTER_HEALTH);
//         prvUpdatePublishTimer(AWS_ALERT_FILTER_HEALTH_5);
//         prvUpdatePublishTimer(AWS_ALERT_FILTER_HEALTH_10);
//         prvUpdatePublishTimer(AWS_ALERT_FILTER_HEALTH_20);
//         prvUpdatePublishTimer(AWS_ALERT_FILTER_INSTALL);
//         prvUpdatePublishTimer(AWS_ALERT_AQI);
//         prvUpdatePublishTimer(AWS_ALERT_GAS);
//         prvUpdatePublishTimer(AWS_ALERT_CO2);
//     }

//     // check filter install alert
//     if (0 == acerpure_device_state.filter_install)
//     {
//         if (0 == (aws_device_alert & (1 << AWS_ALERT_FILTER_INSTALL)))
//         {
//             aws_device_alert |= (1 << AWS_ALERT_FILTER_INSTALL);
//             prvPublishAlertNotificationMessage(AWS_ALERT_FILTER_INSTALL);
//             configPRINTF( ( "set FilterInstallFaultAlert immediately!!! alert: 0x%X\r\n", aws_device_alert ) );
//         }
//         else
//         {
//             if (acerpure_mqtt_timer >= filter_install_alert_timer)
//             {
//                 prvPublishAlertNotificationMessage(AWS_ALERT_FILTER_INSTALL);
//             }
//         }
//     }
//     else
//     {
//         aws_device_alert &= ~(1 << AWS_ALERT_FILTER_INSTALL);
//     }

//     // check filter health
//     // check filter health alert
//     if (acerpure_device_state.filter_health_value == 0)
//     {
//         if (0 == (aws_device_alert & (1 << AWS_ALERT_FILTER_HEALTH)))
//         {
//             aws_device_alert &= 0xf0;   // clear filter health alert first, bit 0~3
//             aws_device_alert |= (1 << AWS_ALERT_FILTER_HEALTH);
//             prvPublishAlertNotificationMessage(AWS_ALERT_FILTER_HEALTH);
//             configPRINTF( ( "set FilterEndOfLifeAlert immediately!!! alert: 0x%X\r\n", aws_device_alert ) );
//         }
//         else
//         {
//             if (acerpure_mqtt_timer >= filter_health_alert_timer)
//             {
//                 prvPublishAlertNotificationMessage(AWS_ALERT_FILTER_HEALTH);
//             }
//         }
//     }
//     else if (acerpure_device_state.filter_health_value > 0 &&
//              acerpure_device_state.filter_health_value <= 5)
//     {
//         if (0 == (aws_device_alert & (1 << AWS_ALERT_FILTER_HEALTH_5)))
//         {
//             aws_device_alert &= 0xf0;   // clear filter health alert first, bit 0~3
//             aws_device_alert |= (1 << AWS_ALERT_FILTER_HEALTH_5);
//             prvPublishAlertNotificationMessage(AWS_ALERT_FILTER_HEALTH_5);
//             configPRINTF( ( "set FilterEndOfLifeAlert5 immediately!!! alert: 0x%X\r\n", aws_device_alert ) );
//         }
//         else
//         {
//             if (acerpure_mqtt_timer >= filter_health_5_alert_timer)
//             {
//                 prvPublishAlertNotificationMessage(AWS_ALERT_FILTER_HEALTH_5);
//             }
//         }
//     }
//     else if (acerpure_device_state.filter_health_value > 5 &&
//              acerpure_device_state.filter_health_value <= 10)
//     {
//         if (0 == (aws_device_alert & (1 << AWS_ALERT_FILTER_HEALTH_10)))
//         {
//             aws_device_alert &= 0xf0;   // clear filter health alert first, bit 0~3
//             aws_device_alert |= (1 << AWS_ALERT_FILTER_HEALTH_10);
//             prvPublishAlertNotificationMessage(AWS_ALERT_FILTER_HEALTH_10);
//             configPRINTF( ( "set FilterEndOfLifeAlert10 immediately!!! alert: 0x%X\r\n", aws_device_alert ) );
//         }
//         else
//         {
//             if (acerpure_mqtt_timer >= filter_health_10_alert_timer)
//             {
//                 prvPublishAlertNotificationMessage(AWS_ALERT_FILTER_HEALTH_10);
//             }
//         }
//     }
//     else if (acerpure_device_state.filter_health_value > 10 &&
//              acerpure_device_state.filter_health_value <= 20)
//     {
//         if (0 == (aws_device_alert & (1 << AWS_ALERT_FILTER_HEALTH_20)))
//         {
//             aws_device_alert &= 0xf0;   // clear filter health alert first, bit 0~3
//             aws_device_alert |= (1 << AWS_ALERT_FILTER_HEALTH_20);
//             prvPublishAlertNotificationMessage(AWS_ALERT_FILTER_HEALTH_20);
//             configPRINTF( ( "set FilterEndOfLifeAlert20 immediately!!! alert: 0x%X\r\n", aws_device_alert ) );
//         }
//         else
//         {
//             if (acerpure_mqtt_timer >= filter_health_20_alert_timer)
//             {
//                 prvPublishAlertNotificationMessage(AWS_ALERT_FILTER_HEALTH_20);
//             }
//         }
//     }
//     else
//     {
//         aws_device_alert &= 0xf0;   // filter health
//     }

//     // check aqi alert
//     // check PM2.5/PM1.0 aqi alert
//     if ((ACERPURE_AIR_DETECT_MODE_PM25 == acerpure_device_state.air_detect_mode && acerpure_device_monitor.pm25_level == 3) ||
//         (ACERPURE_AIR_DETECT_MODE_PM10 == acerpure_device_state.air_detect_mode && acerpure_device_monitor.pm10_level == 3))
//     {
//         if (0 == (aws_device_alert & (1 << AWS_ALERT_AQI)))
//         {
//             aws_device_alert |= (1 << AWS_ALERT_AQI);
//             prvPublishAlertNotificationMessage(AWS_ALERT_AQI);
//             configPRINTF( ( "set AQIWarnAlert immediately!!! alert: 0x%X\r\n", aws_device_alert ) );
//         }
//         else
//         {
//             if (acerpure_mqtt_timer >= aqi_alert_timer)
//             {
//                 prvPublishAlertNotificationMessage(AWS_ALERT_AQI);
//             }
//         }
//     }
//     else
//     {
//         aws_device_alert &= ~(1 << AWS_ALERT_AQI);
//     }

//     // check gas alert
//     if (acerpure_device_monitor.gas_level == 3)
//     {
//         if (0 == (aws_device_alert & (1 << AWS_ALERT_GAS)))
//         {
//             aws_device_alert |= (1 << AWS_ALERT_GAS);
//             prvPublishAlertNotificationMessage(AWS_ALERT_GAS);
//             configPRINTF( ( "publish GASWarnAlert immediately!!! alert: 0x%X\r\n", aws_device_alert ) );
//         }
//         else
//         {
//             if (acerpure_mqtt_timer >= gas_alert_timer)
//             {
//                 prvPublishAlertNotificationMessage(AWS_ALERT_GAS);
//             }
//         }
//     }
//     else
//     {
//         aws_device_alert &= ~(1 << AWS_ALERT_GAS);
//     }

//     // check co2 alert
//     if (acerpure_device_monitor.co2_level == 3)
//     {
//         if (0 == (aws_device_alert & (1 << AWS_ALERT_CO2)))
//         {
//             aws_device_alert |= (1 << AWS_ALERT_CO2);
//             prvPublishAlertNotificationMessage(AWS_ALERT_CO2);
//             configPRINTF( ( "publish CO2WarnAlert immediately!!! alert: 0x%X\r\n", aws_device_alert ) );
//         }
//         else
//         {
//             if (acerpure_mqtt_timer >= co2_alert_timer)
//             {
//                 prvPublishAlertNotificationMessage(AWS_ALERT_CO2);
//             }
//         }
//     }
//     else
//     {
//         aws_device_alert &= ~(1 << AWS_ALERT_CO2);
//     }
// }

// static void prvMQTTConnectAndPublishTask( void * pvParameters )
// {
//     BaseType_t xReturned;
//     TaskHandle_t xMessageParserTask = NULL;

//     /* Avoid compiler warnings about unused parameters. */
//     ( void ) pvParameters;

//     /* Create the MQTT client object and connect it to the MQTT broker. */
//     xReturned = prvCreateClientAndConnectToBroker();

//     if( xReturned == pdPASS )
//     {
//         g_wifi_state = ACERPURE_WIFI_STATE_CONNECTED;

//         /* Create the task that echoes data received in the callback back to the
//          * MQTT broker. */
//         xReturned = xTaskCreate( prvMessageParserTask,                    /* The function that implements the task. */
//                                  "MQTTMessageParser",                     /* Human readable name for the task. */
//                                  democonfigMQTT_ACERPURE_TASK_STACK_SIZE, /* Size of the stack to allocate for the task, in words not bytes! */
//                                  NULL,                                    /* The task parameter is not used. */
//                                  democonfigMQTT_ACERPURE_TASK_PRIORITY,   /* The priority at which the task being created will run. */
//                                  &( xMessageParserTask ) );               /* The handle is stored so the created task can be deleted again at the end of the demo. */

//         if( xReturned == pdPASS )
//         {
//             configPRINTF( ( "MQTT message parser task created.\r\n" ) );
//         }
//         else
//         {
//             /* The task could not be created because there was insufficient FreeRTOS
//              * heap available to create the task's data structures and/or stack. */
//             configPRINTF( ( "MQTT message parser task could not be created - out of heap space?\r\n" ) );
//             goto acerpure_mqtt_connect_fail;
//         }
//     }
//     else
//     {
//         configPRINTF( ( "MQTT acerpure daemon could not connect to broker.\r\n" ) );
//         goto acerpure_mqtt_connect_fail;
//     }

//     // check fw version update?
//     if (g_need_update_version)
//     {
//         configPRINTF( ( "current fw and comm_fw mismatch, publish current version to aws!\r\n" ) );

//         memset(g_aws_device_info.comm_fw, 0, sizeof(g_aws_device_info.comm_fw));
//         snprintf(g_aws_device_info.comm_fw, sizeof(g_aws_device_info.comm_fw), "v%d.%d.%d",
//                     APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_BUILD);

//         prvPublishCommFWMessage();

//         g_aws_ota = AWS_OTA_STATE_IDLE; // OTA complete
//     }

//     if (g_need_update_mainboard_info)
//     {
//         configPRINTF( ( "current mainboard fw and main_fw mismatch, publish current version to aws!\r\n" ) );

//         prvPublishMainFWMessage();
//     }

//     if (g_need_update_version || g_need_update_mainboard_info)
//     {
//         configPRINTF( ( "Update aws device info! model_name: '%s', main_fw: '%s', comm_fw: '%s'\r\n",
//                         g_aws_device_info.model_name, g_aws_device_info.main_fw, g_aws_device_info.comm_fw) );

//         user_data_partition_ops(USER_PARTITION_AWS_DEVICE_INFO, USER_PARTITION_WRITE,
//                             (u8 *)&g_aws_device_info, sizeof(struct user_aws_device_info));

//         g_need_update_version = 0;
//         g_need_update_mainboard_info = 0;
//     }

//     /* Subscribe to the message topic. */
//     xReturned = prvSubscribe();

//     if( xReturned == pdPASS )
//     {
//         /* MQTT client is now connected to a broker. */
//         while (!acerpure_mqtt_task_exit)
//         {
//             // report while device noitify
//             if (device_state_report)
//             {
//                 device_state_report = 0;
//                 prvPublishDeviceStateMessage();
//             }

//             if (device_monitor_report)
//             {
//                 device_monitor_report = 0;
//                 prvPublishDeviceMonitorMessage();
//             }

//             // check the connection is broken?
//             // two device state and one device monitor reports per 10seconds
//             // error count > 3 for about 10~20 seconds timeout
//             if (publish_error > 3)
//             {
//                 acerpure_mqtt_parser_exit = 1;
//                 acerpure_mqtt_task_exit = 1;
//             }

//             // check alert notification here
//             prvCheckAlertNotification();

//             vTaskDelay( pdMS_TO_TICKS( 100 ) );
//         }
//     }
//     else
//     {
//         configPRINTF( ( "MQTT subscribe topic failed!\r\n" ) );
//     }

//     while (acerpure_mqtt_parser_alive)
//     {
//         acerpure_mqtt_parser_exit = 1;
//         configPRINTF( ( "Wait MQTT acerpure parser finished.\r\n" ) );
//         vTaskDelay(500);
//     }

//     /* Disconnect the client. */
//     ( void ) MQTT_AGENT_Disconnect( xMQTTHandle, democonfigMQTT_TIMEOUT );

//     ( void ) MQTT_AGENT_Delete( xMQTTHandle );


//     // vTaskDelete( xMessageParserTask );

// acerpure_mqtt_connect_fail:
//     vMessageBufferDelete( xAWSMessageBuffer );
//     xAWSMessageBuffer = NULL;

//     xMQTTHandle = NULL;
//     xMessageParserTask = NULL;
//     acerpure_mqtt_task_alive = 0;

//     /* End the daemon by deleting all created resources. */
//     configPRINTF( ( "MQTT acerpure daemon finished.\r\n" ) );

//     vTaskDelete( NULL ); /* Delete this task. */
// }


/*
    Acer TEST
*/
// *******************************
#define democonfigMQTT_TIMEOUT 60
#include <mDNS/mDNS.h>

#include <lwip_netconf.h>
#include <lwip/netif.h>

#include "wlan_scenario/example_wlan_scenario.h"
#include "mdns/example_mdns.h"
#include <platform_stdlib.h>
#include <httpd/httpd.h>


// static void example_mdns_thread(void *param)
// {
// 	/* To avoid gcc warnings */
// 	( void ) param;
	
// 	DNSServiceRef dnsServiceRef = NULL;
// 	TXTRecordRef txtRecord;
// 	unsigned char txt_buf[100];	// use fixed buffer for text record to prevent malloc/free

// 	// Delay to wait for IP by DHCP
// 	vTaskDelay(10000);

// 	printf("\nmDNS Init\n");
// 	if(mDNSResponderInit() == 0) {
// 		printf("mDNS Register service\n");
// 		TXTRecordCreate(&txtRecord, sizeof(txt_buf), txt_buf);
// 		TXTRecordSetValue(&txtRecord, "text1", strlen("text1_content"), "text1_content");
// 		TXTRecordSetValue(&txtRecord, "text2", strlen("text2_content"), "text2_content");
// 		dnsServiceRef = mDNSRegisterService("ameba", "_service1._tcp", "local", 5000, &txtRecord);
// 		TXTRecordDeallocate(&txtRecord);
// 		printf("wait for 30s ... \n");
// 		vTaskDelay(30*1000);
		
// 		printf("mDNS Update service\n");
// 		TXTRecordCreate(&txtRecord, sizeof(txt_buf), txt_buf);
// 		TXTRecordSetValue(&txtRecord, "text1", strlen("text1_content_new"), "text1_content_new");
// 		mDNSUpdateService(dnsServiceRef, &txtRecord, 0);
// 		TXTRecordDeallocate(&txtRecord);
// 		printf("wait for 30s ... \n");
// 		vTaskDelay(30*1000);

// 		if(dnsServiceRef)
// 			mDNSDeregisterService(dnsServiceRef);

// 		// deregister service before mdns deinit is not necessary
// 		// mDNS deinit will also deregister all services
// 		printf("mDNS Deinit\n");
// 		mDNSResponderDeinit();
// 	}

// 	vTaskDelete(NULL);
// }

// void acer_mdns(void)
// {
// 	if(xTaskCreate(acer_mdns_thread, ((const char*)"acer_mdns_thread"), 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
// 		printf("\n\r%s xTaskCreate(init_thread) failed", __FUNCTION__);
// }

// *******************************


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
"Can test GET by <A href=\"/test_get?test1=one%20%26%202&test2=three%3D3\">/test_get?test1=one%20%26%202&test2=three%3D3</A><BR>" \
"Can test POST by UI in <A href=\"/test_post.htm\">/test_post.htm</A><BR>" \
"</BODY></HTML>";

		// write HTTP response
		httpd_response_write_header_start(conn, "200 OK", "text/html", strlen(body));
		httpd_response_write_header(conn, "Connection", "close");
		httpd_response_write_header_finish(conn);
		httpd_response_write_data(conn, (uint8_t*)body, strlen(body));
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
    AWS_OTA_STATE g_aws_ota = AWS_OTA_STATE_IDLE;
    #define ota_url "test"
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
    AWS_OTA_STATE g_aws_ota = AWS_OTA_STATE_IDLE;
    #define ota_url "test"
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


static void acer_test_httpd_thread(void *param)
{
	/* To avoid gcc warnings */
	( void ) param;
	httpd_reg_page_callback("/", hp_cb);
	httpd_reg_page_callback("/appversion", appversion_cb);
    httpd_reg_page_callback("/mainfw", mainfw_cb);
    httpd_reg_page_callback("/devicestate", devicestate_cb);
    httpd_reg_page_callback("/devicemonitor", devicemonitor_cb);
	if(httpd_start(80, 5, 4096, HTTPD_THREAD_SINGLE, HTTPD_SECURE_NONE) != 0) {
		printf("ERROR: httpd_start");
		httpd_clear_page_callbacks();
	}
	vTaskDelete(NULL);
}

void acer_test_httpd(void)
{
    uint8_t alert_test = 99;
	if(xTaskCreate(acer_test_httpd_thread, ((const char*)"acer_test_httpd_thread"), 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(acer_test_httpd_thread) failed", __FUNCTION__);
}



/**
 * @brief Runs daemon in the system.
 */
int Acerpure_RunDaemon( void )
{

    // example_mdns();
    // if (g_aws_mqtt_conf.mqtt_broker_endpoint[0] == 0xff)
    // {
    //     configPRINTF( ( "Invalid MQTT config...NEED AWS PAIR, enter IDLE state" ) );

    //     return -1;
    // }

    // configPRINTF( ( "Start AWS IOT daemon! read MQTT information: endpoint: %s, client id: %s\r\n",
    //                 g_aws_mqtt_conf.mqtt_broker_endpoint, g_aws_mqtt_conf.client_id ) );

    // vStartAcerpureAWSDaemon();
    acer_test_httpd();

    return 0;
}

// void vStartAcerpureAWSDaemon( void )
// {
//     /* Create the message buffer used to pass strings from the MQTT callback
//      * function to the task that parse the message and send the corresponding AT command to mainboard
//      * The message buffer requires that there is space
//      * for the message length, which is held in a size_t variable. */
//     xAWSMessageBuffer = xMessageBufferCreate( ( size_t ) AWS_MAX_DATA_LENGTH + sizeof( size_t ) );
//     configASSERT( xAWSMessageBuffer );

//     /* Create the task that publishes messages to the MQTT broker
//      * This task, in turn, creates the task that parse the message
//      * and send the corresponding AT command to mainboard */
//     ( void ) xTaskCreate( prvMQTTConnectAndPublishTask,             /* The function that implements the demo task. */
//                           "MQTTMessagePublish",                     /* The name to assign to the task being created. */
//                           democonfigMQTT_ACERPURE_TASK_STACK_SIZE,  /* The size, in WORDS (not bytes), of the stack to allocate for the task being created. */
//                           NULL,                                     /* The task parameter is not being used. */
//                           democonfigMQTT_ACERPURE_TASK_PRIORITY,    /* The priority at which the task being created will run. */
//                           NULL );                                   /* Not storing the task's handle. */

//     acerpure_mqtt_task_alive = 1;
// }