#ifndef __PACKETS_H

#ifdef __cplusplus
extern "C"
{
#endif

#define PING_PACKET_ID 0
#define PING_PACKET_SIZE 1

#define BROKER_DISCOVERY_REQUEST_FLAG_BROKER 1

#define BROKER_DISCOVERY_REQUEST_PACKET_ID 1
#define BROKER_DISCOVERY_REQUEST_PACKET_SIZE 2

#define BROKER_DISCOVERY_ACK_PACKET_ID 2
#define BROKER_DISCOVERY_ACK_PACKET_SIZE 3

#define PROVISION_PACKET_ID 3
#define PROVISION_PACKET_SIZE 2

#define LAMP_STATE_CHANGE_PACKET_ID 4
#define LAMP_STATE_CHANGE_PACKET_SIZE 1

    unsigned char RegisterNetworkPackets();

#ifdef __cplusplus
}
#endif

#define __PACKETS_H
#endif // __PACKETS_H
