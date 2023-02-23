#ifndef __LISTENER_H


#include "esp_err.h"
#include "esp_event.h"
#include "tcpip_adapter.h"

#ifdef __cplusplus
extern "C"
{
#endif

esp_err_t init_listener_server(u32_t ip_info);

void close_listener_server(void);

esp_err_t listener_listen(void);

#ifdef __cplusplus
}
#endif
#define __LISTENER_H
#endif // __LISTENER_H
