#ifndef __WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event_base.h"
#include "nvs_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

// Wifi configurations
#ifndef CONFIG_ESP8266_WIFI_RX_BUFFER_NUM
#define CONFIG_ESP8266_WIFI_RX_BUFFER_NUM 15
#endif

#ifndef CONFIG_ESP8266_WIFI_RX_PKT_NUM
#define CONFIG_ESP8266_WIFI_RX_PKT_NUM 6
#endif

#ifndef CONFIG_ESP8266_WIFI_LEFT_CONTINUOUS_RX_BUFFER_NUM
#define CONFIG_ESP8266_WIFI_LEFT_CONTINUOUS_RX_BUFFER_NUM 15
#endif

#ifndef CONFIG_ESP8266_WIFI_TX_PKT_NUM
#define CONFIG_ESP8266_WIFI_TX_PKT_NUM 5
#endif

esp_err_t init_wifi(wifi_mode_t wifi_mode, esp_event_handler_t wifi_event_handler);

#ifdef __cplusplus
}
#endif

#define __WIFI_MANAGER_H
#endif // __WIFI_MANAGER_H
