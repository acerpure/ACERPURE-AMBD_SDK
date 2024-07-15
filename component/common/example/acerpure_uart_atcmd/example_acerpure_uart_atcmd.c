/******************************************************************************
 *
 * Copyright(c) 2007 - 2015 Realtek Corporation. All rights reserved.
 *
 *
 ******************************************************************************/
#include <platform_opts.h>
#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>
#include "semphr.h"
#include "device.h"
#include "serial_api.h"
#include "acerpure_uart_atcmd/example_acerpure_uart_atcmd.h"
// #include "example_acerpure_uart_atcmd.h"
#include "flash_api.h"
#include "device_lock.h"
#if defined(configUSE_WAKELOCK_PMU) && (configUSE_WAKELOCK_PMU == 1)
#include "freertos_pmu.h"
#endif
#include "osdep_service.h"
#include "serial_ex_api.h"
#include "pinmap.h"

#include "aws_acerpure.h"
#include "flash_util.h"
extern AWS_OTA_STATE g_aws_ota;
extern AWS_DAEMON_STATE g_aws_state;
extern ACERPURE_WIFI_STATE g_wifi_state;
acerpure_device_info_t acerpure_device_info;
acerpure_device_state_t acerpure_device_state;
acerpure_device_monitor_t acerpure_device_monitor;
acerpure_wifi_test_t acerpure_wifi_test;
int new_monitor_format = 0;	// default : use old format
int g_extended_model = 0; // for C3 model(add uv, co2...)
int g_need_update_mainboard_info = 0;
int g_start_wifi_pair = 0;
int g_start_wifi_test = 0;
extern struct user_aws_device_info g_aws_device_info;

#if CONFIG_EXAMPLE_ACERPURE_UART_ATCMD

// typedef int (*init_done_ptr)(void);
// extern init_done_ptr p_wlan_init_done_callback;
// extern char log_buf[LOG_SERVICE_BUFLEN];
// extern xSemaphoreHandle log_rx_interrupt_sema;
// extern void serial_rx_fifo_level(serial_t *obj, SerialFifoLevel FifoLv);
// extern int atcmd_wifi_restore_from_flash(void);
// extern int atcmd_lwip_restore_from_flash(void);
xSemaphoreHandle atcmd_update_info_sema = NULL;

serial_t at_cmd_sobj;
// char at_string[ATSTRING_LEN];
//_sema at_printf_sema;
// _sema uart_at_dma_tx_sema;
// unsigned char gAT_Echo = 1; // default echo on

// #define UART_AT_MAX_DELAY_TIME_MS   20
extern int device_state_report;
extern int device_monitor_report;
extern uint8_t alert_test;
// int device_state_report;
// int device_monitor_report;
// uint8_t alert_test;

void acerpure_uart_at_send_string(char *str)
{
	unsigned int i = 0;
	while (str[i] != '\0')
	{
		serial_putc(&at_cmd_sobj, str[i]);
		i++;
	}
}

void acerpure_uart_at_send_buf(u8 *buf, u32 len)
{
	unsigned char *st_p = buf;
	if (!len || !buf)
	{
		return;
	}

	while (len)
	{
		serial_putc(&at_cmd_sobj, *st_p);
		st_p++;
		len--;
	}
}

static unsigned char temp_buf[ACERPURE_MAX_ATCMD_BUF] = "\0";
static unsigned int buf_count = 0;
static acerpure_atcmd_content_t acerpure_atcmd_content = {0};
static unsigned char length = 0;
static unsigned char checksum = 0;
static bool acerpure_atcmd_header = _TRUE;
static bool acerpure_atcmd_version = _TRUE;
static bool acerpure_atcmd_length = _TRUE;
static bool acerpure_atcmd_command = _TRUE;
static bool acerpure_atcmd_params = _TRUE;
static bool acerpure_atcmd_checksum = _TRUE;
static bool acerpure_atcmd_end = _TRUE;

