#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "tcpip_adapter.h"
#include "wifi_manager.h"

static unsigned char is_initialized = 0;
static wifi_mode_t current_mode;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    static EventGroupHandle_t network_event_group;
    network_event_group = (EventGroupHandle_t) arg;

    if (event_id == WIFI_EVENT_AP_STACONNECTED) {

        // Station connected to lamp AP
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        printf("\nStation "MACSTR" join, AID=%d\n",MAC2STR(event->mac), event->aid);

        if(network_event_group){
            xEventGroupSetBits(network_event_group, WIFI_AP_STA_CONNECTED_BIT);
        }
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {

        // Station disconnected from lamp AP
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        printf("\nStation "MACSTR" leave, AID=%d\n",MAC2STR(event->mac), event->aid);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // Lamp starting station connection to home AP
        printf("\nWIFI_EVENT_STA_START\n");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // Lamp station disconnected from home AP
        printf("\nWIFI_EVENT_STA_DISCONNECTED\n");
        // esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        // Lamp station received an IP address from home AP
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("\nIP_EVENT_STA_GOT_IP -> ip is %s\n", ip4addr_ntoa(&event->ip_info.ip));

        if(network_event_group){
            xEventGroupSetBits(network_event_group, WIFI_STA_CONNECTED_BIT);
        }
    }
}


static esp_err_t parse_ap_pwd(wifi_config_t * lamp_ap_config, const uint8_t *ap_pwd_str, size_t ap_pwd_size)
{

    if (!ap_pwd_str ||
        ap_pwd_size == 0 ||
        ap_pwd_size > MAX_PASSPHRASE_LEN)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(lamp_ap_config->ap.password, 0, MAX_PASSPHRASE_LEN);

    const uint8_t *tmp = ap_pwd_str;
    uint8_t *p = lamp_ap_config->ap.password;
    while (NULL != tmp && NULL != p && ap_pwd_size > 0)
    {
        *p = *tmp;
        tmp++;
        ap_pwd_size--;
    }

    if (ap_pwd_size != 0)
    {
        memset(lamp_ap_config->ap.password, 0, MAX_PASSPHRASE_LEN);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t parse_sta_credentials(wifi_config_t * lamp_sta_config, const uint8_t *sta_ssid_str, size_t sta_ssid_size, const uint8_t *sta_pwd_str, size_t sta_pwd_size)
{

    if (!sta_ssid_str ||
        sta_ssid_size == 0 ||
        sta_ssid_size > MAX_SSID_LEN ||
        sta_pwd_size > MAX_PASSPHRASE_LEN)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(lamp_sta_config->sta.password, 0, MAX_PASSPHRASE_LEN);
    memset(lamp_sta_config->sta.ssid, 0, MAX_SSID_LEN);

    const uint8_t *stmp = sta_ssid_str;
    uint8_t *p = lamp_sta_config->sta.password, *s = lamp_sta_config->sta.ssid;

    if(NULL != sta_pwd_str && sta_pwd_size > 0){
        const uint8_t *ptmp = sta_pwd_str;
        lamp_sta_config->sta.threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK;

        // Parse/Copy Station password
        while (NULL != ptmp && NULL != p && sta_pwd_size-- > 0)
        {
            *p = *ptmp;
            ptmp++;
        }
        if (sta_pwd_size != 0)
        {
            memset(lamp_sta_config->sta.password, 0, MAX_PASSPHRASE_LEN);
            return ESP_FAIL;
        }
    }

    // Parse/Copy Station SSID
    while (NULL != stmp && NULL != s && sta_ssid_size-- > 0)
    {
        if (*stmp)
        {
            *s = *stmp;
            stmp++;
        }
        else
        {
            memset(lamp_sta_config->sta.ssid, 0, MAX_SSID_LEN);
            return ESP_FAIL;
        }
    }
    if (sta_ssid_size != 0)
    {
        memset(lamp_sta_config->sta.ssid, 0, MAX_SSID_LEN);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t init_wifi(wifi_mode_t wifi_mode, EventGroupHandle_t network_event_group)
{
    static esp_err_t _err;

    tcpip_adapter_init();
    wifi_init_config_t _init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (ESP_OK != (_err = esp_wifi_init(&_init_cfg)))
    {
        if(ESP_OK != (_err = esp_event_loop_create_default()) ||
            ESP_OK != (_err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, network_event_group)) ||
            ESP_OK != (_err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, network_event_group)) ||
            ESP_OK != (_err = esp_wifi_set_mode(wifi_mode)))
        {
            return _err;
        }
        current_mode = wifi_mode;
        esp_wifi_set_ps(WIFI_PS_NONE);
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t init_wifi_ap(EventGroupHandle_t network_event_group, const uint8_t *ap_pwd_str, size_t ap_pwd_size)
{

    if (is_initialized && WIFI_MODE_AP == current_mode)
    {
        return ESP_OK;
    }

    wifi_config_t lamp_ap_config = {
        .ap = {
            .ssid = LAMP_AP_SSID,
            .ssid_len = LAMP_AP_SSID_STRLEN,
            .max_connection = 1,
            .authmode = WIFI_AUTH_OPEN}
    };

    static esp_err_t _err;

    if (ESP_OK != parse_ap_pwd(&lamp_ap_config, ap_pwd_str, ap_pwd_size))
    {
        return ESP_ERR_INVALID_ARG;
    }

    //lamp_ap_config.ap.authmode = WIFI_AUTH_MAX;
    if (ESP_OK != (_err = init_wifi(WIFI_MODE_AP, network_event_group)) ||
        ESP_OK != (_err = esp_wifi_set_config(ESP_IF_WIFI_AP, &lamp_ap_config)) ||
        ESP_OK != (_err = esp_wifi_start()))
    {
        return _err;
    }

    is_initialized = 1;

    return ESP_OK;
}

esp_err_t init_wifi_sta(EventGroupHandle_t network_event_group, const uint8_t *sta_ssid_str, size_t sta_ssid_size, const uint8_t *sta_pwd_str, size_t sta_pwd_size)
{

    if (is_initialized && WIFI_MODE_AP == current_mode)
    {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t lamp_sta_config = {
        .sta = {
            .ssid = {0},
            .password = {0}
        }
    };

    static esp_err_t _err;

    if (ESP_OK != parse_sta_credentials(&lamp_sta_config, sta_ssid_str, sta_ssid_size, sta_pwd_str, sta_pwd_size))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (ESP_OK != (_err = init_wifi(WIFI_MODE_STA, network_event_group)) ||
        ESP_OK != (_err = esp_wifi_set_config(ESP_IF_WIFI_STA, &lamp_sta_config)) ||
        ESP_OK != (_err = esp_wifi_start()))
    {
        return _err;
    }

    is_initialized = 1;

    return ESP_OK;
}

esp_err_t deinit_wifi(void)
{

    if (!is_initialized)
    {
        return ESP_ERR_INVALID_ARG;
    }

    static esp_err_t _err;
    if (ESP_OK != esp_wifi_deauth_sta(0) ||
        ESP_OK != (_err = esp_wifi_stop()) ||
        ESP_OK != (_err = esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &wifi_event_handler)) ||
        ESP_OK != (_err = esp_wifi_deinit()))
    {
        return _err;
    }

    is_initialized = 0;
    return ESP_OK;
}
