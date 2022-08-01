#ifndef _VETTA_TOUCH_H

#include "esp_err.h"

#define CAPACITIVE_SENSOR_GPIO_OUTPUT       (12)    //  Output pin D6

#define DEFAULT_SENSOR_READING_VALUE        (0)

#define READING_SAMPLES_POOL_SIZE           (20)

#define READING_DELAY_MILLIS                (50)

#define CALIBRATION_TIMEOUT_MILLIS          (2000)

#define CALIBRATION_SAMPLES                 (20)

#define TOUCH_DETECTION_THRESHOLD           (15)

/* Configures capacitive touch sensor module (non-thread safe)
@return ESP_OK on success, esp_err_t value in case of errors
*/
esp_err_t init_touch_sensor_module(void);

/*
@return Capacitive sensor analog reading, as a 16 bit unsigned integer
*/
uint16_t read_sensor_analog(void);

#define _VETTA_TOUCH_H
#endif // _VETTA_TOUCH_H
