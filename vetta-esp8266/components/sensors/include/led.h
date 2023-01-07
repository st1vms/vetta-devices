#ifndef _VETTA_LED_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define PWM_PERIOD (1000) // 1 kHz
#define PWM_CHANNEL (0)   // 1 for output led channel

#define LED_GPIO_PIN_OUTPUT_VALUE (15) // Output pin D8
#define PWM_PHASE_VALUE (90.0)

    typedef enum led_step_duty_t
    {
        LED_OFF_DUTY_CYCLE = 0,
        LED_LOW_DUTY_CYCLE = 300,    // (0x99a)     // ~30% PWM_PERIOD
        LED_MEDIUM_DUTY_CYCLE = 750, // (0x1335)    // ~60% PWM_PERIOD
        LED_HIGH_DUTY_CYCLE = 1000   // (0x2004)    // 100% PWM_PERIOD
    } led_step_duty_t;

    typedef struct led_animation_step_t
    {
        const led_step_duty_t duty_cycle;
        const unsigned long int start_delay;
        const unsigned long int end_delay;
    } led_animation_step_t;

    // Ring buffer
    typedef struct led_animation_t
    {
        unsigned char is_playing;
        unsigned char canceled;
        unsigned int step_size;
        signed char reps;
        led_animation_step_t *steps_buf;
    } led_animation_t;

    esp_err_t SET_BLINK_OFF_HIGH_ANIMATION(signed char reps);

    esp_err_t play_led_animation(SemaphoreHandle_t ledStopSem, SemaphoreHandle_t ledAnimationSem);

    void stop_led_animation(SemaphoreHandle_t ledStopSem, SemaphoreHandle_t ledAnimationSem);

    /* Configures led and pwm modules (non-thread safe)
    @return ESP_OK on success, esp_err_t value in case of errors
    */
    esp_err_t init_led_module(void);

    /* Switch the led to off state, calls pwm_stop() and pwm_deinit() right after. (non-thread safe)
    @return ESP_OK on success, esp_err_t value in case of errors
    */
    esp_err_t led_shutdown(void);

    /* Switch the led to off state, pwm_stop() and pwm_deinit() are not called  (non-thread safe)
    @return ESP_OK on success, esp_err_t value in case of errors
    */
    esp_err_t led_off(void);

    /* Switch the led to low state (non-thread safe)
    @return ESP_OK on success, esp_err_t value in case of errors
    */
    esp_err_t led_low(void);

    /* Switch the led to medium state (non-thread safe)
    @return ESP_OK on success, esp_err_t value in case of errors
    */
    esp_err_t led_medium(void);

    /* Switch the led to high state (non-thread safe)
    @return ESP_OK on success, esp_err_t value in case of errors
    */
    esp_err_t led_high(void);

    /*  Switch the led next state (non-thread safe)
    @return @return ESP_OK on success, esp_err_t value in case of errors
    */
    esp_err_t led_set_next(void);

#ifdef __cplusplus
}
#endif

#define _VETTA_LED_H
#endif // _VETTA_LED_H