void acerpure_atcmd_send(u8 command, u8 *param, int param_length)
{
	int i = 0;
	unsigned char checksum = 0;
	u8 buf[ACERPURE_MAX_ATCMD_BUF] = {0};

	buf[0] = ACERPURE_ATCMD_HEADER;
	buf[1] = ACERPURE_ATCMD_VERSION;
	buf[2] = param_length + 1;
	buf[3] = command;
	// parameter(s)
	for (int i = 0; i < param_length; i++)
	{
		buf[ACERPURE_ATCMD_PARAM_OFFSET + i] = *(param + i);
	}

	for (int i = 0; i < param_length + 2; i++) // +2 checksum from length to command and to parameters
	{
		checksum += buf[ACERPURE_ATCMD_LENGTH_OFFSET + i];
	}
	buf[ACERPURE_ATCMD_PARAM_OFFSET + param_length] = checksum;

	buf[ACERPURE_ATCMD_PARAM_OFFSET + param_length + 1] = ACERPURE_ATCMD_END;

	/* TEST */
	configPRINTF( ( "acerpure_atcmd_send, total len: %d\r\n", param_length + 6 ) );

	acerpure_uart_at_send_buf(buf, param_length + 6); // 5: header, version, length, command, checksum, end
}

void reset_acerpure_atcmd_parser()
{
	buf_count = 0;
	length = 0;
	checksum = 0;
	acerpure_atcmd_header = _TRUE;
	acerpure_atcmd_version = _TRUE;
	acerpure_atcmd_length = _TRUE;
	acerpure_atcmd_command = _TRUE;
	acerpure_atcmd_params = _TRUE;
	acerpure_atcmd_checksum = _TRUE;
	memset(&temp_buf, 0, sizeof(temp_buf));
}

