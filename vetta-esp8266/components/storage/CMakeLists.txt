idf_component_register(SRCS "storage.c"
                       INCLUDE_DIRS "include"
                       PRIVATE_HEADER   "string.h"
                                        "sys/unistd.h"
                                        "sys/stat.h"
                                        "esp_spiffs.h"
                                        "esp_err.h")
