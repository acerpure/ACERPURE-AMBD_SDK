#ifndef __EXAMPLE_ACERPURE_UART_ATCMD_H__
#define __EXAMPLE_ACERPURE_UART_ATCMD_H__

/******************************************************************************
 *
 * Copyright(c) 2007 - 2015 Realtek Corporation. All rights reserved.
 *
 *
 ******************************************************************************/
// #if CONFIG_EXAMPLE_ACERPURE_UART_ATCMD
#include "FreeRTOS.h"
#include "semphr.h"

/* UART Pinmux */
#define ACERPURE_UART_TX		PA_23
#define ACERPURE_UART_RX		PA_18
#define ACERPURE_UART_RTS		PA_22
#define ACERPURE_UART_CTS		PA_19

// acerpure uart settings 9600/n/8/1
#define ACERPURE_UART_BAUD_RATE 9600
#define ACERPURE_UART_PARITY 	ParityNone
#define ACERPURE_UART_DATA_BITS	8
#define ACERPURE_UART_STOP_BITS 1
#define ACERPURE_UART_FLOW_CTRL	FlowControlNone

// acerpure uart at command format related
#define ACERPURE_ATCMD_HEADER	0x55
#define ACERPURE_ATCMD_END		0x0d
#define ACERPURE_ATCMD_VERSION	0x01

#define ACERPURE_ATCMD_LENGTH_OFFSET	2
#define ACERPURE_ATCMD_COMMAND_OFFSET 	3
#define ACERPURE_ATCMD_PARAM_OFFSET 	4

#define ACERPURE_MAX_ATCMD_BUF 64

typedef enum {
	/* wifi to device */
	ACERPURE_ATCMD_REPORT_WIFI_STATE = 	0x01,
	ACERPURE_ATCMD_DEVICE_ACTION = 		0x02,
	ACERPURE_ATCMD_MULTIPLE_ACTIONS = 	0x03,
	/* device to wifi */
	ACERPURE_ATCMD_QUERY_WIFI_STATE = 	0xf0,
	ACERPURE_ATCMD_SETUP_WIFI = 		0xf1,
	ACERPURE_ATCMD_DEVICE_INFO = 		0xf2,
	ACERPURE_ATCMD_DEVICE_STATE = 		0xf3,
	ACERPURE_ATCMD_DEVICE_MONITOR = 	0xf4,
	ACERPURE_ATCMD_TEST_WIFI = 			0xf5,
	ACERPURE_ATCMD_DEVICE_MONITOR_1 = 	0xf6,
} ACERPURE_AT_CMD;

/* sub command of ACERPURE_ATCMD_DEVICE_ACTION */
typedef enum {
	ACERPURE_ACTION_POWER = 			0x01,
	ACERPURE_ACTION_AIR_PUR_SPEED =		0x02,
	ACERPURE_ACTION_AIR_CIR_SPEED =		0x03,
	ACERPURE_ACTION_AIR_PUR_ROTATE =	0x04,
	ACERPURE_ACTION_AIR_CIR_ROTATE =	0x05,
	ACERPURE_ACTION_AIR_DETECT_MODE =	0x06,
	ACERPURE_ACTION_KID_MODE =			0x07,
	ACERPURE_ACTION_SLEEP_MODE =		0x08,
	ACERPURE_ACTION_SHUTDOWN_TIMER =	0x09,
	ACERPURE_ACTION_MY_FAVORITE	=		0x0a,
	// extended action as below...
    ACERPURE_ACTION_UV =                0x0b,
    ACERPURE_ACTION_MAX = (ACERPURE_ACTION_UV + 1)
} ACERPURE_ACTION;

typedef enum {
	ACERPURE_AIR_DETECT_MODE_OFF = 		0x00,
	ACERPURE_AIR_DETECT_MODE_PM25 =		0x01,
	ACERPURE_AIR_DETECT_MODE_PM10 =		0x02
} ACERPURE_AIR_DETECT_MODE;

typedef enum {
	ACERPURE_WIFI_STATE_IDLE = 0,
	ACERPURE_WIFI_STATE_PAIRING,	// It is connecting to wifi AP
	ACERPURE_WIFI_STATE_PAIRED,		// It is connected to wifi AP
	ACERPURE_WIFI_STATE_CONNECTED   // It is connected to Cloud service
} ACERPURE_WIFI_STATE;

typedef struct _acerpure_atcmd_content
{
	unsigned char length;
	unsigned char command;
	unsigned char *param;
} acerpure_atcmd_content_t;

typedef struct _acerpuer_action
{
	unsigned char action;
	unsigned char param;
} acerpuer_action_t;

typedef struct _acerpure_device_info
{
	unsigned char model_name[16];
	unsigned char main_fw[12];
} acerpure_device_info_t;

typedef struct _acerpure_wifi_state
{
	unsigned char connection;	// refer to ACERPURE_WIFI_STATE
	unsigned char ota;
} acerpure_wifi_state_t;

#define DEVICE_STATE_COUNT
typedef struct _acerpure_device_state
{
	unsigned char power;
	unsigned char purifier_speed;
	unsigned char circulator_speed;
	unsigned char purifier_rotate;
	unsigned char circulator_rotate;
	unsigned char air_detect_mode;
	unsigned char kid_mode;
	unsigned char sleep_mode;
	unsigned char shutdown_timer;
	unsigned char filter_install;
	unsigned char filter_health_value;
	unsigned char filter_health_level;	// 0 indicator off, 1 indicator on
	unsigned char my_favor_icon;
	unsigned char uv;
} acerpure_device_state_t;

typedef struct _acerpure_wifi_test
{
	unsigned char ssid[16];
	unsigned char password[16];
} acerpure_wifi_test_t;

// for old AT command(ACERPURE_ATCMD_DEVICE_MONITOR) use
typedef enum {
	ACERPURE_MONITOR_TYPE_PM25 		= 0x01,
	ACERPURE_MONITOR_TYPE_PM10 		= 0x02,
	ACERPURE_MONITOR_TYPE_GAS  		= 0x03,
	ACERPURE_MONITOR_TYPE_FILTER 	= 0x04
} ACERPURE_MONITOR_TYPE;

typedef struct _acerpure_device_monitor
{
	unsigned short pm25_value;
	unsigned short pm10_value;
	unsigned short gas_value;
	unsigned short filter_health_value;
	unsigned short co2_value;
	unsigned char pm25_level;
	unsigned char pm10_level;
	unsigned char gas_level;
	unsigned char filter_health_level;
	unsigned char co2_level;
} acerpure_device_monitor_t;

void acerpure_uart_at_send_string(char *str);
void acerpure_uart_at_send_buf(u8 *buf, u32 len);
void acerpure_atcmd_send(u8 command, u8 *param, int param_length);
void example_acerpure_uart_atcmd(void);

// #endif //#if CONFIG_EXAMPLE_ACERPURE_UART_ATCMD
#endif //#ifndef __EXAMPLE_ACERPURE_UART_ATCMD_H__