#ifndef __DBITS_H

#include "dtypes.h"
#include "dpacket.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Serialize a Packet structure into a byte buffer,
     * using no more than `buffer_size` bytes,
     * storing the output size into `out_size`
     *
     * @param buffer Output byte buffer
     * @param buffer_size Output byte buffer size
     * @param packet Packet structure to serialize
     * @param out_size Output size pointer, filled with serialized buffer size
     * @return 1 on success, 0 in case of errors.
     */
    extern char SerializePacket(unsigned char *buffer, size_t buffer_size, dpacket_t packet, size_t *out_size);

    /**
     * @brief Deserialize a byte buffer into a packet structure reference,
     * reading no more than `buffer_size` bytes.
     *
     * @param buffer Byte buffer to deserialize from
     * @param buffer_size Byte buffer size
     * @param packet_out Output packet structure pointer
     * @return 1 on success, 0 in case of errors.
     */
    extern char DeserializeBuffer(unsigned char *buffer, const size_t buffer_size, dpacket_t packet_out);

#ifdef __cplusplus
}
#endif
#define __DBITS_H
#endif // __DBITS_H
