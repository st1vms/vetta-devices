idf_component_register(SRCS "led.c" "touch.c", "utils.h"
                       INCLUDE_DIRS "include"
                       PRIVATE_HEADER   "driver/pwm.h"
                                        "esp8266/gpio_register.h"
                                        "esp8266/pin_mux_register.h"
                                        "driver/adc.h"
                                        "esp_err.h"
                                        "utils.h")
