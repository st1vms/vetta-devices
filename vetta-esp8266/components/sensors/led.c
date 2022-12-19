#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "esp8266/gpio_register.h"
#include "esp8266/pin_mux_register.h"
#include "driver/pwm.h"
#include "led.h"

static uint32_t _current_state;

static inline void __TIME_DELAY_MILLIS(long int x) { vTaskDelay(x / portTICK_PERIOD_MS); };

esp_err_t init_led_module(void)
{
    esp_err_t _err;
    const uint32_t LED_GPIO_PIN_OUTPUT[] = {LED_GPIO_PIN_OUTPUT_VALUE};
    float PWM_PHASE[] = {PWM_PHASE_VALUE};
    uint32_t start_duty[] = {LED_OFF_DUTY_CYCLE};

    // Initializie PWM, set phases, starts PWM
    if (
        ESP_OK == (_err = pwm_init(PWM_PERIOD,
                                   start_duty,
                                   1, // 1 channel
                                   LED_GPIO_PIN_OUTPUT)) &&
        ESP_OK == (_err = pwm_set_phases(PWM_PHASE)) &&
        ESP_OK == (_err = pwm_start()))
    {
        return ESP_OK;
    }
    return _err;
}

static esp_err_t set_duty(uint32_t duty)
{
    if (ESP_OK == pwm_set_duty(PWM_CHANNEL, duty) && ESP_OK == pwm_start())
    {
        _current_state = duty;
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}

esp_err_t led_shutdown(void)
{
    esp_err_t _err;
    if (
        ESP_OK == (_err = led_off()) &&
        ESP_OK == (_err = pwm_stop(PWM_CHANNEL)) &&
        ESP_OK == (_err = pwm_deinit()))
    {
        return ESP_OK;
    }
    return _err;
};

esp_err_t led_off(void) { return set_duty(LED_OFF_DUTY_CYCLE); };

esp_err_t led_low(void) { return set_duty(LED_LOW_DUTY_CYCLE); }

esp_err_t led_medium(void) { return set_duty(LED_MEDIUM_DUTY_CYCLE); };

esp_err_t led_high(void) { return set_duty(LED_HIGH_DUTY_CYCLE); };

esp_err_t led_set_next(void)
{
    switch (_current_state)
    {
    case LED_OFF_DUTY_CYCLE:
        return led_low();
    case LED_LOW_DUTY_CYCLE:
        return led_medium();
    case LED_MEDIUM_DUTY_CYCLE:
        return led_high();
    case LED_HIGH_DUTY_CYCLE:
        return led_off();
    default:
        break;
    }
    return ESP_FAIL;
}


static esp_err_t set_led_animation(led_animation_t *animation, size_t step_count, led_animation_step_t *steps, unsigned char do_loop)
{
    if (NULL == animation || NULL == steps || !step_count)
    {
        return ESP_ERR_INVALID_ARG;
    }

    animation->canceled = 0;
    animation->is_playing = 0;
    animation->do_loop = do_loop;
    animation->step_size = step_count;
    animation->steps_buf = steps;

    return ESP_OK;
}

esp_err_t play_led_animation(led_animation_t *animation)
{
    if (NULL == animation->steps_buf ||
        animation->step_size == 0 ||
        animation->canceled ||
        animation->is_playing)
    {
        return ESP_ERR_INVALID_ARG;
    }

    animation->is_playing = 1;
    do
    {
        for (unsigned int i = 0; i < animation->step_size; i++)
        {
            __TIME_DELAY_MILLIS(animation->steps_buf[i].start_delay);

            if (animation->canceled)
            {
                break;
            }

            set_duty(animation->steps_buf[i].duty_cycle);

            __TIME_DELAY_MILLIS(animation->steps_buf[i].end_delay);
        }
    } while (!animation->canceled && animation->do_loop);

    led_off();

    animation->is_playing = 0;
    return ESP_OK;
}

void stop_led_animation(led_animation_t *animation)
{
    if (NULL != animation && animation->is_playing)
    {
        animation->canceled = 1;
    }
}

static led_animation_step_t _OFF_HIGH_BLINK_STEPS[] = {
    (led_animation_step_t){
        .duty_cycle = LED_HIGH_DUTY_CYCLE,
        .start_delay = 0,
        .end_delay = 500},
    (led_animation_step_t){
        .duty_cycle = LED_OFF_DUTY_CYCLE,
        .start_delay = 0,
        .end_delay = 1000}
};

esp_err_t SET_BLINK_OFF_HIGH_ANIMATION(led_animation_t *animation, unsigned char do_loop)
{
    return set_led_animation(animation, 2, _OFF_HIGH_BLINK_STEPS, do_loop);
}
