idf_component_register(SRCS "wifi_manager.c" "wifi_provision.c" "packets.c" "discovery.c" "listener.c"
                       INCLUDE_DIRS "include"
                       PRIVATE_HEADER   "freertos/FreeRTOS.h"
                                        "freertos/FreeRTOSConfig.h"
                                        "freertos/event_groups.h"
                                        "esp_system.h"
                                        "esp_event.h"
                                        "nvs.h"
                                        "nvs_flash.h"
                                        "lwip/err.h"
                                        "lwip/sockets.h"
                                        "lwip/sys.h"
                                        "esp_err.h"
                                        "dbits.h"
                                        "storage.h")

set(COMPONENT_EMBED_TXTFILES "lamp.pem" "lamp.key" "ca.pem")
