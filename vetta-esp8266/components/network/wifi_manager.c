#include "wifi_manager.h"

static esp_err_t register_wifi_sta_event_handler(esp_event_handler_t handler_func){
    static esp_err_t _err;

    if (ESP_OK != (_err = esp_event_loop_create_default()) ||
        ESP_OK != (_err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, handler_func, NULL)) ||
        ESP_OK != (_err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, handler_func, NULL)))
    {
        return _err;
    }
    return ESP_OK;
}

esp_err_t init_wifi(wifi_mode_t wifi_mode, esp_event_handler_t wifi_event_handler)
{
    static esp_err_t _err;
    static wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    tcpip_adapter_init();

    if (ESP_OK != (_err = esp_wifi_init(&cfg)) ||
        ESP_OK != (_err = register_wifi_sta_event_handler(wifi_event_handler)) ||
        ESP_OK != (_err = esp_wifi_set_mode(wifi_mode)))
    {
        return _err;
    }

    esp_wifi_set_ps(WIFI_PS_NONE);
    return esp_wifi_start();
}
