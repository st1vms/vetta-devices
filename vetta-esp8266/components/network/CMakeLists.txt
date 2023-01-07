idf_component_register(SRCS "wifi_manager.c"
                       INCLUDE_DIRS "include"
                       PRIVATE_HEADER   "freertos/FreeRTOS.h"
                                        "freertos/FreeRTOSConfig.h"
                                        "freertos/event_groups.h"
                                        "esp_event.h"
                                        "esp_err.h")
