#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "dbits.h"
#include "wifi_provision.h"

static int server_socket;
static socklen_t socklen;

static struct sockaddr_in serverAddr;
static struct sockaddr_in broadcastAddr;

static unsigned char recvBuffer[PROVISION_SERVER_BUFFER_SIZE];

static struct timeval time_out_v = {
    .tv_sec = 0,
    .tv_usec = 100000 // 100ms
};

esp_err_t init_wifi_provision(void)
{
    memset(&serverAddr, 0, sizeof(struct sockaddr_in));

    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PROVISION_SERVER_PORT);

    memset(&broadcastAddr, 0, sizeof(struct sockaddr_in));
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(PROVISION_CLIENT_PORT);

    server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (server_socket < 0)
    {
        return ESP_FAIL;
    }

    if (0 > bind(server_socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) ||
        (0 > setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &time_out_v, sizeof(time_out_v))) ||
        (0 > setsockopt(server_socket, SOL_SOCKET, SO_SNDTIMEO, &time_out_v, sizeof(time_out_v))))
    {
        deinit_wifi_provision();
        return ESP_FAIL;
    }

    socklen = sizeof(server_socket);

    return ESP_OK;
}

void deinit_wifi_provision(void)
{
    if (server_socket != -1)
    {
        shutdown(server_socket, 0);
        close(server_socket);
    }
}

static esp_err_t send_provision_packet_hello()
{

    ByteBuffer *byteBuffer = NewByteBuffer();
    if (!byteBuffer)
    {
        return ESP_FAIL;
    }

    if (SerializeDataF(byteBuffer, "b", 0) && byteBuffer->buffer_p != NULL && byteBuffer->buffer_size > 0)
    {
        // Send byte buffer
        int len = sendto(server_socket, byteBuffer->buffer_p, byteBuffer->buffer_size, 0, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr));
        if (len < 0)
        {
            // Send timed-out or failed
            // Free byte buffer
            FreeByteBuffer(byteBuffer);
            return (errno == EAGAIN) ? ESP_ERR_TIMEOUT : ESP_FAIL;
        }
        else if (len > 0)
        {
            // Send succeeded
            // Free byte buffer
            FreeByteBuffer(byteBuffer);
            return ESP_OK;
        }
    }

    // Free byte buffer
    FreeByteBuffer(byteBuffer);
    return ESP_FAIL;
}

static esp_err_t parse_provision_packet_creds(unsigned char *buffer, size_t buflen, spiffs_string_t * ussid, spiffs_string_t * upwd)
{
    ByteBuffer *byteBuffer = NewByteBuffer();
    if (!byteBuffer)
    {
        return ESP_FAIL;
    }

    SerializableDeque *sd = AllocateSerializableDeque();
    if (!sd)
    {
        // Free byte buffer
        FreeByteBuffer(byteBuffer);
        return ESP_FAIL;
    }
    const int template[2] = {UTF8_STRING_DTYPE, UTF8_STRING_DTYPE};
    if (DeserializeBufferToDataDeque(buffer, buflen, template, 2, sd))
    {
        // Packet deserialized

        FreeByteBuffer(byteBuffer); // Free byte buffer
        if (sd->size == 2)
        {
            // Serializable deque size match template
            SerializableDequeNode *node = sd->head_node;
            if (node->value.data_type == UTF8_STRING_DTYPE &&
                node->value.unicode_str_data.length > 0 &&
                node->value.unicode_str_data.length <= MAX_SSID_LENGTH &&
                node->value.unicode_str_data.u8string != NULL)
            {
                // Parse AP SSID
                memcpy(ussid->string_array, node->value.unicode_str_data.u8string, node->value.unicode_str_data.length);
                ussid->string_len = node->value.unicode_str_data.length;

                node = node->next_node;
                if (node->value.data_type == UTF8_STRING_DTYPE &&
                    node->value.unicode_str_data.length > 0 &&
                    node->value.unicode_str_data.length <= MAX_PASSWORD_LENGTH &&
                    node->value.unicode_str_data.u8string != NULL)
                {
                    // Parse AP Password
                    memcpy(upwd->string_array, node->value.unicode_str_data.u8string, node->value.unicode_str_data.length);
                    upwd->string_len = node->value.unicode_str_data.length;

                    // Free allocated resources
                    FreeSerializableDeque(sd, 1);
                    return ESP_OK;
                }
            }
        }
    }

    // Free allocated resources
    FreeSerializableDeque(sd, 1);
    FreeByteBuffer(byteBuffer);
    return ESP_FAIL;
}

esp_err_t provision_listen(spiffs_string_t * ussid, spiffs_string_t * upwd)
{
    static struct sockaddr_in sourceAddr;
    static socklen_t sourceAddr_len;
    static esp_err_t _err;

    sourceAddr_len = sizeof(sourceAddr);

    // Load sendBuffer data
    if (ESP_OK == (_err = send_provision_packet_hello()))
    {

        // Send succeeded
        memset(recvBuffer, 0, PROVISION_SERVER_BUFFER_SIZE);

        // Try receiving provision packet
        int len = recvfrom(server_socket, recvBuffer, PROVISION_SERVER_BUFFER_SIZE - 1, 0, (struct sockaddr *)&sourceAddr, &sourceAddr_len);
        if (len < 0)
        {
            // Receive timed-out or failed
            return (errno == EAGAIN) ? ESP_ERR_TIMEOUT : ESP_FAIL;
        }
        else if (len > 0)
        {
            // Received succeeded
            recvBuffer[PROVISION_SERVER_BUFFER_SIZE] = 0; // Null-terminate buffer

            // Parse provision buffer
            return parse_provision_packet_creds(recvBuffer, len, ussid, upwd);
        }
    }
    return _err;
}
