#define DEFAULT_CHANLIST        0x07FFF800
#define SECURE                  1
#define TC_LINKKEY_JOIN
#define NV_INIT
#define NV_RESTORE
#define POWER_SAVING
#define NWK_AUTO_POLL
#define ZTOOL_P2
#define MT_UART_DEFAULT_OVERFLOW        FALSE
#define HAL_UART_DMA    0       // Disable P2 UART DMA to Enable P2 UART ISR
#define MT_TASK
#define MT_APP_FUNC
#define MT_SYS_FUNC
#define MT_DEBUG_FUNC
#define MT_ZDO_FUNC
#define MT_ZDO_MGMT
#define MT_APP_CNF_FUNC
#define ENABLE_MT_SYS_RESET_SHUTDOWN
//#define LCD_SUPPORTED   DEBUG
#define MULTICAST_ENABLED       FALSE
#define HAL_LCD                 FALSE
#define HAL_LED3_ENABLE
#define HAL_LED4_ENABLE
#define HAL_LED5_ENABLE
#define HAL_PUSH1_ENABLE
#define LED_POLARITY            ACTIVE_LOW
#define PUSH_POLARITY           ACTIVE_LOW
#define ISR_KEYINTERRUPT
#define ZCL_READ
#define ZCL_DISCOVER
#define ZCL_WRITE
#define ZCL_BASIC
#define ZCL_IDENTIFY
//#define ZCL_ON_OFF
//#define ZCL_GROUPS
#define ZCL_REPORT
#define ZCL_TIME
#define ZCL_POWER_CONFIG
#define ZCL_ALARMS
//#define BDB_REPORTING
#define ZCL_REPORTING_DEVICE
//#define ZCL_REPORT_DESTINATION_DEVICE   // to enable BDB report receiving/processing functionality
//#define DPRINTF_OSALHEAPTRACE
#define OSALMEM_METRICS         TRUE
#define OSAL_TOTAL_MEM