#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "lwip/ip_addr.h"
#include "openssl/ssl.h"
#include "dbits.h"
#include "packets.h"
#include "listener.h"

#define OPENSSL_SERVER_FRAGMENT_SIZE 2048

extern const uint8_t ca_pem_start[] asm("_binary_ca_pem_start");
extern const uint8_t ca_pem_end[]   asm("_binary_ca_pem_end");
extern const uint8_t lamp_pem_start[] asm("_binary_lamp_pem_start");
extern const uint8_t lamp_pem_end[]   asm("_binary_lamp_pem_end");
extern const uint8_t lamp_key_start[] asm("_binary_lamp_key_start");
extern const uint8_t lamp_key_end[]   asm("_binary_lamp_key_end");

static u32_t ipAddress;

static int server_socket = -1;

static int client_socket = -1;
static struct sockaddr_in clientAddr;
static unsigned int clientAddrLen = sizeof(clientAddr);

static unsigned char recvBuffer[LISTENER_SERVER_BUFFER_SIZE];
static unsigned char sendBuffer[LISTENER_SERVER_BUFFER_SIZE];

static SSL * ssl_session;
static SSL_CTX * ssl_ctx;

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


esp_err_t init_listener_server(u32_t ip){

    ipAddress = ip;

    ssl_ctx = init_ssl_context();
    if(!ssl_ctx){
        return ESP_FAIL;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
        return ESP_FAIL;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = 0;
    serverAddr.sin_port = htons(LISTENER_SERVER_PORT);

    int ret = bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (ret < 0 || listen(server_socket, 1) < 0) {
        close(server_socket);
        server_socket = -1;
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
        return ESP_FAIL;
    }

    return ESP_OK;
}

void close_client_socket(void){
    printf("\nCLOSING CLIENT SOCKET\n");
    if(ssl_session != NULL){
        SSL_shutdown(ssl_session);
        SSL_free(ssl_session);
        ssl_session = NULL;
    }

    if(client_socket != -1){
        close(client_socket);
        client_socket = -1;
    }
}


void close_listener_server(void)
{
    printf("\nCLOSING SERVER SOCKET\n");
    close_client_socket();

    if(server_socket != -1){
        close(server_socket);
        server_socket = -1;
    }

    if(ssl_ctx != NULL){
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
    }
}

esp_err_t send_ping(){

    if(client_socket == -1 || ssl_session == NULL){
        return ESP_FAIL;
    }

    dpacket_struct_t dpacket;
    if(!NewPacket(&dpacket, PING_PACKET_ID)){
        return ESP_FAIL;
    }

    if(!AddSerializable(&dpacket, UINT8_STYPE, (data_union_t){.decimal_v.u8_v = 0}))
    {
        FreePacket(&dpacket);
        return ESP_FAIL;
    }

    size_t packet_size = 0;
    memset(sendBuffer, 0, sizeof(unsigned char)*LISTENER_SERVER_BUFFER_SIZE);
    if(!SerializePacket(sendBuffer, LISTENER_SERVER_BUFFER_SIZE-1, &dpacket, &packet_size) ||
        packet_size == 0 || packet_size >= LISTENER_SERVER_BUFFER_SIZE)
    {
        FreePacket(&dpacket);
        return ESP_FAIL;
    }
    FreePacket(&dpacket);

    if(packet_size != SSL_write(ssl_session, sendBuffer, packet_size)){
        return ESP_FAIL;
    }

    return ESP_OK;
}

listener_event_t listener_listen(void){

    static struct timeval time_out_v;
    time_out_v.tv_sec = 0;
    time_out_v.tv_usec = 500000; // 500ms

    fd_set server_set, client_set;

    FD_ZERO(&server_set);

    FD_SET(server_socket, &server_set);

    int ret = 0;
    if(client_socket == -1){
        // select
        ret = select(server_socket + 1, &server_set, NULL, NULL, &time_out_v);
        if (ret == -1){
            // Select Error
            return RESULT_FAIL;
        }
        else if (ret == 0){
            // Select timeout
            return RESULT_NO_ACTION;
        }

        // accept
        client_socket = accept(server_socket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (client_socket < 0) {
            return RESULT_NO_ACTION;
        }

        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out_v, sizeof(time_out_v));

        // TODO Speed up handshake
        printf("\nSocket Accepted\n");

        ssl_session = SSL_new(ssl_ctx);
        if (!ssl_session) {
            close(client_socket);
            client_socket = -1;
            printf("\nERROR SSL_new()\n");
            return RESULT_NO_ACTION;
        }

        SSL_set_fd(ssl_session, client_socket);

        ret = SSL_accept(ssl_session);
        if (!ret) {

            close(client_socket);
            client_socket = -1;

            SSL_free(ssl_session);
            ssl_session = NULL;
            printf("\nERROR SSL_accept()\n");
            return RESULT_NO_ACTION;
        }
        printf("\nSSL SESSION CREATED\n");
    }

    FD_ZERO(&client_set);
    FD_SET(client_socket, &client_set);

    // select
    ret = select(client_socket + 1, &client_set, NULL, NULL, &time_out_v);
    if (ret == -1){
        // Select Error
        return RESULT_FAIL;
    }
    else if (ret == 0){
        // Select timeout

        // Send Ping
        if(ESP_OK != send_ping()){
            close_client_socket();
        }

        return RESULT_CLIENT_STALE;
    }

    printf("\nCLIENT SELECTED\n");

    memset(recvBuffer, 0, LISTENER_SERVER_BUFFER_SIZE*sizeof(unsigned char));
    ret = SSL_read(ssl_session, recvBuffer, LISTENER_SERVER_BUFFER_SIZE - 1);
    if (ret > 0) {

        recvBuffer[ret] = 0; // NULL Terminate buffer

        printf("\nREAD %d BYTES\n", ret);

        dpacket_struct_t dpacket;
        if(!DeserializeBuffer(recvBuffer, ret, &dpacket)){
            close_client_socket();
            return RESULT_NO_ACTION;
        }

        if(dpacket.packet_id != LAMP_STATE_CHANGE_PACKET_ID ||
            dpacket.data_list.size != LAMP_STATE_CHANGE_PACKET_SIZE ||
            dpacket.data_list.first_node == NULL ||
            dpacket.data_list.first_node->stype != UINT8_STYPE)
        {
            FreePacket(&dpacket);
            close_client_socket();
            return RESULT_NO_ACTION;
        }

        uint8_t state = dpacket.data_list.first_node->data.decimal_v.u8_v;
        FreePacket(&dpacket);

        switch (state)
        {
        case 0:
            return RESULT_LED_OFF;
        case 1:
            return RESULT_LED_LOW;
        case 2:
            return RESULT_LED_MEDIUM;
        case 3:
            return RESULT_LED_HIGH;
        case 4:
            return RESULT_LED_NEXT;
        default:
            close_client_socket();
            return RESULT_NO_ACTION;
        }

    }else if(ret < 0){
        close_client_socket();
        return RESULT_NO_ACTION;
    }

    printf("\nRECV TIMEOUT\n");
    return RESULT_CLIENT_STALE;
}
