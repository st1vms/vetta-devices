#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "lwip/ip_addr.h"
#include "dbits.h"
#include "packets.h"
#include "listener.h"

static u32_t ipAddress;
esp_err_t init_listener_server(u32_t ip){
    ipAddress = ip;
    return ESP_OK;
}

void close_listener_server(void)
{

}

esp_err_t listener_listen(void){

    return ESP_OK;
}
