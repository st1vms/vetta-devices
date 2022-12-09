idf_component_register(SRCS "wifi_manager.c"
                       INCLUDE_DIRS "include"
                       PRIVATE_HEADER   "freertos/FreeRTOS.h"
                                        "freertos/FreeRTOSConfig.h"
                                        "freertos/task.h"
                                        "esp_wifi.h"
                                        "tcpip_adapter.h"
                                        "esp_err.h")
