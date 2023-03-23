#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "dbits.h"
#include "packets.h"
#include "wifi_provision.h"

static int server_socket = -1;

static unsigned char recvBuffer[PROVISION_SERVER_BUFFER_SIZE];

esp_err_t init_wifi_provision(void)
{
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0)
    {
        return ESP_FAIL;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(((u32_t)0x00000000UL));
    serverAddr.sin_port = htons(PROVISION_SERVER_PORT);

    int ret = bind(server_socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
    {
        close(server_socket);
        server_socket = -1;
        return ESP_FAIL;
    }

    return ESP_OK;
}

void deinit_wifi_provision(void)
{
    if (server_socket != -1)
    {
        close(server_socket);
        server_socket = -1;
    }
}

esp_err_t provision_listen(spiffs_string_t *ussid, spiffs_string_t *upwd, uint32_t * pinCode)
{
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
    if (ret == -1)
    {
        // Select Error
        return ESP_FAIL;
    }
    else if (ret == 0)
    {
        // Select timeout
        return ESP_ERR_TIMEOUT;
    }

    memset(recvBuffer, 0, sizeof(unsigned char) * PROVISION_SERVER_BUFFER_SIZE);
    int len = recvfrom(server_socket, recvBuffer, PROVISION_SERVER_BUFFER_SIZE - 1, 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (len < 0)
    {
        // Error occured during receiving
        return ESP_FAIL;
    }
    else if (len == 0)
    {
        // Read timeout
        return ESP_ERR_TIMEOUT;
    }
    else
    {

        // Data received
        recvBuffer[len] = 0; // NULL terminate buffer

        // Deserialize Packet
        dpacket_struct_t dpacket;
        if (DeserializeBuffer(recvBuffer, len, &dpacket))
        {
            if (dpacket.packet_id == PROVISION_PACKET_ID &&
                dpacket.data_list.size == PROVISION_PACKET_SIZE &&
                dpacket.data_list.first_node != NULL && dpacket.data_list.first_node->next_node != NULL &&
                dpacket.data_list.first_node->stype == UTF8_STRING_STYPE &&
                dpacket.data_list.first_node->next_node->stype == UTF8_STRING_STYPE &&
                dpacket.data_list.first_node->data.utf8_str_v.length > 0 && // SSID
                dpacket.data_list.first_node->data.utf8_str_v.length <= MAX_SSID_LENGTH &&
                dpacket.data_list.first_node->next_node->data.utf8_str_v.length <= MAX_PASSWORD_LENGTH) // Password
            {
                // Copy SSID
                for (size_t i = 0; i < dpacket.data_list.first_node->data.utf8_str_v.length; i++)
                {
                    ussid->string_array[i] = dpacket.data_list.first_node->data.utf8_str_v.utf8_string[i];
                }
                ussid->string_array[dpacket.data_list.first_node->data.utf8_str_v.length] = 0; // NULL terminate SSID string
                ussid->string_len = dpacket.data_list.first_node->data.utf8_str_v.length;

                if (dpacket.data_list.first_node->next_node->data.utf8_str_v.length > 0)
                {
                    // Copy AP Password
                    for (size_t i = 0; i < dpacket.data_list.first_node->next_node->data.utf8_str_v.length; i++)
                    {
                        upwd->string_array[i] = dpacket.data_list.first_node->next_node->data.utf8_str_v.utf8_string[i];
                    }
                    upwd->string_array[dpacket.data_list.first_node->next_node->data.utf8_str_v.length] = 0; // NULL terminate SSID string
                    upwd->string_len = dpacket.data_list.first_node->next_node->data.utf8_str_v.length;
                }
                else
                {
                    upwd->string_len = 0;
                }

                // Get Pin Code ( 6 digits )
                serializable_list_node_t *n = dpacket.data_list.first_node->next_node->next_node;
                if(n != NULL && n->stype == UINT32_STYPE && n->data.decimal_v.u32_v > 99999){
                    *pinCode = n->data.decimal_v.u32_v;
                    // Free packet reference
                    FreePacket(&dpacket);

                    // Return success
                    return ESP_OK;
                }
            }

            // Free packet reference
            FreePacket(&dpacket);
        }
    }

    return ESP_FAIL;
}
