#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "espconfig_overwrite.h"
#include "led.h"
#include "push_button.h"
#include "storage.h"
#include "touch.h"
#include "packets.h"
#include "wifi_manager.h"
#include "wifi_provision.h"

// Sensors Events

#define CAPACITIVE_SENSOR_LED_EVENT_WAIT (0x01)
#define CAPACITIVE_SENSOR_LED_EVENT (1UL << 0UL)

#define LED_BLINK_START_EVENT_WAIT (0x02)
#define LED_BLINK_START_EVENT (1UL << 1UL)

#define LED_BLINK_LOOP_START_EVENT_WAIT (0x04)
#define LED_BLINK_LOOP_START_EVENT (1UL << 2UL)

#define BUTTON_INTR_EVENT_WAIT (0x01)
#define BUTTON_INTR_EVENT (1UL << 0UL)

// Network Events
#define WIFI_SHUTDOWN_EVENT_WAIT (0x01)
#define WIFI_SHUTDOWN_EVENT (1UL << 0UL)

#define WIFI_START_AP_EVENT_WAIT (0x02)
#define WIFI_START_AP_EVENT (1UL << 1UL)

#define WIFI_AP_STA_CONNECTED_EVENT_WAIT (0x04)
#define WIFI_AP_STA_CONNECTED_EVENT (1UL << 2UL)

#define WIFI_AP_STA_DISCONNECTED_EVENT_WAIT (0x08)
#define WIFI_AP_STA_DISCONNECTED_EVENT (1UL << 3UL)

#define WIFI_START_STA_EVENT_WAIT (0x10)
#define WIFI_START_STA_EVENT (1UL << 4UL)

#define WIFI_STA_CONNECTED_EVENT_WAIT (0x20)
#define WIFI_STA_CONNECTED_EVENT (1UL << 5UL)

/* FreeRTOS event group to signal network events*/

static const UBaseType_t LED_SENSOR_TASK_PRIORITY = 7;
static const UBaseType_t CAPACITIVE_SENSOR_TASK_PRIORITY = 7;
static const UBaseType_t BUTTON_TASK_PRIORITY = 6;
static const UBaseType_t NETWORK_TASK_PRIORITY = 5;

static TaskHandle_t ledUpdaterTask = NULL;
static TaskHandle_t capSensorTask = NULL;
static TaskHandle_t buttonTask = NULL;
static TaskHandle_t networkTask = NULL;

static SemaphoreHandle_t ledStopSem = NULL;
static SemaphoreHandle_t ledAnimationSem = NULL;

