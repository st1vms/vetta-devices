#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "button.h"

esp_err_t init_button(gpio_isr_t isr_handler){

    static esp_err_t _err;

    // Configure the pins
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pin_bit_mask = (1ULL<<BUTTON_GPIO_NUMBER);
    io_conf.intr_type = GPIO_INTR_LOW_LEVEL;

    // Configure GPIO pin and install ISR service
    if( ESP_OK != (_err = gpio_config(&io_conf)) ||
        ESP_OK != (_err = gpio_install_isr_service(0))){
        return _err;
    }

    // Add/Register the ISR handler
    return gpio_isr_handler_add(BUTTON_GPIO_NUMBER, isr_handler, NULL);
}
