#ifndef __WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C"
{
#endif

// esp-wifi configurations
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

// AP/Station configuration

#define LAMP_AP_SSID "VettaV1"

#define LAMP_AP_SSID_STRLEN (7UL)

    esp_err_t init_wifi_ap(esp_event_handler_t event_handler);
    esp_err_t init_wifi_sta(esp_event_handler_t event_handler, const uint8_t *sta_ssid_str, size_t sta_ssid_size, const uint8_t *sta_pwd_str, size_t sta_pwd_size);

    esp_err_t deinit_wifi(esp_event_handler_t event_handler);

#ifdef __cplusplus
}
#endif

#define __WIFI_MANAGER_H
#endif // __WIFI_MANAGER_H
