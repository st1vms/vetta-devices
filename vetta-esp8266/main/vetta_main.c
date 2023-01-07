#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "espconfig_overwrite.h"
#include "led.h"
#include "push_button.h"
#include "storage.h"
#include "touch.h"
#include "wifi_manager.h"

#define CAPACITIVE_SENSOR_LED_EVENT_WAIT (0x01)
#define CAPACITIVE_SENSOR_LED_EVENT (0UL)

#define LED_BLINK_START_EVENT_WAIT (0x02)
#define LED_BLINK_START_EVENT (1UL)

#define LED_BLINK_LOOP_START_EVENT_WAIT (0x04)
#define LED_BLINK_LOOP_START_EVENT (2UL)

#define BUTTON_INTR_EVENT_WAIT (0x01)
#define BUTTON_INTR_EVENT (0UL)

#define NETWORK_AP_START_WAIT (0x01)
#define NETWORK_AP_START_EVENT (0UL)

#define NETWORK_STA_START_WAIT (0x02)
#define NETWORK_STA_START_EVENT (1UL)

/* FreeRTOS event group to signal network events*/
static EventGroupHandle_t network_event_group;

static const UBaseType_t LED_SENSOR_TASK_PRIORITY = tskIDLE_PRIORITY;
static const UBaseType_t CAPACITIVE_SENSOR_TASK_PRIORITY = tskIDLE_PRIORITY;
static const UBaseType_t BUTTON_TASK_PRIORITY = tskIDLE_PRIORITY;
static const UBaseType_t NETWORK_TASK_PRIORITY = 1;

static TaskHandle_t ledUpdaterTask = NULL;
static TaskHandle_t capSensorTask = NULL;
static TaskHandle_t buttonTask = NULL;
static TaskHandle_t networkTask = NULL;

static SemaphoreHandle_t ledStopSem = NULL;
static SemaphoreHandle_t ledAnimationSem = NULL;
static SemaphoreHandle_t networkSem = NULL;
static SemaphoreHandle_t networkStopSem = NULL;

static unsigned char network_task_working = 0;

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
            if (ledNotificationValue & CAPACITIVE_SENSOR_LED_EVENT_WAIT)
            {
                // Led update from sensor
                led_set_next();
            }
            else if (ledNotificationValue & LED_BLINK_START_EVENT_WAIT)
            {
                if (ESP_OK == SET_BLINK_OFF_HIGH_ANIMATION(1))
                {
                    play_led_animation(ledStopSem, ledAnimationSem);
                }
            }
            else if (ledNotificationValue & LED_BLINK_LOOP_START_EVENT_WAIT)
            {
                if (ESP_OK == SET_BLINK_OFF_HIGH_ANIMATION(-1))
                {
                    play_led_animation(ledStopSem, ledAnimationSem);
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

            if (samples[READING_SAMPLES_POOL_SIZE-1] != 0 &&
                idle_read != 0 &&
                can_update_led &&
                is_touch(total / READING_SAMPLES_POOL_SIZE, idle_read))
            {
                can_update_led = 0;
                time_offset = 0;
                // send led update event from sensor task
                xTaskNotify(ledUpdaterTask, (1UL << CAPACITIVE_SENSOR_LED_EVENT), eSetBits);
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
    xTaskNotifyFromISR(buttonTask, (1UL << BUTTON_INTR_EVENT), eSetBits, &xHigherPriorityTaskWoken);

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

// Network function prototypes, for code clarity
static void waitNetworkStop(void);

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
            if (buttonNotificationValue & BUTTON_INTR_EVENT_WAIT)
            {
                switch (get_press_event())
                {
                case PRESS_EVENT_RESET:

                    // Wait for network task to stop
                    waitNetworkStop();

                    // Reset persistent storage
                    spiffs_data_reset();

                    // Send blink animation event
                    xTaskNotify(ledUpdaterTask, (1UL << LED_BLINK_START_EVENT), eSetBits);

                    TIME_DELAY_MILLIS(1000);

                    stop_led_animation(ledStopSem, ledAnimationSem);
                    break;
                case PRESS_EVENT_DISCOVERY:

                    // Wait for network task to stop
                    waitNetworkStop();

                    xTaskNotify(networkTask, (1UL << NETWORK_AP_START_EVENT), eSetBits);
                    break;

                default:
                    break;
                }
            }
        }
    }
}