static unsigned char cap_sensor_active = 1;

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
                if (ESP_OK == SET_FAST_BLINK_ANIMATION(1))
                {
                    play_led_animation(ledStopSem, ledAnimationSem);
                    led_off();
                }
            }
            else if (ledNotificationValue & LED_BLINK_LOOP_START_EVENT_WAIT)
            {
                if (ESP_OK == SET_BLINK_OFF_HIGH_ANIMATION(-1))
                {
                    play_led_animation(ledStopSem, ledAnimationSem);
                    led_off();
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
        if (!cap_sensor_active)
        {
            TIME_DELAY_MILLIS(READING_DELAY_MILLIS);
            continue;
        }

        total -= samples[ix];

        if ((samples[ix] = read_sensor_analog()) > 0)
        {
            total += samples[ix++];
            // Logs capacitive sensor readings

            /* if(idle_read != 0){
                printf("\nt:%u , idle: %u\n", total / READING_SAMPLES_POOL_SIZE, idle_read );
            } */

            if (ix == READING_SAMPLES_POOL_SIZE)
            {
                if (idle_read == 0)
                {
                    idle_read = total / READING_SAMPLES_POOL_SIZE;
                }
                ix = 0;
            }

            if (samples[READING_SAMPLES_POOL_SIZE - 1] != 0 &&
                idle_read != 0 &&
                can_update_led &&
                is_touch(total / READING_SAMPLES_POOL_SIZE, idle_read))
            {
                can_update_led = 0;
                time_offset = 0;

                // send led update event from sensor task
                xTaskNotify(ledUpdaterTask, CAPACITIVE_SENSOR_LED_EVENT, eSetBits);
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
    xTaskNotifyFromISR(buttonTask, BUTTON_INTR_EVENT, eSetBits, &xHigherPriorityTaskWoken);

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static unsigned char ap_credentials_available = 0;
static spiffs_string_t upwd;
static spiffs_string_t ussid;
static void reset_persistent_storage(void)
{
    // Reset SPIFFS persistent storage
    spiffs_data_reset();
    ap_credentials_available = 0;

    // Clear references
    if (ussid.string_array != NULL)
    {
        memset(ussid.string_array, 0, sizeof(uint8_t) * MAX_SPIFFS_STRING_LENGTH);
    }
    ussid.string_len = 0;

    if (upwd.string_array != NULL)
    {
        memset(upwd.string_array, 0, sizeof(uint8_t) * MAX_SPIFFS_STRING_LENGTH);
    }
    upwd.string_len = 0;
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
            if (buttonNotificationValue & BUTTON_INTR_EVENT_WAIT)
            {
                switch (get_press_event())
                {
                case PRESS_EVENT_RESET:

                    xTaskNotify(ledUpdaterTask, LED_BLINK_START_EVENT, eSetBits);

                    reset_persistent_storage();

                    // Delay one second before shutting down wifi and led animation
                    TIME_DELAY_MILLIS(1000);

                    xTaskNotify(networkTask, WIFI_SHUTDOWN_EVENT, eSetBits);
                    break;
                case PRESS_EVENT_DISCOVERY:

                    reset_persistent_storage();

                    xTaskNotify(networkTask, WIFI_START_AP_EVENT, eSetBits);
                    break;
                default:
                    break;
                }
            }
        }
    }
}

static unsigned char network_task_working = 0;
static unsigned char is_provisioning = 0;
static unsigned char sta_connected = 0;
static wifi_event_ap_staconnected_t *event_ap_staconnected;
static wifi_event_ap_stadisconnected_t *event_ap_stadisconnected;
static ip_event_got_ip_t *event_sta_gotip;
static int s_retry_num = 0;
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {

        // Station connected to lamp AP
        event_ap_staconnected = (wifi_event_ap_staconnected_t *)event_data;

        xTaskNotify(networkTask, WIFI_AP_STA_CONNECTED_EVENT, eSetBits);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {

        // Station disconnected from lamp AP
        event_ap_stadisconnected = (wifi_event_ap_stadisconnected_t *)event_data;

        xTaskNotify(networkTask, WIFI_AP_STA_DISCONNECTED_EVENT, eSetBits);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // Lamp starting station connection to home AP
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // Lamp station disconnected from home AP

        sta_connected = 0;

        // TODO Implement reconnection
        ap_credentials_available = 0;

        xTaskNotify(networkTask, WIFI_SHUTDOWN_EVENT, eSetBits);

        // Give recalibration time to cap sensor
        TIME_DELAY_MILLIS(1000);
        cap_sensor_active = 1;
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        // Lamp station received an IP address from home AP
        event_sta_gotip = (ip_event_got_ip_t *)event_data;
        s_retry_num = 0;
        xTaskNotify(networkTask, WIFI_STA_CONNECTED_EVENT, eSetBits);

        // Give recalibration time to cap sensor
        TIME_DELAY_MILLIS(1000);
        cap_sensor_active = 1;
    }
}

void shutdown_wifi(void)
{
    if (network_task_working)
    {
        network_task_working = 0;
        if (is_provisioning)
        {
            deinit_wifi_provision();
        }
        is_provisioning = 0;
        deinit_wifi(&wifi_event_handler);
    }
}

static void network_task(void *params)
{
    static uint32_t networkNotificationValue;
    static esp_err_t _err;
    static unsigned char shutdown_notify = 0;

    // Sleep execution, to help calibrating capacitive sensor
    TIME_DELAY_MILLIS(500);

    for (;;)
    {
        if (shutdown_notify)
        {
            shutdown_notify = 0;
            xTaskNotify(networkTask, WIFI_SHUTDOWN_EVENT, eSetBits);
        }

        if (xTaskNotifyWait(0x00,                      /* Don't clear any notification bits on entry. */
                            0xffffffffUL,              /* Reset the notification value to 0 on exit. */
                            &networkNotificationValue, /* Notified value */
                            (TickType_t)20) == pdTRUE)
        {
            if (networkNotificationValue & WIFI_SHUTDOWN_EVENT_WAIT)
            {
                shutdown_wifi();
                stop_led_animation(ledStopSem, ledAnimationSem);
            }
            else if (networkNotificationValue & WIFI_START_AP_EVENT_WAIT)
            {
                printf("\nAP START EVENT\n");
                shutdown_wifi();
                if (ESP_OK == init_wifi_ap(&wifi_event_handler))
                {
                    // Send blink animation event
                    xTaskNotify(ledUpdaterTask, LED_BLINK_LOOP_START_EVENT, eSetBits);
                    network_task_working = 1;
                    continue;
                }
            }
            else if (networkNotificationValue & WIFI_AP_STA_CONNECTED_EVENT_WAIT)
            {
                printf("\nAP STA CONNECTED EVENT\n");
                if (event_ap_staconnected)
                {
                    printf("\nWIFI_AP_STA_CONNECTED_BIT " MACSTR " join, AID=%d\n", MAC2STR(event_ap_staconnected->mac), event_ap_staconnected->aid);
                    // Initialize Wifi provisioning
                    if (!is_provisioning)
                    {
                        if (ESP_OK == init_wifi_provision())
                        {
                            is_provisioning = 1;
                        }
                        else
                        {
                            shutdown_notify = 1;
                            continue;
                        }
                    }
                }
            }
            else if (networkNotificationValue & WIFI_AP_STA_DISCONNECTED_EVENT_WAIT)
            {
                printf("\nAP STA DISCONNECTED EVENT\n");
                if (event_ap_stadisconnected)
                {
                    printf("\nStation " MACSTR " leave, AID=%d\n", MAC2STR(event_ap_stadisconnected->mac), event_ap_stadisconnected->aid);
                    // Deinit Wifi provisioning
                    if (is_provisioning)
                    {
                        deinit_wifi_provision();
                    }
                    is_provisioning = 0;
                }
            }
            else if (networkNotificationValue & WIFI_START_STA_EVENT_WAIT)
            {
                printf("\nSTA START EVENT\n");
                cap_sensor_active = 0;
                TIME_DELAY_MILLIS(500); // Give time to cap sensor for deactivating

                shutdown_wifi();
                if (ap_credentials_available &&
                    ESP_OK == init_wifi_sta(&wifi_event_handler,
                                            ussid.string_array, ussid.string_len,
                                            upwd.string_array, upwd.string_len))
                {
                    network_task_working = 1;
                }
            }
            else if (networkNotificationValue & WIFI_STA_CONNECTED_EVENT_WAIT)
            {
                if (event_sta_gotip)
                {
                    printf("\nIP_EVENT GOT IP \nMAC=" MACSTR "GW=%s IP=%s NM=%s\n",
                           MAC2STR(event_ap_staconnected->mac),
                           ip4addr_ntoa(&event_sta_gotip->ip_info.gw),
                           ip4addr_ntoa(&event_sta_gotip->ip_info.ip),
                           ip4addr_ntoa(&event_sta_gotip->ip_info.netmask));
                    sta_connected = 1;
                }
            }
        }

        if (sta_connected && network_task_working)
        {
            printf("\nSTA CONNECTED\n");
        }
        else if (ap_credentials_available && !network_task_working)
        {
            printf("\nSTA START\n");
            xTaskNotify(networkTask, WIFI_START_STA_EVENT, eSetBits);
        }
        else if (is_provisioning && network_task_working)
        {
            // Wifi provisioning semi-blocking listen
            printf("\nprovision_listen...\n");
            if (ESP_ERR_TIMEOUT == (_err = provision_listen(&ussid, &upwd)))
            {
                continue;
            }
            else if (_err == ESP_OK)
            {
                // Retrieved SSID and password, ending provisioning

                if (ESP_OK == save_user_ap_ssid(ussid.string_array, ussid.string_len) &&
                    ESP_OK == save_user_ap_password(upwd.string_array, upwd.string_len))
                {
                    printf("\nWifi Credentials retrieved!\n");
                    printf("\nSSID -> %s\nPassword -> %s\n", ussid.string_array, upwd.string_array);
                    ap_credentials_available = 1;
                }
            }
            deinit_wifi_provision();
            is_provisioning = 0;
            xTaskNotify(networkTask, WIFI_SHUTDOWN_EVENT, eSetBits);
            printf("\nCALLING SHUTDOWN\n");
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

        if (RegisterNetworkPackets())
        {
            // Create button task
            xTaskCreate(network_task,
                        "network_task",
                        NETWORK_TASK_STACK_DEPTH,
                        NULL,
                        NETWORK_TASK_PRIORITY,
                        &networkTask);
        }

        if (ESP_OK == get_user_ap_ssid_string(&ussid) &&
            ESP_OK == get_user_ap_password_string(&upwd))
        {
            ap_credentials_available = 1;
            printf("\nWIFI Credentials set\n");
        }
        else
        {
            ap_credentials_available = 0;
        }
    }
}
