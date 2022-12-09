#ifndef _BUTTON_H

#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// Pull-up push button

static const gpio_num_t BUTTON_GPIO_NUMBER = GPIO_NUM_4;

#define BUTTON_PIN (1ULL<<BUTTON_GPIO_NUMBER)

// Initialize ISR for pull-up button ( intr_type LOW )
esp_err_t init_button(gpio_isr_t isr_handler);

#ifdef __cplusplus
}
#endif

#define _BUTTON_H
#endif //_BUTTON_H
