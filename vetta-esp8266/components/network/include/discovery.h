#ifndef __DISCOVERY_H

#include "esp_err.h"
#include "esp_event.h"
#include "sys/socket.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define DISCOVERY_SERVER_SELECT_TIMEOUT (500)
#define LAMP_MODEL_INTEGER 0
#define DISCOVERY_SERVER_PORT 50005
#define BROKER_DISCOVERY_SERVER_PORT 50000
#define DISCOVERY_SERVER_BUFFER_SIZE (128UL)

    esp_err_t init_discovery_server(in_addr_t ip, uint32_t lamp_seed);

    void close_discovery_server(void);

    esp_err_t discovery_listen(uint8_t current_lamp_state, uint8_t is_managed, uint32_t pinCode);

#ifdef __cplusplus
}
#endif
#define __DISCOVERY_H
#endif // __DISCOVERY_H
