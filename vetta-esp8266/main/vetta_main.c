#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "utils.h"
#include "led.h"
#include "touch.h"

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

}
