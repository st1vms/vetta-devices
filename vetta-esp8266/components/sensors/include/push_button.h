#ifndef _PUSH_BUTTON_H

#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

    static const gpio_num_t BUTTON_GPIO_NUMBER = GPIO_NUM_13;

#define BUTTON_PIN (1ULL << BUTTON_GPIO_NUMBER)

#define RESET_REASON_PRESS_DELAY_MILLIS (5000UL)

#define DISCOVERY_REASON_PRESS_DELAY_MILLIS (1000UL)

    typedef enum
    {
        PRESS_EVENT_ERROR,
        PRESS_EVENT_RESET,
        PRESS_EVENT_DISCOVERY,
    } PressEvent_t;

    // Initialize ISR for pull-down button ( intr_type LOW )
    esp_err_t init_button(gpio_isr_t isr_handler);

    PressEvent_t get_press_event(void);

#ifdef __cplusplus
}
#endif

#define _PUSH_BUTTON_H
#endif //_PUSH_BUTTON_H
