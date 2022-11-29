#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "touch.h"


uint16_t read_sensor_analog(void)
{
    static uint16_t _input_value;
    _input_value = DEFAULT_SENSOR_READING_VALUE;
    if(ESP_OK == adc_read(&_input_value))
    {
        return _input_value;
    }
    return DEFAULT_SENSOR_READING_VALUE;
}

uint16_t calibrate_idle(void)
{
    static time_t time_offset;
    time_offset = 0;

    static uint16_t res;
    res = DEFAULT_SENSOR_READING_VALUE;

    static uint16_t prev;

    for(;;)
    {
        prev = res;

        if(!(res = read_sensor_analog()))
        {
            break;
        }

        if(time_offset >= CALIBRATION_DELAY_MILLIS &&
            // Inclusive range test
            (res >= prev - CALIBRATION_ACCURACY_DELTA && res <= prev + CALIBRATION_ACCURACY_DELTA))
        {
            return res;
        }

        TIME_DELAY_MILLIS(READING_DELAY_MILLIS);
        time_offset += READING_DELAY_MILLIS;
    }

    return DEFAULT_SENSOR_READING_VALUE;
}


esp_err_t init_touch_sensor_module(void)
{
    adc_config_t _analog_input_pin_config = (adc_config_t) {
        .mode = ADC_READ_TOUT_MODE,
        .clk_div = 32  // (menuconfig->Component config->PHY->vdd33_const) - 1
    };

    static esp_err_t _err;

    if(ESP_OK == ( _err = adc_init(&_analog_input_pin_config)))
    {return ESP_OK;}

    return _err;
}


unsigned char is_touch(uint16_t analog_value, uint16_t calibrated_idle_read)
{
    if(analog_value > calibrated_idle_read || analog_value == DEFAULT_SENSOR_READING_VALUE)
    {
        return 0;
    }

    return TOUCH_DETECTION_MIN_DELTA <= (calibrated_idle_read - analog_value) &&
            (calibrated_idle_read - analog_value) <= TOUCH_DETECTION_MAX_DELTA ? 1 : 0;
}
