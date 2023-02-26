#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "dbits.h"
#include "packets.h"
#include "discovery.h"

static in_addr_t ipAddress = 0;

static int server_socket = -1;

static unsigned char recvBuffer[DISCOVERY_SERVER_BUFFER_SIZE];
static unsigned char sendBuffer[DISCOVERY_SERVER_BUFFER_SIZE];

esp_err_t init_discovery_server(in_addr_t ip){

    ipAddress = ip;

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0)
    {
        return ESP_FAIL;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = ipAddress;
    serverAddr.sin_port = htons(DISCOVERY_SERVER_PORT);

    if(0 > bind(server_socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)))
    {
        close(server_socket);
        server_socket = -1;
        return ESP_FAIL;
    }

    return ESP_OK;
}

void close_discovery_server(void){
    if (server_socket != -1)
    {
        close(server_socket);
        server_socket = -1;
    }
}

static esp_err_t send_discovery_response(u32_t networkAddr, uint8_t current_lamp_state){

    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0)
    {
        return ESP_FAIL;
    }

    struct sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(destAddr));

    destAddr.sin_family = AF_INET;
    destAddr.sin_addr.s_addr = networkAddr;
    destAddr.sin_port = htons(BROKER_DISCOVERY_SERVER_PORT);

    dpacket_struct_t dpacket;
    if(!NewPacket(&dpacket, BROKER_DISCOVERY_ACK_PACKET_ID)){
        close(client_socket);
        return ESP_FAIL;
    }

    if(!AddSerializable(&dpacket, UINT32_STYPE, (data_union_t){.decimal_v.u32_v = ipAddress}) ||
        !AddSerializable(&dpacket, UINT8_STYPE, (data_union_t){.decimal_v.u8_v = 0}) ||
        !AddSerializable(&dpacket, UINT8_STYPE, (data_union_t){.decimal_v.u8_v = current_lamp_state}))
    {
        FreePacket(&dpacket);
        close(client_socket);
        return ESP_FAIL;
    }

    size_t packet_size = 0;
    memset(sendBuffer, 0, sizeof(unsigned char)*DISCOVERY_SERVER_BUFFER_SIZE);
    if(!SerializePacket(sendBuffer, DISCOVERY_SERVER_BUFFER_SIZE - 1, &dpacket, &packet_size) ||
        packet_size == 0 || packet_size >= DISCOVERY_SERVER_BUFFER_SIZE)
    {
        FreePacket(&dpacket);
        close(client_socket);
        return ESP_FAIL;
    }
    FreePacket(&dpacket);

    if(packet_size != sendto(client_socket, sendBuffer, packet_size, 0,
        (const struct sockaddr*)&destAddr, (socklen_t) sizeof(destAddr)))
    {
        close(client_socket);
        return ESP_FAIL;
    }
    close(client_socket);

    return ESP_OK;
}


esp_err_t discovery_listen(uint8_t current_lamp_state){

    static struct sockaddr_in clientAddr;
    static socklen_t clientAddrLen;
    clientAddrLen = sizeof(clientAddr);

    static struct timeval time_out_v;
    time_out_v.tv_sec = 0;
    time_out_v.tv_usec = 500000; // 500ms

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

    // Socket selected

    memset(recvBuffer, 0, sizeof(unsigned char) * DISCOVERY_SERVER_BUFFER_SIZE);
    int len = recvfrom(server_socket, recvBuffer, DISCOVERY_SERVER_BUFFER_SIZE - 1, 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (len < 0){
        // Error occured during receiving
        return ESP_FAIL;
    }
    else if (len == 0){
        // Read timeout
        return ESP_ERR_TIMEOUT;
    }
    else
    {
        // Data received
        recvBuffer[len] = 0; // NULL terminate buffer

        dpacket_struct_t dpacket;
        if (DeserializeBuffer(recvBuffer, len, &dpacket))
        {
            serializable_list_node_t * node = dpacket.data_list.first_node;
            if (dpacket.packet_id == BROKER_DISCOVERY_REQUEST_PACKET_ID &&
                dpacket.data_list.size == BROKER_DISCOVERY_REQUEST_PACKET_SIZE &&
                node != NULL &&
                node->stype == UINT32_STYPE &&
                node->data.decimal_v.u32_v > 0)
            {
                // Validate extra fields, without touching them.
                if(node->next_node != NULL && node->next_node->stype == UINT32_STYPE &&
                    node->next_node->data.decimal_v.u32_v > 0)
                {
                    // Free packet reference
                    FreePacket(&dpacket);

                    // Send discovery response
                    return send_discovery_response(node->data.decimal_v.u32_v, current_lamp_state);
                }
            }
            // Free packet reference
            FreePacket(&dpacket);
        }
    }

    return ESP_ERR_TIMEOUT;
}
