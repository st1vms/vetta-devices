#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "touch.h"

uint16_t read_sensor_analog(void)
{
    static uint16_t _input_value;
    _input_value = DEFAULT_SENSOR_READING_VALUE;
    if (ESP_OK == adc_read(&_input_value))
    {
        return _input_value;
    }
    return DEFAULT_SENSOR_READING_VALUE;
}

esp_err_t init_touch_sensor_module(void)
{
    adc_config_t _analog_input_pin_config = (adc_config_t){
        .mode = ADC_READ_TOUT_MODE,
        .clk_div = 33 // (menuconfig->Component config->PHY->vdd33_const) - 1
    };

    static esp_err_t _err;

    if (ESP_OK == (_err = adc_init(&_analog_input_pin_config)))
    {
        return ESP_OK;
    }

    return _err;
}

unsigned char is_touch(int32_t analog_value, uint16_t calibrated_idle_read)
{
    if (analog_value <= TOUCH_DETECTION_TRESHOLD)
    {
        return 0;
    }
    return abs(analog_value - calibrated_idle_read) >= TOUCH_DETECTION_TRESHOLD ? 1 : 0;
}