int start_acerpure_atcmd_parser()
{
	printf("Start parsing ACERPURE AT command: 0x%X\r\n", acerpure_atcmd_content.command);
	int i = 0;

	switch (acerpure_atcmd_content.command)
	{
		case ACERPURE_ATCMD_QUERY_WIFI_STATE:
		{
			acerpure_wifi_state_t wifi_state;
			wifi_state.connection = (unsigned char)g_wifi_state;
			wifi_state.ota = (unsigned char)g_aws_ota;
			acerpure_atcmd_send(ACERPURE_ATCMD_REPORT_WIFI_STATE,
								(unsigned char *)&wifi_state, sizeof(wifi_state));

			printf("#### AT COMMAND RECEIVED: Query wifi state: %d, ota state: %d\r\n",
					wifi_state.connection, wifi_state.ota);
			break;
		}
		case ACERPURE_ATCMD_SETUP_WIFI:
		{
			printf("#### AT COMMAND RECEIVED: Setup wifi!\r\n");

			if (AWS_DAEMON_STATE_PAIRING != g_aws_state)
			{
				g_start_wifi_pair = 1;
			}

			break;
		}
		case ACERPURE_ATCMD_DEVICE_INFO:
		{
			printf("#### AT COMMAND RECEIVED: Device information!\r\n");

			memset(&acerpure_device_info, 0, sizeof(acerpure_device_info));

			rtw_memcpy((void *)acerpure_device_info.model_name,
							(void *)acerpure_atcmd_content.param,
							sizeof(acerpure_device_info.model_name));

			rtw_memcpy((void *)acerpure_device_info.main_fw,
						(void *)(acerpure_atcmd_content.param + sizeof(acerpure_device_info.model_name)),
						sizeof(acerpure_device_info.main_fw));

			printf("Model name: %s, Main firmware version: %s\r\n",
					acerpure_device_info.model_name,
					acerpure_device_info.main_fw);

			rtw_up_sema_from_isr(&atcmd_update_info_sema);

			break;
		}
		case ACERPURE_ATCMD_DEVICE_STATE:
		{
			printf("#### AT COMMAND RECEIVED: Device state!\r\n");

            // origin state num: 13 states
            if (acerpure_atcmd_content.length > 14) // 13 + 1 command byte
            {
                g_extended_model = 1;
            }
            else
            {
                g_extended_model = 0;
                // set extended model state to N/A
                acerpure_device_state.uv = 0xff;    // N/A
            }
			// rtw_memcpy((void *)&acerpure_device_state,
			// 			(void *)acerpure_atcmd_content.param,
			// 			sizeof(acerpure_device_state));
            // add uv, use length for compatible
            rtw_memcpy((void *)&acerpure_device_state,
                        (void *)acerpure_atcmd_content.param,
                        acerpure_atcmd_content.length - 1);   // length = 1 byte command + n bytes param

			printf("\r\n======== Device state start ========\r\n");
			for (int i = 0; i < sizeof(acerpure_device_state); i++)
			{
				printf("%02X ", *(acerpure_atcmd_content.param + i));
			}
			printf("\r\n======== Device state end ========\r\n");

#if 0
			// hardcoded filter install to test
			if (alert_test)
				acerpure_device_state.filter_install = 0;
			else
				acerpure_device_state.filter_install = 1;

			// hardcoded filter health test
			if (alert_test)
				acerpure_device_state.filter_health_value = 6;
			//else
				//acerpure_device_state.filter_health_value = 60;
#endif

			device_state_report = 1;

			break;
		}
        // old monitor AT command: ACERPURE_ATCMD_DEVICE_MONITOR!
        // use ACERPURE_ATCMD_DEVICE_MONITOR_1 instead
		case ACERPURE_ATCMD_DEVICE_MONITOR:
		{
			printf("#### AT COMMAND RECEIVED: Device monitor!\r\n");

			unsigned char *ptr = acerpure_atcmd_content.param;

			ACERPURE_MONITOR_TYPE type = *ptr;
			unsigned short value;
			value = ((*(ptr + 1) << 8) | (*(ptr + 2))) & 0xffff;
			unsigned char level;
			level = *(ptr + 3) & 0xff;

			printf("Device monitor type: %d, value: %d, level: %d\r\n",
					type, value, level);

			if (ACERPURE_MONITOR_TYPE_PM25 == type)
			{
				acerpure_device_monitor.pm25_value = value;
				acerpure_device_monitor.pm25_level = level;

				// publish pm2.5, pm1.0, gas to aws while pm2.5 update
				// if (acerpure_device_monitor.pm10_value != 0 && acerpure_device_monitor.pm25_value != 0)
				{
					device_monitor_report = 1;
				}
			}
			else if (ACERPURE_MONITOR_TYPE_PM10 == type)
			{
				acerpure_device_monitor.pm10_value = value;
				acerpure_device_monitor.pm10_level = level;
			}
			else if (ACERPURE_MONITOR_TYPE_GAS == type)
			{
				acerpure_device_monitor.gas_value = value;
				acerpure_device_monitor.gas_level = level;
			}
			else if (ACERPURE_MONITOR_TYPE_FILTER == type)
			{
				acerpure_device_monitor.filter_health_value = value;
				acerpure_device_monitor.filter_health_level = level;
			}
			else
			{
				printf("Invalid ACERPURE monitor type: 0x%X\r\n", type);
				return -1;
			}

			break;
		}
		case ACERPURE_ATCMD_TEST_WIFI:
		{
			memset(&acerpure_wifi_test, 0, sizeof(acerpure_wifi_test));

			rtw_memcpy((void *)acerpure_wifi_test.ssid,
							(void *)acerpure_atcmd_content.param,
							sizeof(acerpure_wifi_test.ssid));

			rtw_memcpy((void *)acerpure_wifi_test.password,
						(void *)(acerpure_atcmd_content.param + sizeof(acerpure_wifi_test.ssid)),
						sizeof(acerpure_wifi_test.password));

			printf("Test AP ssid: %s, password: %s\r\n",
					acerpure_wifi_test.ssid,
					acerpure_wifi_test.password);

			// if (AWS_DAEMON_STATE_TEST != g_aws_state)
			// {
			// 	g_start_wifi_test = 1;
			// }

			break;
		}
		case ACERPURE_ATCMD_DEVICE_MONITOR_1:
		{
			printf("#### AT COMMAND RECEIVED: Device monitor(new)!\r\n");

			unsigned char *ptr = acerpure_atcmd_content.param;

			acerpure_device_monitor.pm25_value = ((*ptr << 8) | (*(ptr + 1))) & 0xffff;
			acerpure_device_monitor.pm25_level = *(ptr + 2) & 0xff;

			acerpure_device_monitor.pm10_value = ((*(ptr + 3) << 8) | (*(ptr + 4))) & 0xffff;
			acerpure_device_monitor.pm10_level = *(ptr + 5) & 0xff;

			acerpure_device_monitor.gas_value = ((*(ptr + 6) << 8) | (*(ptr + 7))) & 0xffff;
			acerpure_device_monitor.gas_level = *(ptr + 8) & 0xff;

            // add CO2, monitor values in previous model: + 3(pm2.5) + 3(pm1.0) + 3(gas)
            // if (acerpure_atcmd_content.length > 10) // +1 for command byte
            if (g_extended_model)
            {
                acerpure_device_monitor.co2_value = ((*(ptr + 9) << 8) | (*(ptr + 10))) & 0xffff;
                acerpure_device_monitor.co2_level = *(ptr + 11) & 0xff;
            }
            else
            {
                acerpure_device_monitor.co2_value = 0;
                acerpure_device_monitor.co2_level = 0;  // 'N/A'
            }
#if 0
			// hardcoded aqi to test
			if (alert_test)
			{
				acerpure_device_monitor.pm25_level = 3;
				acerpure_device_monitor.pm10_level = 1;
			}
			else
			{
				acerpure_device_monitor.pm25_level = 1;
				acerpure_device_monitor.pm10_level = 1;
			}

			// hardcoded gas test
			if (alert_test)
				acerpure_device_monitor.gas_level = 3;
			else
				acerpure_device_monitor.gas_level = 1;
#endif

			printf("Device monitor pm2.5 'value: %d, level: %d', pm1.0 'value: %d, level: %d', gas 'value: %d, level: %d', co2 'value: %d, level: %d'\r\n",
					acerpure_device_monitor.pm25_value, acerpure_device_monitor.pm25_level,
					acerpure_device_monitor.pm10_value, acerpure_device_monitor.pm10_level,
					acerpure_device_monitor.gas_value, acerpure_device_monitor.gas_level,
                    acerpure_device_monitor.co2_value, acerpure_device_monitor.co2_level
					);

			new_monitor_format = 1;	// new at command format for monitor
			device_monitor_report = 1;

			break;
		}
		default:
		{
			printf("Invalid ACERPURE AT command: 0x%X\r\n", acerpure_atcmd_content.command);
			return -1;
		}
	}

	return 0;
}

