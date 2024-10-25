#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp8266/gpio_register.h"
#include "esp8266/pin_mux_register.h"
#include "driver/pwm.h"
#include "led.h"

static uint32_t _current_state = LED_OFF_DUTY_CYCLE;

static led_animation_t current_led_animation = {
    .canceled = 0,
    .is_playing = 0,
    .reps = 0,
    .step_size = 0,
    .steps_buf = NULL};

// From sdkconfig.h
#ifndef CONFIG_FREERTOS_HZ
#define CONFIG_FREERTOS_HZ 100
#endif

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
        _current_state = LED_OFF_DUTY_CYCLE;
        return ESP_OK;
    }
    return _err;
}

// TODO Implement "smooth" PWM

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

static esp_err_t set_led_animation(size_t step_count, led_animation_step_t *steps, signed char reps)
{
    if (NULL == steps || !step_count)
    {
        return ESP_ERR_INVALID_ARG;
    }

    current_led_animation.canceled = 0;
    current_led_animation.is_playing = 0;
    current_led_animation.reps = reps;
    current_led_animation.step_size = step_count;
    current_led_animation.steps_buf = steps;

    return ESP_OK;
}

esp_err_t play_led_animation(SemaphoreHandle_t ledStopSem, SemaphoreHandle_t ledAnimationSem)
{
    if (NULL == current_led_animation.steps_buf ||
        current_led_animation.step_size == 0 ||
        current_led_animation.is_playing)
    {
        return ESP_ERR_INVALID_ARG;
    }

    current_led_animation.is_playing = 1;
    unsigned int i = 0;

    while (i <= current_led_animation.step_size)
    {
        if (pdTRUE == xSemaphoreTake(ledStopSem, current_led_animation.steps_buf[i].start_delay / portTICK_PERIOD_MS))
        {
            break;
        }
        set_duty(current_led_animation.steps_buf[i].duty_cycle);
        if (pdTRUE == xSemaphoreTake(ledStopSem, current_led_animation.steps_buf[i].end_delay / portTICK_PERIOD_MS))
        {
            break;
        }

        if (++i == current_led_animation.step_size && (current_led_animation.reps == -1 ||
                                                       (current_led_animation.reps)-- > 0))
        {
            i = 0;
        }
    }

    set_duty(LED_OFF_DUTY_CYCLE);

    current_led_animation.is_playing = 0;
    xSemaphoreGive(ledAnimationSem);

    return ESP_OK;
}

void stop_led_animation(SemaphoreHandle_t ledStopSem, SemaphoreHandle_t ledAnimationSem)
{
    if (!current_led_animation.is_playing)
    {
        return;
    }

    if (pdTRUE == xSemaphoreGive(ledStopSem) &&
        pdTRUE == xSemaphoreTake(ledAnimationSem, portMAX_DELAY))
    {
        xSemaphoreTake(ledStopSem, (TickType_t)10);
    }
}

esp_err_t SET_BLINK_OFF_HIGH_ANIMATION(signed char reps)
{
    static led_animation_step_t _OFF_HIGH_BLINK_STEPS[] = {
        (led_animation_step_t){
            .duty_cycle = LED_HIGH_DUTY_CYCLE,
            .start_delay = 0,
            .end_delay = 500},
        (led_animation_step_t){
            .duty_cycle = LED_OFF_DUTY_CYCLE,
            .start_delay = 0,
            .end_delay = 500}};
    return set_led_animation(2, _OFF_HIGH_BLINK_STEPS, reps);
}

esp_err_t SET_FAST_BLINK_ANIMATION(signed char reps)
{
    static led_animation_step_t _OFF_HIGH_BLINK_STEPS[] = {
        (led_animation_step_t){
            .duty_cycle = LED_HIGH_DUTY_CYCLE,
            .start_delay = 0,
            .end_delay = 250},
        (led_animation_step_t){
            .duty_cycle = LED_OFF_DUTY_CYCLE,
            .start_delay = 0,
            .end_delay = 250}};
    return set_led_animation(2, _OFF_HIGH_BLINK_STEPS, reps);
}


uint8_t led_get_state(void){
    switch (_current_state)
    {
    case LED_OFF_DUTY_CYCLE:
        return 0;
    case LED_LOW_DUTY_CYCLE:
        return 1;
    case LED_MEDIUM_DUTY_CYCLE:
        return 2;
    case LED_HIGH_DUTY_CYCLE:
        return 3;
    }
    return 0;
}
