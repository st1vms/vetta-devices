#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h"
#include "led.h"
#include "touch.h"

#define configTASK_NOTIFICATION_ARRAY_ENTRIES       (1)

#define CAPACITIVE_SENSOR_TASK_PRIORITY             (1)
#define SMARTCONFIG_TASK_PRIORITY                   (3)
#define NETWORK_TASK_PRIORITY                       (4)

#define LED_UPDATER_TASK_STACK_DEPTH                (2048)

#define CAPACITIVE_SENSOR_TASK_STACK_DEPTH          (2048)

#define CAPACITIVE_SENSOR_LED_NOTIFY_DELAY          (500)
#define CAPACITIVE_SENSOR_LED_NOTIFICATION_VALUE    (0x1)

#define ESP_SMARTCONFIG_TYPE SC_TYPE_ESPTOUCH_V2

static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;

static unsigned char wifi_connected = 0;

static TaskHandle_t ledUpdaterTask = NULL;
static TaskHandle_t capSensorTask = NULL;

static void led_updater_task(void * params)
{
    static uint32_t ledNotificationValue;

    for(;;)
    {
        if(xTaskNotifyWait(0x00,      /* Don't clear any notification bits on entry. */
                           0xffffffffUL, /* Reset the notification value to 0 on exit. */
                           &ledNotificationValue, /* Notified value */
                           portMAX_DELAY ) == pdTRUE)
        {
            if(ledNotificationValue & CAPACITIVE_SENSOR_LED_NOTIFICATION_VALUE)
            {
                led_set_next();
            }
        }
    }
}


static void capacitive_sensor_task(void * params)
{
    static uint16_t idle_read;
    static uint16_t samples[READING_SAMPLES_POOL_SIZE] = {0};
    size_t ix = 0;

    unsigned char calibrated = 0;
    unsigned char can_update = 0;

    time_t time_offset = 0;
    long int total = 0;

    idle_read = calibrate_idle();

    for(;;)
    {
        total -= samples[ix];
        if((samples[ix] = read_sensor_analog()) > 0)
        {
            total += samples[ix];
            ix++;

            if(ix == READING_SAMPLES_POOL_SIZE)
            {
                ix = 0;
                calibrated = 1;
            }
        }

        if(calibrated && can_update && is_touch(total / READING_SAMPLES_POOL_SIZE, idle_read))
        {
            can_update = 0;
            time_offset = 0;
            // send led update event
            xTaskNotifyGive(ledUpdaterTask);
        }

        TIME_DELAY_MILLIS(READING_DELAY_MILLIS);

        if(time_offset >= CAPACITIVE_SENSOR_LED_NOTIFY_DELAY)
        {
            can_update = 1;
            continue;
        }
        time_offset += READING_DELAY_MILLIS;
    }
}

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;
static void smart_config_task(void * params);

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        xTaskCreate(smart_config_task, "smart_config_task", 4096, NULL, SMARTCONFIG_TASK_PRIORITY, NULL);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = 0;
        ESP_LOGI("tsk", "\nWIFI_EVENT_STA_DISCONNECTED\n");
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI("tsk", "\nIP_EVENT_STA_GOT_IP\n");
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        // Scan done
        ESP_LOGI("tsk", "\nSC_EVENT_SCAN_DONE\n");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        // Found channel
        ESP_LOGI("tsk", "\nSC_EVENT_FOUND_CHANNEL\n");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        // Got SSID and password
        smartconfig_event_got_ssid_pswd_t* evt = (smartconfig_event_got_ssid_pswd_t*)event_data;
        wifi_config_t wifi_config;
        uint8_t rvd_data[33] = {0};

        if (evt->type == ESP_SMARTCONFIG_TYPE && ESP_OK == esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data))) {
            // Received esptouch reserved data
            ESP_LOGI("tsk", "\nreserved data: %s\n", rvd_data);
        }

        esp_wifi_disconnect();
        esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
        esp_wifi_connect();
    }else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        ESP_LOGI("tsk", "\nSC_EVENT_SEND_ACK_DONE\n");
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

static void smart_config_task(void * params){
    EventBits_t uxBits;
    if(ESP_OK == esp_smartconfig_set_type(ESP_SMARTCONFIG_TYPE))
    {
        smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
        if (ESP_OK == esp_smartconfig_start(&cfg))
        {

            for (;;) {
                uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);

                if (uxBits & CONNECTED_BIT) {
                    // WiFi Connected to AP
                    wifi_connected = 1;
                    ESP_LOGI("tsk", "\nWiFi Connected to AP\n");
                }

                if (uxBits & ESPTOUCH_DONE_BIT) {
                    // smartconfig over
                    ESP_LOGI("tsk", "\nsmartconfig over\n");
                    esp_smartconfig_stop();
                    vTaskDelete(NULL);
                }
            }
        }
    }
}

static esp_err_t initialise_wifi(void)
{
    static esp_err_t _err;

    tcpip_adapter_init();

    s_wifi_event_group = xEventGroupCreate();

    if(ESP_OK != (_err = esp_event_loop_create_default()))
    {
        return _err;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    if(ESP_OK != (_err = esp_wifi_init(&cfg)))
    {
        return _err;
    }

    esp_wifi_set_ps(WIFI_PS_NONE);

    if( ESP_OK != (_err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL)) ||
        ESP_OK != (_err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL)) ||
        ESP_OK != (_err = esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL)) ||
        ESP_OK != (_err = esp_wifi_set_mode(WIFI_MODE_STA)) ||
        ESP_OK != (_err = esp_wifi_start()))
        {
            return _err;
        }
    return ESP_OK;
}

void app_main()
{
    // Initialization
    esp_err_t _err;
    if(ESP_OK != (_err = init_led_module()) || ESP_OK != (_err = led_low()))
    {
        printf("\ninit_led_module() error {%d}\n", _err);
    }
    else if(ESP_OK != (_err = init_touch_sensor_module()))
    {
        printf("\ninit_touch_sensor_module() error {%d}\n", _err);
    }
    else{

        // Create capacitive sensor task
        xTaskCreate(capacitive_sensor_task,
                    "sensor_task",
                    CAPACITIVE_SENSOR_TASK_STACK_DEPTH,
                    NULL,
                    CAPACITIVE_SENSOR_TASK_PRIORITY,
                    &capSensorTask);

        // Create led updater task
        xTaskCreate(led_updater_task,
                    "led_task",
                    LED_UPDATER_TASK_STACK_DEPTH,
                    NULL,
                    tskIDLE_PRIORITY,
                    &ledUpdaterTask);

        if(ESP_OK != (_err = nvs_flash_init()))
        {
            printf("\nnvs_flash_init() error {%d}\n", _err);
        }
        else if(ESP_OK != (_err = initialise_wifi()))
        {
            printf("\ninitialise_wifi() error {%d}\n", _err);
        }
    }

}
