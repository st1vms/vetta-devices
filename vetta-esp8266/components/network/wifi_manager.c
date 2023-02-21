#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "tcpip_adapter.h"
#include "wifi_manager.h"

static unsigned char is_initialized = 0;
static wifi_mode_t current_mode;

static esp_err_t parse_sta_credentials(wifi_config_t *lamp_sta_config,
                                       const uint8_t *sta_ssid_str, size_t sta_ssid_size,
                                       const uint8_t *sta_pwd_str, size_t sta_pwd_size)
{

    if (!sta_ssid_str ||
        sta_ssid_size == 0 ||
        sta_ssid_size > MAX_SSID_LEN ||
        sta_pwd_size == 0 ||
        sta_pwd_size > MAX_PASSPHRASE_LEN)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(lamp_sta_config->sta.password, 0, MAX_PASSPHRASE_LEN * sizeof(uint8_t));
    memset(lamp_sta_config->sta.ssid, 0, MAX_SSID_LEN * sizeof(uint8_t));

    for (size_t i = 0; i < sta_ssid_size; i++)
    {
        lamp_sta_config->sta.ssid[i] = sta_ssid_str[i];
    }

    for (size_t i = 0; i < sta_pwd_size; i++)
    {
        lamp_sta_config->sta.password[i] = sta_pwd_str[i];
    }

    return ESP_OK;
}

static esp_err_t init_wifi(wifi_mode_t wifi_mode, esp_event_handler_t event_handler)
{
    static esp_err_t _err;

    if (is_initialized)
    {
        return ESP_OK;
    }

    tcpip_adapter_init();
    if (ESP_OK == (_err = esp_event_loop_create_default()))
    {
        wifi_init_config_t _init_cfg = WIFI_INIT_CONFIG_DEFAULT();
        if (ESP_OK != (_err = esp_wifi_init(&_init_cfg)) ||
            ESP_OK != (_err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL)) ||
            ESP_OK != (_err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL)) ||
            ESP_OK != (_err = esp_wifi_set_mode(wifi_mode)))
        {
            return _err;
        }
        current_mode = wifi_mode;
        // esp_wifi_set_ps(WIFI_PS_NONE);
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t init_wifi_ap(esp_event_handler_t event_handler)
{
    static esp_err_t _err;

    wifi_config_t lamp_ap_config = {
        .ap = {
            .ssid = LAMP_AP_SSID,
            .ssid_len = LAMP_AP_SSID_STRLEN,
            .max_connection = 1,
            .authmode = WIFI_AUTH_OPEN}};

    // lamp_ap_config.ap.authmode = WIFI_AUTH_MAX;
    if (ESP_OK != (_err = init_wifi(WIFI_MODE_AP, event_handler)) ||
        ESP_OK != (_err = esp_wifi_set_config(ESP_IF_WIFI_AP, &lamp_ap_config)) ||
        ESP_OK != (_err = esp_wifi_start()))
    {
        return _err;
    }

    tcpip_adapter_ip_info_t ip_info;

    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

    tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);

    tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info);

    tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);

    is_initialized = 1;

    return ESP_OK;
}

esp_err_t init_wifi_sta(esp_event_handler_t event_handler,
                        const uint8_t *sta_ssid_str, size_t sta_ssid_size,
                        const uint8_t *sta_pwd_str, size_t sta_pwd_size)
{
    static esp_err_t _err;

    wifi_config_t lamp_sta_config = {
        .sta = {
            .ssid = {0},
            .password = {0},
            .threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK}};

    if (ESP_OK != parse_sta_credentials(&lamp_sta_config, sta_ssid_str, sta_ssid_size, sta_pwd_str, sta_pwd_size))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (ESP_OK != (_err = init_wifi(WIFI_MODE_STA, event_handler)) ||
        ESP_OK != (_err = esp_wifi_set_config(ESP_IF_WIFI_STA, &lamp_sta_config)) ||
        ESP_OK != (_err = esp_wifi_start()))
    {
        return _err;
    }

    is_initialized = 1;

    return ESP_OK;
}

esp_err_t deinit_wifi(esp_event_handler_t event_handler)
{

    if (!is_initialized)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (current_mode == WIFI_MODE_AP)
    {
        esp_wifi_deauth_sta(0);
        tcpip_adapter_dhcps_stop(WIFI_IF_AP);
    }

    static esp_err_t _err;
    if (ESP_OK != (_err = esp_wifi_stop()) ||
        ESP_OK != (_err = esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, event_handler)) ||
        ESP_OK != (_err = esp_event_loop_delete_default()) ||
        ESP_OK != (_err = esp_wifi_deinit()))
    {
        return _err;
    }

    is_initialized = 0;
    return ESP_OK;
}
