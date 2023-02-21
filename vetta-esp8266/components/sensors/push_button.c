#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "push_button.h"

// From sdkconfig.h
#ifndef CONFIG_FREERTOS_HZ
#define CONFIG_FREERTOS_HZ 100
#endif
static unsigned char is_pressed = 0;

static const gpio_config_t io_conf = {
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .pin_bit_mask = (1ULL << BUTTON_GPIO_NUMBER),
    .intr_type = GPIO_INTR_ANYEDGE};

esp_err_t init_button(gpio_isr_t isr_handler)
{

    static esp_err_t _err;

    // Configure GPIO pin and install ISR service
    if (ESP_OK != (_err = gpio_config(&io_conf)) ||
        ESP_OK != (_err = gpio_install_isr_service(0)))
    {
        return _err;
    }

    // Add/Register the ISR handler
    return gpio_isr_handler_add(BUTTON_GPIO_NUMBER, isr_handler, NULL);
}

static inline TickType_t getTickMillis()
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

static TickType_t start = 0, end = 0;
PressEvent_t get_press_event(void)
{

    // Pull-down -> 1 pressed | 0 released
    if (gpio_get_level(BUTTON_GPIO_NUMBER))
    {
        is_pressed = 1;
        start = getTickMillis();
        end = start;
    }
    else if (is_pressed)
    {
        is_pressed = 0;
        end = getTickMillis();
        if (end - start > DISCOVERY_REASON_PRESS_DELAY_MILLIS)
        {
            if (end - start > RESET_REASON_PRESS_DELAY_MILLIS)
            {
                return PRESS_EVENT_RESET;
            }
            return PRESS_EVENT_DISCOVERY;
        }
    }

    return PRESS_EVENT_ERROR;
}
