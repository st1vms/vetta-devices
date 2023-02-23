#include "dbits.h"
#include "packets.h"

static int brokerDiscoveryRequestPacketFormat[BROKER_DISCOVERY_REQUEST_PACKET_SIZE] = {
    UINT32_STYPE,   // Network Address Integer
    UINT32_STYPE    // App identifier
};


static int brokerDiscoveryAckPacketFormat[BROKER_DISCOVERY_ACK_PACKET_SIZE] = {
    UINT32_STYPE,   // Network Address Integer
    UINT8_STYPE,    // Lamp model
    UINT8_STYPE     // Lamp state
};

static int provisionPacketFormat[PROVISION_PACKET_SIZE] = {
    UTF8_STRING_STYPE, // SSID
    UTF8_STRING_STYPE  // AP Password
};

unsigned char RegisterNetworkPackets()
{
    return RegisterPacket(BROKER_DISCOVERY_REQUEST_PACKET_ID, brokerDiscoveryRequestPacketFormat, BROKER_DISCOVERY_REQUEST_PACKET_SIZE) &&
        RegisterPacket(BROKER_DISCOVERY_ACK_PACKET_ID, brokerDiscoveryAckPacketFormat, BROKER_DISCOVERY_ACK_PACKET_SIZE) &&
        RegisterPacket(PROVISION_PACKET_ID, provisionPacketFormat, PROVISION_PACKET_SIZE);
}
