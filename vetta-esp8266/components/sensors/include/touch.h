#ifndef _VETTA_TOUCH_H

#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_SENSOR_READING_VALUE        (0)

#define READING_SAMPLES_POOL_SIZE           (10)

#define READING_DELAY_MILLIS                (10)

#define TOUCH_DETECTION_TRESHOLD            (5)

/* Configures capacitive touch sensor module (non-thread safe)
@return ESP_OK on success, esp_err_t value in case of errors
*/
esp_err_t init_touch_sensor_module(void);

/*
@return Capacitive sensor analog reading, as a 16 bit unsigned integer
*/
uint16_t read_sensor_analog(void);

/*
@param analog_value sensor reading value to check for touch
@param calibrated_idle_read the idle reading value, retrieved from calibrate_idle()
@return 1 in case of positive evaluation ( touch detected ), 0 otherwise
*/
unsigned char is_touch(uint16_t analog_value, uint16_t calibrated_idle_read);

#ifdef __cplusplus
}
#endif

#define _VETTA_TOUCH_H
#endif // _VETTA_TOUCH_H
