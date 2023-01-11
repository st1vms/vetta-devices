#ifndef _DBITS_BITCONV_H
#ifdef __cplusplus
extern "C" {
#endif

#include "dbits/dtypes.h"
#include "dbits/deque.h"

char AddBit(Deque * bits, enum DequeInsertModeEnum imode, deque_size_t count, unsigned char bitv);

char SizeHeaderToBits(UInt8 value, enum IntegerHeaderSize bitsize, Deque *out);
char IntToBits(Int64 value, enum SignedIntegerSize bitsize, Deque *out, unsigned char has_header);
char UIntToBits(UInt64 value, enum UnsignedIntegerSize bitsize, Deque *out, unsigned char has_header);

char UTF8StringToBits(char **utf8_string, size_t slength, Deque *out);

char DoubleToBits(Double value, Deque *out);
char BooleanToBit(Boolean value, Deque *out);

deque_size_t BitsToSizeHeader(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, enum IntegerHeaderSize header_bitsize, UInt8 *out);

deque_size_t BitsToInt8(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, Int8 *out, unsigned char has_header);
deque_size_t BitsToInt16(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, Int16 *out, unsigned char has_header);
deque_size_t BitsToInt32(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, Int32 *out, unsigned char has_header);
deque_size_t BitsToInt64(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, Int64 *out, unsigned char has_header);
deque_size_t BitsToUInt8(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, UInt8 *out, unsigned char has_header);
deque_size_t BitsToUInt16(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, UInt16 *out, unsigned char has_header);
deque_size_t BitsToUInt32(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, UInt32 *out, unsigned char has_header);
deque_size_t BitsToUInt64(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, UInt64 *out, unsigned char has_header);

deque_size_t BitsToUTF8String(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, UnicodeStringBucket *out);

deque_size_t BitsToDouble(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, Double *out);
deque_size_t BitToBoolean(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, Boolean *out);

size_t BitsToUCharArray(Deque *bits, unsigned char **out_buffer);
char UCharArrayToBits(unsigned char *buffer, size_t buffer_size, Deque *out_bits);

#ifdef __cplusplus
}
#endif
#define _DBITS_BITCONV_H
#endif //_DBITS_BITCONV_H