void acerpure_uart_irq(uint32_t id, SerialIrq event)
{
	serial_t *sobj = (serial_t *)id;
	unsigned char rc = 0;

	if (event == RxIrq)
	{
		// protection!
		if (buf_count == (ACERPURE_MAX_ATCMD_BUF - 1))
		{
			printf("ERROR: exceed size limit\r\n");
			reset_acerpure_atcmd_parser();
			return;
		}

		rc = serial_getc(sobj);
		temp_buf[buf_count] = rc;

		if (acerpure_atcmd_header)
		{
			if (ACERPURE_ATCMD_HEADER != temp_buf[buf_count])
			{
				reset_acerpure_atcmd_parser();
				return;
			}
			else
			{
				buf_count++;
				acerpure_atcmd_header = _FALSE;
				return;
			}
		}

		if (acerpure_atcmd_version)
		{
			if (ACERPURE_ATCMD_VERSION != temp_buf[buf_count])
			{
				printf("Wrong AT command version!\r\n");
				reset_acerpure_atcmd_parser();
				return;
			}
			else
			{
				buf_count++;
				acerpure_atcmd_version = _FALSE;
				return;
			}
		}

		if (acerpure_atcmd_length)
		{
			if (0 == temp_buf[buf_count])
			{
				printf("Invalid AT command length!\r\n");
				reset_acerpure_atcmd_parser();
				return;
			}
			else
			{
				acerpure_atcmd_content.length = temp_buf[buf_count];
				checksum = acerpure_atcmd_content.length;
				length = acerpure_atcmd_content.length;
				buf_count++;
				acerpure_atcmd_length = _FALSE;
				return;
			}
		}

		if (acerpure_atcmd_command)
		{
			acerpure_atcmd_content.command = temp_buf[buf_count];
			checksum += acerpure_atcmd_content.command;
			length--;
			buf_count++;
			acerpure_atcmd_command = _FALSE;

			if (0 == length)	// no parameter
			{
				acerpure_atcmd_content.param = NULL;
				acerpure_atcmd_params = _FALSE;
			}
			return;
		}

		if (acerpure_atcmd_params)
		{
			checksum += temp_buf[buf_count];
			buf_count++;
			length--;

			if (0 == length)	// no more parameter
			{
				acerpure_atcmd_content.param = &temp_buf[ACERPURE_ATCMD_PARAM_OFFSET];
				acerpure_atcmd_params = _FALSE;
			}
			return;
		}

		if (acerpure_atcmd_checksum)
		{
			if (checksum != temp_buf[buf_count])
			{
				printf("Invalid ACERPURE AT command checksum! NEED %d, but %d\r\n",
						checksum, temp_buf[buf_count]);
				// print more debug message
				printf("\r\n======== Print Invalid command for debug ========\r\n");
				for (int i = 0; i <= buf_count; i++)
				{
					printf("%02X ", temp_buf[i]);
				}
				printf("\r\n======== Print Invalid command for debug ========\r\n");

				reset_acerpure_atcmd_parser();
				return;
			}
			else
			{
				buf_count++;
				acerpure_atcmd_checksum = _FALSE;
				return;
			}
		}

		// finally, check end code
		if (ACERPURE_ATCMD_END != temp_buf[buf_count])
		{
			printf("Invalid ACERPURE AT command end code!\r\n");
		}
		else
		{
			if (start_acerpure_atcmd_parser() < 0)
			{
				printf("ACERPURE AT command parser failed!\r\n");
			}
		}
		reset_acerpure_atcmd_parser();

		return;
	}
}

