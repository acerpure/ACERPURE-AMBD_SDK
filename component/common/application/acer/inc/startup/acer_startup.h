#include "FreeRTOS.h"

/*-----------------------------------------------------------*/
/**
 * @brief Application task startup hook for applications using Wi-Fi. If you are not
 * using Wi-Fi, then start network dependent applications in the vApplicationIPNetorkEventHook
 * function. If you are not using Wi-Fi, this hook can be disabled by setting
 * configUSE_DAEMON_TASK_STARTUP_HOOK to 0.
 */
void vApplicationDaemonTaskStartupHook( void );

static void prvDaemonTask( void * pvParameters );

/*-----------------------------------------------------------*/
/**
 * @brief Connects to Wi-Fi.
 */
static int prvWifiConnect( void );

static int prvConnectAP(const char *ssid, const char *password);

static int eraseFastconnectData( void );

/*-----------------------------------------------------------*/
/**
 * @brief Initializes the board.
 */
static void prvMiscInitialization( void );

/**
 * @brief Application runtime entry point.
 */
extern int acer_startup_main( void );
