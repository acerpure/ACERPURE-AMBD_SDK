#ifndef _AWS_ACERPURE_H_
#define _AWS_ACERPURE_H_

typedef enum {
	AWS_DAEMON_STATE_IDLE = 0,		// try to connect AP
	AWS_DAEMON_STATE_PAIRING = 1,	// pairing process with mobile APP
	AWS_DAEMON_STATE_PAIRED = 2,	// connected with AP and try to connect AWS shadow
	AWS_DAEMON_STATE_CONNECTED = 3,	// connected with AWS shadow and do nothing
	AWS_DAEMON_STATE_TEST = 4		// test wifi module connection
} AWS_DAEMON_STATE;

typedef enum {
	AWS_OTA_STATE_IDLE = 0,
	AWS_OTA_STATE_UPGRADING = 1,
	AWS_OTA_STATE_ERROR = 2
} AWS_OTA_STATE;

// uint8_t device_alert, note!! only support 8 alerts 0~7
// please be careful...json string mapping value must be checked
typedef enum {
	AWS_ALERT_FILTER_HEALTH = 0,
	AWS_ALERT_FILTER_HEALTH_5 = 1,
	AWS_ALERT_FILTER_HEALTH_10 = 2,
	AWS_ALERT_FILTER_HEALTH_20 = 3,
	AWS_ALERT_FILTER_INSTALL = 4,
	AWS_ALERT_AQI = 5,
	AWS_ALERT_GAS = 6,
	AWS_ALERT_CO2 = 7
} AWS_ALERT;

int Acerpure_RunDaemon( void );

void acer_test_httpd(void);
void acer_test_mdns(void);

#endif /* _AWS_ACERPURE_H_ */