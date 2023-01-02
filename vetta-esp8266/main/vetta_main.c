#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "led.h"
#include "touch.h"
#include "esp_timer.h"
#include "push_button.h"
#include "wifi_manager.h"
#include "storage.h"
#include "espconfig_overwrite.h"

static const UBaseType_t LED_SENSOR_TASK_PRIORITY = tskIDLE_PRIORITY;
static const UBaseType_t CAPACITIVE_SENSOR_TASK_PRIORITY = tskIDLE_PRIORITY;
static const UBaseType_t BUTTON_TASK_PRIORITY = 1;
static const UBaseType_t NETWORK_TASK_PRIORITY = 2;

static TaskHandle_t ledUpdaterTask = NULL;
static TaskHandle_t capSensorTask = NULL;
static TaskHandle_t buttonTask = NULL;
static TaskHandle_t networkTask = NULL;

static inline void TIME_DELAY_MILLIS(long int x) {vTaskDelay(x / portTICK_PERIOD_MS);};

static led_animation_t current_led_animation;

static void led_updater_task(void *params)
{
    static uint32_t ledNotificationValue;

    for (;;)
    {
        if (xTaskNotifyWait(0x00,                  /* Don't clear any notification bits on entry. */
                            0xffffffffUL,          /* Reset the notification value to 0 on exit. */
                            &ledNotificationValue, /* Notified value */
                            portMAX_DELAY) == pdTRUE)
        {
            if (ledNotificationValue & CAPACITIVE_SENSOR_LED_NOTIFICATION_WAIT_VALUE)
            {
                // Led update from sensor
                led_set_next();
            }
            else if (ledNotificationValue & LED_ANIMATION_START_NOTIFICATION_WAIT_VALUE)
            {
                printf("\nStart Animation..\n");
                play_led_animation(&current_led_animation);
                printf("\nEnd Animation..\n");
            }
        }
    }
}

static void capacitive_sensor_task(void *params)
{
    static uint16_t idle_read;
    idle_read = 0;

    static uint16_t samples[READING_SAMPLES_POOL_SIZE] = {0};
    static size_t ix;
    ix = 0;

    static uint16_t total;
    total = 0;

    static TickType_t time_offset;
    time_offset = 0;

    static unsigned char can_update_led;
    can_update_led = 0;

    for (;;)
    {
        total -= samples[ix];
        if ((samples[ix] = read_sensor_analog()) > 0)
        {
            total += samples[ix++];

            // Logs capacitive sensor readings
            /*
            if(idle_read != 0){
                printf("\nt:%u , idle: %u\n", total / READING_SAMPLES_POOL_SIZE, idle_read );
            }
            */
            if(ix == READING_SAMPLES_POOL_SIZE){
                if(idle_read == 0){
                    idle_read = total / READING_SAMPLES_POOL_SIZE;
                }
                ix = 0;
            }

            if (samples[READING_SAMPLES_POOL_SIZE] != 0 &&
                idle_read != 0 &&
                can_update_led &&
                is_touch(total / READING_SAMPLES_POOL_SIZE, idle_read))
            {
                can_update_led = 0;
                time_offset = 0;
                // send led update event from sensor task
                xTaskNotify(ledUpdaterTask, (1UL << CAPACITIVE_SENSOR_LED_NOTIFICATION_VALUE), eSetBits);
            }
        }

        TIME_DELAY_MILLIS(READING_DELAY_MILLIS);

        if (time_offset >= CAPACITIVE_SENSOR_LED_UPDATE_DELAY)
        {
            can_update_led = 1;
            continue;
        }
        time_offset += READING_DELAY_MILLIS;
    }
}

