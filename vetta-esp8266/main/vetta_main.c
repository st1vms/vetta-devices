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
#include "wifi_provision.h"

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

#define WIFI_STA_CONNECTED_BIT  BIT0
#define WIFI_STA_DISCONNECTED_BIT BIT1
#define WIFI_AP_STA_CONNECTED_BIT BIT2
#define WIFI_AP_STA_DISCONNECTED_BIT BIT3

/* FreeRTOS event group to signal network events*/
static EventGroupHandle_t network_event_group;

static const UBaseType_t LED_SENSOR_TASK_PRIORITY = 2;
static const UBaseType_t CAPACITIVE_SENSOR_TASK_PRIORITY = 1;
static const UBaseType_t BUTTON_TASK_PRIORITY = 1;
static const UBaseType_t NETWORK_TASK_PRIORITY = 2;

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

// Network wait function prototype, for code clarity
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

static wifi_event_ap_staconnected_t* event_ap_staconnected;
static wifi_event_ap_stadisconnected_t* event_ap_stadisconnected;
static ip_event_got_ip_t* event_sta_gotip;
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {

        // Station connected to lamp AP
        event_ap_staconnected = (wifi_event_ap_staconnected_t*) event_data;

        if(network_event_group){
            xEventGroupSetBits(network_event_group, WIFI_AP_STA_CONNECTED_BIT);
        }
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {

        // Station disconnected from lamp AP
        event_ap_stadisconnected = (wifi_event_ap_stadisconnected_t*) event_data;

        if(network_event_group){
            xEventGroupSetBits(network_event_group, WIFI_AP_STA_DISCONNECTED_BIT);
        }
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
        event_sta_gotip = (ip_event_got_ip_t*) event_data;

        if(network_event_group){
            xEventGroupSetBits(network_event_group, WIFI_STA_CONNECTED_BIT);
        }
    }
}

static spiffs_string_t upwd;
static spiffs_string_t ussid;
static void network_task(void *params)
{
    static uint32_t networkNotificationValue;
    static EventBits_t event_bits;

    static unsigned char is_provisioning = 0;
    static unsigned char sta_connected = 0;
    static esp_err_t _err;

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

                if(ESP_OK == init_wifi_ap(&wifi_event_handler)){

                    // Lamp AP has been requested to start
                    while(pdTRUE != xSemaphoreTake(networkStopSem, (TickType_t) 20 ))
                    {
                        if(is_provisioning){
                            // Wifi provisioning semi-blocking listen
                            printf("\nprovision_listen...\n");
                            if(ESP_OK == (_err = provision_listen(&ussid, &upwd))){
                                // Retrieved SSID and password, ending provisioning
                                deinit_wifi_provision();
                                is_provisioning = 0;
                                xSemaphoreGive(networkStopSem);

                                if( ESP_OK == save_user_ap_ssid(ussid.string_array, ussid.string_len) && ESP_OK == save_user_ap_password(upwd.string_array, upwd.string_len))
                                {
                                    xTaskNotify(networkTask, (1UL << NETWORK_STA_START_WAIT), eSetBits);
                                }

                            }else if(_err == ESP_FAIL){
                                deinit_wifi_provision();
                                is_provisioning = 0;
                                xSemaphoreGive(networkStopSem);
                            }
                            continue;
                        }

                        // Wait for station events
                        event_bits = xEventGroupWaitBits(network_event_group,
                        WIFI_AP_STA_CONNECTED_BIT | WIFI_AP_STA_DISCONNECTED_BIT,
                        pdFALSE,
                        pdFALSE,
                        (TickType_t) 10);


                        // Process AP event
                        if (event_bits & WIFI_AP_STA_CONNECTED_BIT) {
                            xEventGroupClearBits(network_event_group, WIFI_AP_STA_CONNECTED_BIT);

                            if(event_ap_staconnected){
                                printf("\nWIFI_AP_STA_CONNECTED_BIT "MACSTR" join, AID=%d\n",MAC2STR(event_ap_staconnected->mac), event_ap_staconnected->aid);
                                // Initialize Wifi provisioning
                                if(ESP_OK == init_wifi_provision()){
                                    is_provisioning = 1;
                                    continue;
                                }
                            }
                        }

                        if(event_bits & WIFI_AP_STA_DISCONNECTED_BIT){
                            xEventGroupClearBits(network_event_group, WIFI_AP_STA_DISCONNECTED_BIT);

                            if(event_ap_stadisconnected){
                                printf("\nStation "MACSTR" leave, AID=%d\n",MAC2STR(event_ap_stadisconnected->mac), event_ap_stadisconnected->aid);
                                // Deinit Wifi provisioning
                                deinit_wifi_provision();
                                is_provisioning = 0;
                            }
                        }
                    }

                    deinit_wifi(&wifi_event_handler);
                }

                stop_led_animation(ledStopSem, ledAnimationSem);

            }else if (networkNotificationValue & NETWORK_STA_START_WAIT)
            {
                if((NULL != ussid.string_array && ussid.string_len > 0))
                {

                    if(ESP_OK == init_wifi_sta(&wifi_event_handler, ussid.string_array, ussid.string_len,
                        (upwd.string_array != NULL && upwd.string_len > 0) ? upwd.string_array : NULL,
                        upwd.string_array != NULL ? upwd.string_len : 0))
                    {
                        // Lamp AP has been requested to start
                        while(pdTRUE != xSemaphoreTake(networkStopSem, (TickType_t) 20 ))
                        {
                            if(sta_connected){
                                printf("\nsta_connected...\n");
                                continue;
                            }

                            // Wait for AP events
                            event_bits = xEventGroupWaitBits(network_event_group,
                            WIFI_STA_CONNECTED_BIT | WIFI_STA_DISCONNECTED_BIT,
                            pdFALSE,
                            pdFALSE,
                            (TickType_t) 10);

                            // Process AP event
                            if (event_bits & WIFI_STA_CONNECTED_BIT) {
                                xEventGroupClearBits(network_event_group, WIFI_STA_CONNECTED_BIT);

                                if(event_sta_gotip){
                                    printf("\nnIP_EVENT_STA_GOT_IP -> ip is %s\n", ip4addr_ntoa(&(event_sta_gotip->ip_info.ip)));
                                    sta_connected = 1;
                                    continue;
                                }
                            }else if (event_bits & WIFI_STA_DISCONNECTED_BIT){
                                xEventGroupClearBits(network_event_group, WIFI_STA_DISCONNECTED_BIT);
                            }
                        }
                        deinit_wifi(&wifi_event_handler);
                    }
                }
            }
        }

        xSemaphoreTake(networkStopSem, (TickType_t)10);
        // Critical section end
        network_task_working = 0;
        if (networkSem){
            // Release sem, allowing button_task to stop waiting
            xSemaphoreGive(networkSem);
        }
    }
}


static void waitNetworkStop(void)
{
    if (!networkSem || !networkStopSem || !network_task_working){
        return;
    }

    // Signal network task to stop and wait indefinitely until signaled
    if (pdTRUE == xSemaphoreGive(networkStopSem) &&
        pdTRUE == xSemaphoreTake(networkSem, portMAX_DELAY))
    {
        // Network task signaled us, reacquire stop semaphore and return
        xSemaphoreTake(networkStopSem, (TickType_t)10);
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

        /* ussid = get_user_ap_ssid_string();
        upwd =  get_user_ap_password_string();

        // If STA SSID is set, try automatic reconnection
        if(NULL != ussid){
           xTaskNotify(networkTask, (1UL << NETWORK_STA_START_EVENT), eSetBits);
        } */
    }
}
