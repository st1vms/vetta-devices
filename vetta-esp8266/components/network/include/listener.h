#ifndef __LISTENER_H

#include "esp_err.h"
#include "esp_event.h"
#include "tcpip_adapter.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define LISTENER_SERVER_BUFFER_SIZE (128)

#define LISTENER_SERVER_PORT (50032)

    typedef enum listener_event_t{
        RESULT_FAIL = ESP_FAIL,
        RESULT_NO_ACTION = ESP_OK,
        RESULT_LED_OFF,
        RESULT_LED_LOW,
        RESULT_LED_MEDIUM,
        RESULT_LED_HIGH,
        RESULT_LED_NEXT,
    }listener_event_t;

    esp_err_t init_listener_server(u32_t ip_info);

    void close_listener_server(void);

    listener_event_t listener_listen(void);

#ifdef __cplusplus
}
#endif
#define __LISTENER_H
#endif // __LISTENER_H
