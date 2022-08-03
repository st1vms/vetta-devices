#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "led.h"
#include "touch.h"

#define configTASK_NOTIFICATION_ARRAY_ENTRIES       (1)

#define LED_UPDATER_TASK_STACK_DEPTH                (2048)

#define CAPACITIVE_SENSOR_TASK_STACK_DEPTH          (2048)

#define CAPACITIVE_SENSOR_LED_NOTIFY_DELAY          (500)
#define CAPACITIVE_SENSOR_LED_NOTIFICATION_VALUE    (0x1)

static TaskHandle_t ledUpdaterTask = NULL;
static TaskHandle_t capSensorTask = NULL;

static void led_updater_task(void * params)
{
    uint32_t ledNotificationValue;

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
    static uint16_t samples[READING_SAMPLES_POOL_SIZE] = {0};
    size_t ix = 0;

    unsigned char calibrated = 0;
    unsigned char can_update = 0;

    time_t time_offset = 0;
    long int total = 0;

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

        if(calibrated && can_update && is_touch(total / READING_SAMPLES_POOL_SIZE))
        {
            can_update = 0;
            time_offset = 0;

            // send led update event
            xTaskNotifyGive(ledUpdaterTask);
        }

        DELAY_MILLIS(READING_DELAY_MILLIS);

        if(time_offset >= CAPACITIVE_SENSOR_LED_NOTIFY_DELAY)
        {
            can_update = 1;
            continue;
        }
        time_offset += READING_DELAY_MILLIS;
    }
}


void app_main()
{
    esp_err_t _err;
    if(ESP_OK != (_err = init_led_module()) || ESP_OK != (_err = led_low()))
    {
        printf("\nLED PWM module error {%d}\n", _err);
        return;
    }

    if(ESP_OK != (_err = init_touch_sensor_module()))
    {
        printf("\nTouch sensor module error {%d}\n", _err);
        return;
    }

    // Create capacitive sensor task
    xTaskCreate(capacitive_sensor_task,
                "sensor_task",
                CAPACITIVE_SENSOR_TASK_STACK_DEPTH,
                NULL,
                tskIDLE_PRIORITY,
                &capSensorTask);

    // Create led updater task
    xTaskCreate(led_updater_task,
                "led_task",
                LED_UPDATER_TASK_STACK_DEPTH,
                NULL,
                tskIDLE_PRIORITY,
                &ledUpdaterTask);
}
