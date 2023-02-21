#ifndef __DPACKET_H

// Max number of packets
#define PACKET_TABLE_SIZE 3

// Max number of fields x packet
#define MAX_PACKET_FIELDS 5

#include "dtypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct serializable_list_node_t
    {
        struct serializable_list_node_t *next_node;
        serializable_type_t stype;
        data_union_t data;

    } serializable_list_node_t;

    typedef struct serializable_list_t
    {
        unsigned long int size;
        serializable_list_node_t *first_node;
    } serializable_list_t;

    typedef struct dpacket_struct_t
    {
        packet_id_t packet_id;
        serializable_list_t data_list;
    } dpacket_struct_t;

    typedef dpacket_struct_t *dpacket_t;

    /**
     * @brief Initialize a packet reference,
     *  the output reference must be freed using FreePacket() when it's no more needed.
     * @param packet_p Output packet structure pointer
     * @param packet_id Packet ID
     * @return 1 in case a reference was stored, 0 on errors.
     */
    extern char NewPacket(dpacket_t packet_p, packet_id_t packet_id);

    /**
     * @brief Register a packet along with fields and ID,
     *  this function is needed for DeserializeBuffer()
     *  in order to look for the corresponding packet id when deserializing a packet.
     * @param packet_id Packet ID, unique for this packet, must not be greater than PACKET_TABLE_SIZE.
     * @param packet_format Format int array, sequentially storing the packet's fields types.
     * @param format_size Format array size, must not be greater than MAX_PACKET_FIELDS.
     * @return 1 on success, 0 in case of errors.
     */
    extern char RegisterPacket(packet_id_t packet_id, int *packet_format, size_t format_size);

    /**
     * @brief Get the packet format array from a packet_id,
     *  storing the array size in `out_size`
     *
     * @param packet_id The packet id to look for in the packet table.
     * @param out_size Output format array size.
     * @return int array pointer on success.
     * In case of errors, NULL is returned and `out_size` will get the value of 0
     */
    int *GetPacketFormat(packet_id_t packet_id, size_t *out_size);

    /**
     * @brief This function will attempt to free the serializable list stored in
     * this packet reference.
     * After calling this function, the new packet will contain 0 serializables,
     * but it can still be adoperated.
     *
     * @param list_p Packet structure pointer
     */
    extern void FreePacket(dpacket_t list_p);

    /**
     * @brief Add a Serializable to the end of the packet serializable list
     *
     * @param dpacket_p Packet structure pointer
     * @param stype Serializable type enum
     * @param datav Data variable union
     * @return 1 on success, 0 in case of errors
     */
    extern char AddSerializable(dpacket_t dpacket_p, serializable_type_t stype, data_union_t datav);

    /**
     * @brief Add a Serializable to the end of the packet serializable list
     *
     * @param dpacket_p Packet structure pointer
     * @param string_v string pointer
     * @param string_len string length (excluding last 0, if present)
     * @return 1 on success, 0 in case of errors
     */
    extern char AddUTF8StringSerializable(dpacket_t dpacket_p, const unsigned char *string_v, size_t string_len);

#ifdef __cplusplus
}
#endif
#define __DPACKET_H
#endif // __DPACKET_H
