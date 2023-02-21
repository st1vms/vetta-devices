#include "dbits.h"
#include "packets.h"

static int provisionPacketFormat[PROVISION_PACKET_SIZE] = {
    UTF8_STRING_STYPE, // SSID
    UTF8_STRING_STYPE  // AP Password
};

unsigned char RegisterNetworkPackets()
{
    return RegisterPacket(PROVISION_PACKET_ID, provisionPacketFormat, PROVISION_PACKET_SIZE);
}
