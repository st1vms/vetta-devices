#ifndef __WIFI_PROVISION_H

#include "esp_err.h"
#include "storage.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define PROVISION_SERVER_IP "192.168.4.1"
#define PROVISION_SERVER_PORT 50030
#define PROVISION_SERVER_BUFFER_SIZE (128UL)

    esp_err_t init_wifi_provision(void);
    void deinit_wifi_provision(void);
    esp_err_t provision_listen(spiffs_string_t *ussid, spiffs_string_t *upwd, uint32_t * pinCode);

#ifdef __cplusplus
}
#endif
#define __WIFI_PROVISION_H
#endif // __WIFI_PROVISION_H
