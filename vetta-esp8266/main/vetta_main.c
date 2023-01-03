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
static const UBaseType_t BUTTON_TASK_PRIORITY = tskIDLE_PRIORITY;
static const UBaseType_t NETWORK_TASK_PRIORITY = 1;

static TaskHandle_t ledUpdaterTask = NULL;
static TaskHandle_t capSensorTask = NULL;
static TaskHandle_t buttonTask = NULL;
static TaskHandle_t networkTask = NULL;

static SemaphoreHandle_t networkSem = NULL;
static SemaphoreHandle_t networkStopSem = NULL;

static unsigned char network_task_working;

static inline void TIME_DELAY_MILLIS(long int x) { vTaskDelay(x / portTICK_PERIOD_MS); };

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
            else if (ledNotificationValue & LED_BLINK_ANIMATION_START_NOTIFICATION_WAIT_VALUE)
            {
                if (ESP_OK == SET_BLINK_OFF_HIGH_ANIMATION(1))
                {

                    printf("\nStart Animation..\n");
                    play_led_animation();
                    printf("\nEnd Animation..\n");
                }
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
            if (ix == READING_SAMPLES_POOL_SIZE)
            {
                if (idle_read == 0)
                {
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

static void waitNetworkStop(void)
{
    if(!networkSem || !networkStopSem || !network_task_working){return;}

    if(pdTRUE == xSemaphoreGive(networkStopSem) &&
        pdTRUE == xSemaphoreTake(networkSem, portMAX_DELAY))
    {
        xSemaphoreTake(networkStopSem, (TickType_t) 10);
    }
}

static void button_task(void *params)
{

    static uint32_t buttonNotificationValue;

    for (;;)
    {
        if (xTaskNotifyWait(0x00,                     /* Don't clear any notification bits on entry. */
                            0xffffffffUL,             /* Reset the notification value to 0 on exit. */
                            &buttonNotificationValue, /* Notified value */
                            portMAX_DELAY) == pdTRUE)
        {
            if (buttonNotificationValue & BUTTON_INTR_EVENT_WAIT_VALUE)
            {
                switch (get_press_event())
                {
                case PRESS_EVENT_RESET:

                    // Send blink animation event
                    xTaskNotify(ledUpdaterTask, (1UL << LED_BLINK_ANIMATION_START_NOTIFICATION_VALUE), eSetBits);

                    waitNetworkStop();

                    // Reset persistent storage
                    spiffs_data_reset();

                    xTaskNotify(networkTask, (1UL << NETWORK_AP_START_VALUE), eSetBits);

                    stop_led_animation();
                    break;
                case PRESS_EVENT_DISCOVERY:

                    waitNetworkStop();

                    break;
                default:
                    break;
                }
            }
        }
    }
}

static void network_task(void *params)
{

    static uint32_t networkNotificationValue;

    unsigned long int tick = 0;

    for (;;)
    {
        if (xTaskNotifyWait(0x00,                      /* Don't clear any notification bits on entry. */
                            0xffffffffUL,              /* Reset the notification value to 0 on exit. */
                            &networkNotificationValue, /* Notified value */
                            portMAX_DELAY) == pdTRUE)
        {
            if (networkNotificationValue & NETWORK_AP_START_WAIT_VALUE)
            {
                tick = 0;

                // Received AP start notification
                printf("\nNetwork task start...\n");

                network_task_working = 1;
                // Critical section start
                while(pdTRUE != xSemaphoreTake(networkStopSem, (TickType_t) 10))
                {
                    printf("\n1 second delay... %lu\n", tick);
                    TIME_DELAY_MILLIS(1000);
                    tick += 1;
                }
                network_task_working = 0;
                // Critical section end
                if(networkSem){
                    xSemaphoreGive(networkSem);
                }

                printf("\nNetwork task end...\n");
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

        network_task_working = 0;

        if(!networkSem){
            networkSem = xSemaphoreCreateBinary();
            xSemaphoreTake(networkSem, (TickType_t) 10);
        }

        if(!networkStopSem){
            networkStopSem = xSemaphoreCreateBinary();
            xSemaphoreTake(networkStopSem, (TickType_t) 10);
        }

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
    }
}
