#ifndef _VETTA_TOUCH_H

#include "esp_err.h"

#define CAPACITIVE_SENSOR_GPIO_OUTPUT       (12)    //  Output pin D6

#define DEFAULT_SENSOR_READING_VALUE        (0)

#define READING_SAMPLES_POOL_SIZE           (20)

#define READING_DELAY_MILLIS                (10)

#define CALIBRATION_TIMEOUT_MILLIS          (2000)

#define CALIBRATION_SAMPLES                 (20)

#define TOUCH_DETECTION_MIN_DELTA           (10)
#define TOUCH_DETECTION_MAX_DELTA           (100)


#ifndef CONFIG_FREERTOS_HZ
#define CONFIG_FREERTOS_HZ  (100)   //menuconfig -> component config -> FreeRTOS -> Tick rate (hz)
#endif //CONFIG_FREERTOS_HZ
static inline void DELAY_MILLIS(long int x) {vTaskDelay(x / portTICK_PERIOD_MS);};

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
@return 1 in case of positive evaluation ( touch detected ), 0 otherwise
*/
unsigned char is_touch(uint16_t analog_value);

#define _VETTA_TOUCH_H
#endif // _VETTA_TOUCH_H
