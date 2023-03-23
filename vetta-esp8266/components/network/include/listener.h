#ifndef __LISTENER_H

#include "esp_err.h"
#include "esp_event.h"
#include "tcpip_adapter.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define LISTENER_PING_DELAY (5000)
#define LISTENER_SERVER_SELECT_TIMEOUT (500)

#define LISTENER_PING_STEPS (10)
#define LISTENER_SERVER_BUFFER_SIZE (128)

#define LISTENER_SERVER_PORT (50032)

    typedef enum listener_event_t{
        RESULT_FAIL = ESP_FAIL,
        RESULT_NO_ACTION = ESP_OK,
        RESULT_CLIENT_STALE,
        RESULT_LED_OFF,
        RESULT_LED_LOW,
        RESULT_LED_MEDIUM,
        RESULT_LED_HIGH,
        RESULT_LED_NEXT,
    }listener_event_t;

    esp_err_t init_listener_server(u32_t ip_info);

    void close_listener_server(void);

    listener_event_t listener_listen(void);

    esp_err_t send_state_ping(void);

#ifdef __cplusplus
}
#endif
#define __LISTENER_H
#endif // __LISTENER_H
