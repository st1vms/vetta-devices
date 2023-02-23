#ifndef __PACKETS_H

#ifdef __cplusplus
extern "C"
{
#endif

#define BROKER_DISCOVERY_REQUEST_FLAG_BROKER 1

#define BROKER_DISCOVERY_REQUEST_PACKET_ID 1
#define BROKER_DISCOVERY_REQUEST_PACKET_SIZE 2

#define BROKER_DISCOVERY_ACK_PACKET_ID 2
#define BROKER_DISCOVERY_ACK_PACKET_SIZE 3

#define PROVISION_PACKET_ID 3
#define PROVISION_PACKET_SIZE 2

    unsigned char RegisterNetworkPackets();

#ifdef __cplusplus
}
#endif

#define __PACKETS_H
#endif // __PACKETS_H
