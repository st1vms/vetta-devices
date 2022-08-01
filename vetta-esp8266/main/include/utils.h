#ifndef __VETTA_UTILS_H

#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"

inline void DELAY_MILLIS(long int x) {vTaskDelay(x / portTICK_PERIOD_MS);};

#define __VETTA_UTILS_H
#endif // __VETTA_UTILS_H
