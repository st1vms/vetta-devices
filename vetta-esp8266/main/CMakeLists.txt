idf_component_register(SRCS "vetta_main.c"
                        PRIVATE_HEADER "espconfig_overwrite.h"
                        INCLUDE_DIRS)
# Flash SPIFFS storage
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)
