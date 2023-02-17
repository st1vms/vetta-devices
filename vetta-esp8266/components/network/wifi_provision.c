#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <sys/socket.h>
#include "openssl/ssl.h"
#include "packets.h"
#include "dbits.h"
#include "wifi_provision.h"

extern const uint8_t ca_pem_start[] asm("_binary_ca_pem_start");
extern const uint8_t ca_pem_end[]   asm("_binary_ca_pem_end");
extern const uint8_t lamp_pem_start[] asm("_binary_lamp_pem_start");
extern const uint8_t lamp_pem_end[]   asm("_binary_lamp_pem_end");
extern const uint8_t lamp_key_start[] asm("_binary_lamp_key_start");
extern const uint8_t lamp_key_end[]   asm("_binary_lamp_key_end");

static int server_socket = -1;

static struct sockaddr_in serverAddr;

static unsigned char recvBuffer[PROVISION_SERVER_BUFFER_SIZE];


static SSL_CTX * ssl_ctx;
static SSL * ssl_session;

static SSL_CTX * init_ssl_context(){
    SSL_CTX* ctx;

    ctx = SSL_CTX_new(TLSv1_2_server_method());
    if (!ctx) {
        return NULL;
    }

    int ret = SSL_CTX_use_certificate_ASN1(ctx, lamp_pem_end - lamp_pem_start, lamp_pem_start);
    if (!ret) {
        SSL_CTX_free(ctx);
        return NULL;
    }

    ret = SSL_CTX_use_PrivateKey_ASN1(0, ctx, lamp_key_start, lamp_key_end - lamp_key_start);
    if (!ret) {
        SSL_CTX_free(ctx);
        return NULL;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    return ctx;
}

static esp_err_t init_provision_server(void)
{
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        return ESP_FAIL;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = 0;
    serverAddr.sin_port = htons(PROVISION_SERVER_PORT);

    int ret = bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (ret || listen(server_socket, 1)) {
        close(server_socket);
        server_socket = -1;
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t init_wifi_provision(void)
{
    ssl_ctx = init_ssl_context();
    if(!ssl_ctx){
        return ESP_FAIL;
    }

    if(ESP_OK != init_provision_server()){
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
        return ESP_FAIL;
    }

    return ESP_OK;
}

void deinit_wifi_provision(void)
{
    if(server_socket != -1){
        close(server_socket);
        server_socket = -1;
    }

    if(ssl_ctx != NULL){
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
    }
}

esp_err_t provision_listen(spiffs_string_t * ussid, spiffs_string_t * upwd)
{
    static esp_err_t _err;
    static socklen_t socklen;

    static struct timeval time_out_v;

    time_out_v.tv_sec = 0;
    time_out_v.tv_usec = 100000; // 100ms

    fd_set set;

    FD_ZERO(&set);
    FD_SET(server_socket, &set);

    // select
    int ret = select(server_socket + 1, &set, NULL, NULL, &time_out_v);
    if (ret == -1){
        // Select Error
        return ESP_FAIL;
    }
    else if (ret == 0){
        // Select timeout
        return ESP_ERR_TIMEOUT;
    }

    ssl_session = SSL_new(ssl_ctx);
    if (!ssl_session) {
        return ESP_FAIL;
    }

    // accept
    int clientSock = accept(server_socket, (struct sockaddr*)&serverAddr, &socklen);
    if (clientSock < 0) {
        SSL_free(ssl_session);
        ssl_session = NULL;
        return ESP_ERR_TIMEOUT;
    }

    SSL_set_fd(ssl_session, clientSock);

    ret = SSL_accept(ssl_session);
    if (!ret) {

        close(clientSock);
        clientSock = -1;

        SSL_free(ssl_session);
        ssl_session = NULL;
        return ESP_FAIL;
    }

    // read
    memset(recvBuffer, 0, PROVISION_SERVER_BUFFER_SIZE*sizeof(unsigned char));
    ret = SSL_read(ssl_session, recvBuffer, PROVISION_SERVER_BUFFER_SIZE - 1);
    if (ret > 0) {
        // NULL terminate buffer
        recvBuffer[ret] = 0;

        // Deserialize Packet
        dpacket_struct_t dpacket;
        if(DeserializeBuffer(recvBuffer, ret, &dpacket)){
            if(dpacket.packet_id == PROVISION_PACKET_ID &&
                dpacket.data_list.size == PROVISION_PACKET_SIZE &&
                dpacket.data_list.first_node != NULL && dpacket.data_list.first_node->next_node != NULL &&
                dpacket.data_list.first_node->stype == UTF8_STRING_STYPE &&
                dpacket.data_list.first_node->next_node->stype == UTF8_STRING_STYPE &&
                dpacket.data_list.first_node->data.utf8_str_v.length > 0 &&  // SSID
                dpacket.data_list.first_node->data.utf8_str_v.length <= MAX_SSID_LEN &&
                dpacket.data_list.first_node->next_node->data.utf8_str_v.length <= MAX_PASSWORD_LENGTH) // Password
            {
                // Copy SSID
                for(size_t i = 0; i < dpacket.data_list.first_node->data.utf8_str_v.length; i++){
                    ussid->string_array[i] = dpacket.data_list.first_node->data.utf8_str_v.utf8_string[i];
                }
                ussid->string_array[dpacket.data_list.first_node->data.utf8_str_v.length] = 0; // NULL terminate SSID string
                ussid->string_len = dpacket.data_list.first_node->data.utf8_str_v.length;

                if(dpacket.data_list.first_node->next_node->data.utf8_str_v.length > 0){
                    // Copy AP Password
                    for(size_t i = 0; i < dpacket.data_list.first_node->next_node->data.utf8_str_v.length; i++){
                        upwd->string_array[i] = dpacket.data_list.first_node->next_node->data.utf8_str_v.utf8_string[i];
                    }
                    upwd->string_array[dpacket.data_list.first_node->next_node->data.utf8_str_v.length] = 0; // NULL terminate SSID string
                    upwd->string_len = dpacket.data_list.first_node->next_node->data.utf8_str_v.length;
                }else{
                    upwd->string_len = 0;
                }

                // Set return value to success
                _err = ESP_OK;
            }

            // Free packet reference
            FreePacket(&dpacket);
        }else{_err = ESP_FAIL;} // Return error
    }else{_err = ESP_FAIL;} // Return error

    SSL_shutdown(ssl_session);

    close(clientSock);
    clientSock = -1;

    SSL_free(ssl_session);
    ssl_session = NULL;
    return _err;
}
