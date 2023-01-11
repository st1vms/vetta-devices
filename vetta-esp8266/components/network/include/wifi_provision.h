#ifndef __WIFI_PROVISION_H

#include "esp_err.h"
#include "storage.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define PROVISION_CLIENT_PORT   (50041)
#define PROVISION_SERVER_PORT   (50042)
#define PROVISION_SERVER_BUFFER_SIZE (128UL)

esp_err_t init_wifi_provision(void);
void deinit_wifi_provision(void);
esp_err_t provision_listen(spiffs_string_t * ussid, spiffs_string_t * upwd);

#ifdef __cplusplus
}
#endif
#define __WIFI_PROVISION_H
#endif // __WIFI_PROVISION_H
