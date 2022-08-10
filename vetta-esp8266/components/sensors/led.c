#include "esp8266/gpio_register.h"
#include "esp8266/pin_mux_register.h"
#include "driver/pwm.h"
#include "esp_log.h"
#include "led.h"

static uint32_t _current_state;

esp_err_t init_led_module(void)
{
    esp_err_t _err;
    const uint32_t LED_GPIO_PIN_OUTPUT[] = {LED_GPIO_PIN_OUTPUT_VALUE};
    float PWM_PHASE[] = {PWM_PHASE_VALUE};
    uint32_t start_duty[] = {LED_OFF_DUTY_CYCLE};

    // Initializie PWM, set phases, starts PWM
    if(
        ESP_OK == (_err = pwm_init(PWM_PERIOD,
            start_duty,
            1,  // 1 channel
            LED_GPIO_PIN_OUTPUT)
        ) &&
        ESP_OK == (_err = pwm_set_phases(PWM_PHASE)) &&
        ESP_OK == (_err = pwm_start()))
    {
        return ESP_OK;
    }
    return _err;
}

static esp_err_t set_duty(uint32_t duty)
{
    return (ESP_OK == pwm_set_duty(PWM_CHANNEL, duty) && ESP_OK == pwm_start())
        ? ESP_OK : ESP_ERR_INVALID_ARG;
}

esp_err_t led_shutdown(void) {
    esp_err_t _err;
    if(
        ESP_OK == (_err = set_duty(LED_OFF_DUTY_CYCLE)) &&
        ESP_OK == (_err = pwm_stop(PWM_CHANNEL)) &&
        ESP_OK == (_err = pwm_deinit()))
    {
        return ESP_OK;
    }
    return _err;
};

esp_err_t led_off(void) {return set_duty(LED_OFF_DUTY_CYCLE);};

esp_err_t led_low(void) {return set_duty(LED_LOW_DUTY_CYCLE);}

esp_err_t led_medium(void) {return set_duty(LED_MEDIUM_DUTY_CYCLE);};

esp_err_t led_high(void){return set_duty(LED_HIGH_DUTY_CYCLE);};

esp_err_t led_set_next(void)
{
    switch (_current_state)
    {
    case LED_OFF_DUTY_CYCLE:
        if(ESP_OK == set_duty(LED_LOW_DUTY_CYCLE))
        {
            _current_state = LED_LOW_DUTY_CYCLE;
            return ESP_OK;
        }
        break;
    case LED_LOW_DUTY_CYCLE:
        if(ESP_OK == set_duty(LED_MEDIUM_DUTY_CYCLE))
        {
            _current_state = LED_MEDIUM_DUTY_CYCLE;
            return ESP_OK;
        }
        break;
    case LED_MEDIUM_DUTY_CYCLE:
        if(ESP_OK == set_duty(LED_HIGH_DUTY_CYCLE))
        {
            _current_state = LED_HIGH_DUTY_CYCLE;
            return ESP_OK;
        }
        break;
    case LED_HIGH_DUTY_CYCLE:
        if(ESP_OK == set_duty(LED_OFF_DUTY_CYCLE))
        {
            _current_state = LED_OFF_DUTY_CYCLE;
            return ESP_OK;
        }
        break;
    default:
        break;
    }
    return ESP_FAIL;
}