static const spiffs_string_t * upwd;
static const spiffs_string_t * ussid;
static void network_task(void *params)
{
    static uint32_t networkNotificationValue;
    static EventBits_t event_bits;

    const spiffs_string_t * apwd = get_lamp_ap_password_string();
    for (;;)
    {
        if (xTaskNotifyWait(0x00,                      /* Don't clear any notification bits on entry. */
                            0xffffffffUL,              /* Reset the notification value to 0 on exit. */
                            &networkNotificationValue, /* Notified value */
                            portMAX_DELAY) == pdTRUE)
        {
            // Critical section start
            network_task_working = 1;

            // Process network task event
            if (networkNotificationValue & NETWORK_AP_START_WAIT){
                // Send blink animation event
                xTaskNotify(ledUpdaterTask, (1UL << LED_BLINK_LOOP_START_EVENT), eSetBits);

                if(ESP_OK == init_wifi_ap(network_event_group, !apwd ? NULL : apwd->string_array, !apwd ? 0 : apwd->string_len)){

                    // Lamp AP has been requested to start
                    while(pdTRUE != xSemaphoreTake(networkStopSem, (TickType_t) 10 ))
                    {
                        // Wait for station events
                        event_bits = xEventGroupWaitBits(network_event_group,
                        WIFI_AP_STA_CONNECTED_BIT,
                        pdFALSE,
                        pdFALSE,
                        (TickType_t) 20);

                        // Process AP event
                        if (event_bits & WIFI_AP_STA_CONNECTED_BIT) {
                            printf("\nWIFI_AP_STA_CONNECTED_BIT\n");
                            xEventGroupClearBits(network_event_group, WIFI_AP_STA_CONNECTED_BIT);
                        }
                    }
                    deinit_wifi();
                }

                stop_led_animation(ledStopSem, ledAnimationSem);

            }else if (networkNotificationValue & NETWORK_STA_START_WAIT)
            {
                if(NULL != ussid && ussid->string_len > 0){
                    // AP credentials are set

                    if(ESP_OK == init_wifi_sta(network_event_group, ussid->string_array, ussid->string_len,
                        (upwd != NULL && upwd->string_len > 0) ? upwd->string_array : NULL,
                        upwd != NULL ? upwd->string_len : 0))
                    {
                        // Lamp AP has been requested to start
                        while(pdTRUE != xSemaphoreTake(networkStopSem, (TickType_t) 10 ))
                        {
                            // Wait for AP events
                            event_bits = xEventGroupWaitBits(network_event_group,
                            WIFI_STA_CONNECTED_BIT,
                            pdFALSE,
                            pdFALSE,
                            (TickType_t) 20);

                            // Process AP event
                            if (event_bits & WIFI_STA_CONNECTED_BIT) {
                                printf("\nWIFI_STA_CONNECTED_BIT\n");
                                xEventGroupClearBits(network_event_group, WIFI_STA_CONNECTED_BIT);
                            }
                        }

                        deinit_wifi();
                    }
                }
            }

            // Critical section end
            network_task_working = 0;
            if (networkSem)
            {
                // Release sem, allowing button_task to stop waiting
                xSemaphoreGive(networkSem);
            }
        }
    }
}


static void waitNetworkStop(void)
{
    if (!networkSem || !networkStopSem || !network_task_working)
    {
        return;
    }

    printf("\nSIGNAL NETWORK STOP 1..\n");
    // Signal network task to stop and wait indefinitely until signaled
    if (pdTRUE == xSemaphoreGive(networkStopSem) &&
        pdTRUE == xSemaphoreTake(networkSem, portMAX_DELAY))
    {
        // Network task signaled us, reacquire stop semaphore and return
        xSemaphoreTake(networkStopSem, (TickType_t)10);
        printf("\nSIGNAL NETWORK STOP 2..\n");
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
        if(!network_event_group){
            network_event_group = xEventGroupCreate();
        }

        network_task_working = 0;

        if (!ledStopSem)
        {
            ledStopSem = xSemaphoreCreateBinary();
            xSemaphoreTake(ledStopSem, (TickType_t)10);
        }
        if (!ledAnimationSem)
        {
            ledAnimationSem = xSemaphoreCreateBinary();
            xSemaphoreTake(ledAnimationSem, (TickType_t)10);
        }

        if (!networkSem)
        {
            networkSem = xSemaphoreCreateBinary();
            xSemaphoreTake(networkSem, (TickType_t)10);
        }

        if (!networkStopSem)
        {
            networkStopSem = xSemaphoreCreateBinary();
            xSemaphoreTake(networkStopSem, (TickType_t)10);
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

        ussid = get_user_ap_ssid_string();
        upwd =  get_user_ap_password_string();

        // If STA SSID is set, try automatic reconnection
        if(NULL != ussid){
           xTaskNotify(networkTask, (1UL << NETWORK_STA_START_EVENT), eSetBits);
        }
    }
}
