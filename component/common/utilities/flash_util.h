#ifndef __FLASH_UTIL_H__
#define __FLASH_UTIL_H__

#include "basic_types.h"
#include "wifi_structures.h"
// #include <wlan_fast_connect/example_wlan_fast_connect.h>

// #define ATCMD_WIFI_CONN_STORE_MAX_NUM (1)
struct user_wifi_conf
{
	// int32_t auto_enable;
	rtw_wifi_setting_t setting;
	// int32_t reconn_num;
	// int32_t reconn_last_index;
	// struct wlan_fast_reconnect reconn[ATCMD_WIFI_CONN_STORE_MAX_NUM];
};

struct user_aws_mqtt_conf
{
	unsigned char mqtt_broker_endpoint[256];
	unsigned char thing_name[32];
	unsigned char sub_topic[32];
	unsigned char send_topic[32];
	unsigned char client_id[32];
};

struct user_aws_device_info
{
	unsigned char model_name[16];
	unsigned char main_fw[12];		// mainboard fw
	unsigned char comm_fw[12];		// wifi module fw
};

struct user_aws_alert_conf
{
	uint32_t filter_install;
	uint32_t filter_health_20;
	uint32_t filter_health_10;
	uint32_t filter_health_5;
	uint32_t filter_health;
	uint32_t aqi;
	uint32_t gas;
	uint32_t co2;
};

// struct user_aws_root_ca_conf
// {
// 	char cert[1024];
// };

struct user_aws_client_ca_conf
{
	char cert[2048];
};

struct user_aws_key_conf
{
	char key[2048];
};

typedef enum {
	USER_PARTITION_WIFI = 0,
	USER_PARTITION_AWS_MQTT,
	USER_PARTITION_AWS_DEVICE_INFO,
	USER_PARTITION_AWS_ALERT,
	USER_PARTITION_AWS_ROOT_CA,
	USER_PARTITION_AWS_KEY,
	USER_PARTITION_AWS_CLIENT_CA,
} USER_PARTITION;

typedef enum {
	USER_PARTITION_READ = 0,
	USER_PARTITION_WRITE = 1,
	USER_PARTITION_ERASE = 2
} USER_PARTITION_OP;

#define FLASH_BACKUP_SECTOR		(0x8000)

// first sector in user data
// #define WIFI_SETTING_SECTOR			0x000F5000
#define WIFI_SETTING_SECTOR				0x08100000
// first segment for wifi config
#define WIFI_CONF_DATA_OFFSET			(0)
#define WIFI_CONF_DATA_SIZE				((((sizeof(struct user_wifi_conf)-1)>>2) + 1)<<2)

// second sector in user data
// #define AWS_MQTT_SETTING_SECTOR		0x000F6000
#define AWS_MQTT_SETTING_SECTOR			0x08101000
// first segment for aws mqtt config
#define AWS_MQTT_CONF_DATA_OFFSET		(0)
#define AWS_MQTT_CONF_DATA_SIZE			((((sizeof(struct user_aws_mqtt_conf)-1)>>2) + 1)<<2)

// third sector in user data
// first segment for device info
// #define AWS_DEVICE_INFO_SECTOR		0x000F7000
#define AWS_DEVICE_INFO_SECTOR			0x08102000
#define AWS_DEVICE_INFO_DATA_OFFSET		(0)
#define AWS_DEVICE_INFO_DATA_SIZE		((((sizeof(struct user_aws_device_info)-1)>>2) + 1)<<2)

// 4th sector in user data
// first segment for alert config
// #define AWS_ALERT_SETTING_SECTOR		0x000F8000
#define AWS_ALERT_SETTING_SECTOR		0x08103000
#define AWS_ALERT_CONF_DATA_OFFSET		(0)
#define AWS_ALERT_CONF_DATA_SIZE		((((sizeof(struct user_aws_alert_conf)-1)>>2) + 1)<<2)

// 5th sector in user data
// #define AWS_ROOT_CA_SETTING_SECTOR  	0x000F9000
// #define AWS_ROOT_CA_SETTING_SECTOR		0x08104000
// first segment for aws root ca config
// #define AWS_ROOT_CA_CONF_DATA_OFFSET	(0)
// #define AWS_ROOT_CA_CONF_DATA_SIZE		((((sizeof(struct user_aws_root_ca_conf)-1)>>2) + 1)<<2)

// 6th sector in user data
// #define AWS_KEY_SETTING_SECTOR  		0x000FA000
#define AWS_KEY_SETTING_SECTOR 			0x08105000
// first segment for aws ca config
#define AWS_KEY_CONF_DATA_OFFSET		(0)
#define AWS_KEY_CONF_DATA_SIZE	        ((((sizeof(struct user_aws_key_conf)-1)>>2) + 1)<<2)

// 7th sector in user data
// #define AWS_CLIENT_CA_SETTING_SECTOR	0x000FB000
#define AWS_CLIENT_CA_SETTING_SECTOR 	0x08106000
// first segment for aws ca config
#define AWS_CLIENT_CA_CONF_DATA_OFFSET	(0)
#define AWS_CLIENT_CA_CONF_DATA_SIZE	((((sizeof(struct user_aws_client_ca_conf)-1)>>2) + 1)<<2)

void user_data_partition_ops(USER_PARTITION id, USER_PARTITION_OP ops, u8 *data, u16 len);

#endif /* __FLASH_UTIL_H__ */