static void button_isr_handler(void *args)
{
    static BaseType_t xHigherPriorityTaskWoken;

    xHigherPriorityTaskWoken = pdFALSE;

    // Notify button task
    xTaskNotifyFromISR(buttonTask, (1UL << BUTTON_INTR_EVENT_VALUE), eSetBits, &xHigherPriorityTaskWoken);

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static unsigned char network_task_done = 0;
static unsigned char network_stop_flag = 0;
static void stopNetworkTaskWaiting(void){

    static unsigned long int timeout_millis;
    timeout_millis = 0;

    network_stop_flag = 1;
    while(!network_task_done || timeout_millis >= 10000) // 10 seconds timeout
    {
        TIME_DELAY_MILLIS(10);
        timeout_millis += xTaskGetTickCount() * portTICK_PERIOD_MS;
    }
    network_stop_flag = 0;
}


/* static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // Station started
        ESP_LOGI("wifi", "\nWIFI_EVENT_STA_START\n");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI("wifi", "\nWIFI_EVENT_STA_DISCONNECTED\n");
        //esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI("wifi", "\nIP_EVENT_STA_GOT_IP\n");
    }
} */

static void reset_callback(void){

    if(!current_led_animation.is_playing){
        if(ESP_OK == SET_BLINK_OFF_HIGH_ANIMATION(&current_led_animation, 1)){
            xTaskNotify(ledUpdaterTask, (1UL << LED_ANIMATION_START_NOTIFICATION_VALUE), eSetBits);
        }
    }
}

static void button_task(void *params)
{
    static uint32_t buttonNotificationValue;

    for (;;)
    {
        if (xTaskNotifyWait(0x00,                      /* Don't clear any notification bits on entry. */
                            0xffffffffUL,              /* Reset the notification value to 0 on exit. */
                            &buttonNotificationValue,  /* Notified value */
                            portMAX_DELAY) == pdTRUE)
        {
            if (buttonNotificationValue & BUTTON_INTR_EVENT_WAIT_VALUE)
            {
                switch (get_press_event())
                {
                    case PRESS_EVENT_RESET:
                        reset_callback();
                        break;
                    case PRESS_EVENT_DISCOVERY:
                        printf("\nDiscovery event\n");
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

static void network_task(void *params){

    static uint32_t networkNotificationValue;

    for(;;){
        if (xTaskNotifyWait(0x00,                      /* Don't clear any notification bits on entry. */
                            0xffffffffUL,              /* Reset the notification value to 0 on exit. */
                            &networkNotificationValue,  /* Notified value */
                            portMAX_DELAY) == pdTRUE)
        {
            if (networkNotificationValue & NETWORK_AP_START_WAIT_VALUE)
            {
                // Received AP start notification
                network_task_done = 0;

                // Critical section start
                while(!network_stop_flag){
                    TIME_DELAY_MILLIS(10);
                }
                // Critical section end

                network_task_done = 1;
            }
        }
    }
}


void app_main()
{
    static esp_err_t _err;

    // Initialize led module and capacitive sensor module, set the led to low
    if (ESP_OK != (_err = init_led_module()) || ESP_OK != (_err = led_low()))
    {
        printf("\ninit_led_module() error {%d}\n", _err);
    }
    else if (ESP_OK != (_err = init_touch_sensor_module()))
    {
        printf("\ninit_touch_sensor_module() error {%d}\n", _err);
    }
    else
    {
        // Create led updater task
        xTaskCreate(led_updater_task,
                    "led_task",
                    LED_UPDATER_TASK_STACK_DEPTH,
                    NULL,
                    LED_SENSOR_TASK_PRIORITY,
                    &ledUpdaterTask);

        // Create capacitive sensor task
        xTaskCreate(capacitive_sensor_task,
                    "sensor_task",
                    CAPACITIVE_SENSOR_TASK_STACK_DEPTH,
                    NULL,
                    CAPACITIVE_SENSOR_TASK_PRIORITY,
                    &capSensorTask);

        // Create button task
        xTaskCreate(button_task,
                    "button_task",
                    BUTTON_TASK_STACK_DEPTH,
                    NULL,
                    BUTTON_TASK_PRIORITY,
                    &buttonTask);

        // Create network task
        xTaskCreate(network_task,
                    "network_task",
                    NETWORK_TASK_STACK_DEPTH,
                    NULL,
                    NETWORK_TASK_PRIORITY,
                    &networkTask);

        // Tasks created
        // Initialize button ISR, NVS and Wifi
        if (ESP_OK != (_err = init_button(&button_isr_handler)))
        {
            printf("\ninitialise_wifi() error {%d}\n", _err);
        }
        else if (ESP_OK != (_err = nvs_flash_init()))
        {
            printf("\nnvs_flash_init() error {%d}\n", _err);
        }

        const spiffs_string_t * u = get_lamp_ap_password_string();
        if(u != NULL)
        {
            printf("\n %s - %u\n", u->string_array, u->string_len);
        }

    }
}
