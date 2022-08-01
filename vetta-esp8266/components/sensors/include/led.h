#ifndef _VETTA_LED_H

#include "esp_err.h"

#define PWM_PERIOD                  (1000)      // 1 kHz
#define PWM_CHANNEL                 (0)         // 1 for output led channel

#define LED_GPIO_PIN_OUTPUT_VALUE   (15)        // Output pin D8
#define PWM_PHASE_VALUE             (90.0)

#define LED_OFF_DUTY_CYCLE          (0)
#define LED_LOW_DUTY_CYCLE          (300)       // (0x99a)     // ~30% PWM_PERIOD
#define LED_MEDIUM_DUTY_CYCLE       (750)       // (0x1335)    // ~60% PWM_PERIOD
#define LED_HIGH_DUTY_CYCLE         (1000)      // (0x2004)    // 100% PWM_PERIOD

/* Configures led and pwm modules (non-thread safe)
@return ESP_OK on success, esp_err_t value in case of errors
*/
esp_err_t init_led_module(void);

/* Switch the led to off state, calls pwm_stop() and pwm_deinit() right after. (non-thread safe)
@return ESP_OK on success, ESP_ERR_INVALID_ARG in case of errors
*/
esp_err_t led_shutdown(void);

/* Switch the led to off state, pwm_stop() and pwm_deinit() are not called  (non-thread safe)
@return ESP_OK on success, ESP_ERR_INVALID_ARG in case of errors
*/
esp_err_t led_off(void);

/* Switch the led to low state (non-thread safe)
@return ESP_OK on success, ESP_ERR_INVALID_ARG in case of errors
*/
esp_err_t led_low(void);

/* Switch the led to medium state (non-thread safe)
@return ESP_OK on success, ESP_ERR_INVALID_ARG in case of errors
*/
esp_err_t led_medium(void);

/* Switch the led to high state (non-thread safe)
@return ESP_OK on success, ESP_ERR_INVALID_ARG in case of errors
*/
esp_err_t led_high(void);

#define _VETTA_LED_H
#endif // _VETTA_LED_H
