#ifndef __ESP_CONFIG_OVERWRITE_H

#ifdef __cplusplus
extern "C"
{
#endif

// From sdkconfig.h
#ifndef CONFIG_FREERTOS_HZ
#define CONFIG_FREERTOS_HZ 100
#endif

// Task configurations
#define LED_UPDATER_TASK_STACK_DEPTH 2048

#define CAPACITIVE_SENSOR_TASK_STACK_DEPTH 2048

#define BUTTON_TASK_STACK_DEPTH 2048

#define CAPACITIVE_SENSOR_LED_UPDATE_DELAY 500

#define NETWORK_TASK_STACK_DEPTH 8192

// Task notifications
#ifndef configTASK_NOTIFICATION_ARRAY_ENTRIES
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 1
#else
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 1
#endif

#ifdef __cplusplus
}
#endif

#define __ESP_CONFIG_OVERWRITE_H
#endif // __ESP_CONFIG_OVERWRITE_H
