#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "touch.h"

static long int idle_reading = 0;

uint16_t read_sensor_analog(void)
{
    static uint16_t _input_value;
    _input_value = idle_reading;
    if(ESP_OK == adc_read(&_input_value))
    {
        return _input_value;
    }
    return idle_reading;
}

static unsigned char wait_for_calibration(void)
{
    time_t start_time = 0;
    long int prev_value = 0, count = 0;

    while(start_time < CALIBRATION_TIMEOUT_MILLIS || count < CALIBRATION_SAMPLES)
    {
        if(!(prev_value = read_sensor_analog()))
        {
            return 0;
        }else if(prev_value != idle_reading)
        {
            idle_reading = prev_value;
        }else{count++;}

        DELAY_MILLIS(READING_DELAY_MILLIS);
        start_time += READING_DELAY_MILLIS;
    }

    return 1;
}

esp_err_t init_touch_sensor_module(void)
{
    adc_config_t _analog_input_pin_config = (adc_config_t) {
        .mode = ADC_READ_TOUT_MODE,
        .clk_div = 32  // (menuconfig->Component config->PHY->vdd33_const) - 1
    };

    static esp_err_t _err;

    if(ESP_OK == ( _err = adc_init(&_analog_input_pin_config)) && wait_for_calibration())
    {return ESP_OK;}

    return _err;
}


unsigned char is_touch(uint16_t analog_value)
{
    if(analog_value > idle_reading || analog_value == DEFAULT_SENSOR_READING_VALUE)
    {
        return 0;
    }

    return TOUCH_DETECTION_MIN_DELTA <= (idle_reading - analog_value) &&
            (idle_reading - analog_value) <= TOUCH_DETECTION_MAX_DELTA ? 1 : 0;
}