void acerpure_uart_atcmd_main(void)
{
	// init acerpure device data
	memset(&acerpure_device_info, 0, sizeof(acerpure_device_info));
	// strncpy(acerpure_device_info.model_name, DEFAULT_MODEL_NAME, sizeof(acerpure_device_info.model_name));
	// strncpy(acerpure_device_info.main_fw, DEFAULT_MAIN_FW, sizeof(acerpure_device_info.main_fw));
	// init device state
	memset(&acerpure_device_state, 0, sizeof(acerpure_device_state));
	// init device monitor
	memset(&acerpure_device_monitor, 0, sizeof(acerpure_device_monitor));

	// 9600/n/8/1
	serial_init(&at_cmd_sobj, ACERPURE_UART_TX, ACERPURE_UART_RX);
	serial_baud(&at_cmd_sobj, ACERPURE_UART_BAUD_RATE);
	serial_format(&at_cmd_sobj, ACERPURE_UART_DATA_BITS, ACERPURE_UART_PARITY, ACERPURE_UART_STOP_BITS);
	serial_rx_fifo_level(&at_cmd_sobj, FifoLvHalf);

	// disable flow control
	serial_set_flow_control(&at_cmd_sobj, FlowControlNone, ACERPURE_UART_RTS, ACERPURE_UART_CTS);

	serial_irq_handler(&at_cmd_sobj, acerpure_uart_irq, (uint32_t)&at_cmd_sobj);
	serial_irq_set(&at_cmd_sobj, RxIrq, 1);

	printf("Start ACERPURE AT command service: uart config(%d/%d/%d/%d/%d)\r\n",
			ACERPURE_UART_BAUD_RATE,
			ACERPURE_UART_PARITY,
			ACERPURE_UART_DATA_BITS,
			ACERPURE_UART_STOP_BITS,
			ACERPURE_UART_FLOW_CTRL);

	while (1)
	{
		while (xSemaphoreTake(atcmd_update_info_sema, portMAX_DELAY) != pdTRUE);

		// restore to flash if mismatch
		if (strcmp(g_aws_device_info.model_name, acerpure_device_info.model_name) ||
			strcmp(g_aws_device_info.main_fw, acerpure_device_info.main_fw))
		{
			g_need_update_mainboard_info = 1;

			rtw_memcpy((void *)g_aws_device_info.model_name,
						(void *)acerpure_device_info.model_name,
						sizeof(g_aws_device_info.model_name));

			rtw_memcpy((void *)g_aws_device_info.main_fw,
						(void *)acerpure_device_info.main_fw,
						sizeof(g_aws_device_info.main_fw));

			// move to aws daemon, restore to flash after publishing to aws
			// user_data_partition_ops(USER_PARTITION_AWS_DEVICE_INFO, USER_PARTITION_WRITE,
			// 					(u8 *)&g_aws_device_info, sizeof(struct user_aws_device_info));
		}
		printf("\n\r[MEM] After do cmd, available heap %d\n\r", xPortGetFreeHeapSize());
	}
}

static void acerpure_uart_atcmd_thread(void *param)
{
	acerpure_uart_atcmd_main();
	vTaskDelete(NULL);
}

void example_acerpure_uart_atcmd(void)
{
	/* Initial uart rx swmaphore*/
	vSemaphoreCreateBinary(atcmd_update_info_sema);
	xSemaphoreTake(atcmd_update_info_sema, 1/portTICK_RATE_MS);

	if (xTaskCreate(acerpure_uart_atcmd_thread,
					((const char*)"acerpure_uart_atcmd_thread"),
					1024,
					NULL,
					tskIDLE_PRIORITY + 1,
					NULL) != pdPASS)
	{
		printf("\n\r%s xTaskCreate(acerpure_uart_atcmd_thread) failed", __FUNCTION__);
	}
}
#endif