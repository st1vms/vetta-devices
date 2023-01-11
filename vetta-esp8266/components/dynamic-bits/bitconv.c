
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "dbits/bitconv.h"

char AddBit(Deque * bits, enum DequeInsertModeEnum imode, deque_size_t count, unsigned char bitv)
{
    if(bitv != BIT_ONE && bitv != BIT_ZERO)
    {return 0;}

    return FillDeque(bits, imode, count, (DequeNodeValueType) { .bit_v = bitv}, DEQUE_BIT_VALUE);
}

// SERIALIZATION FUNCTIONS START

char SizeHeaderToBits(UInt8 value, enum IntegerHeaderSize bitsize, Deque *out)
{
    if (!IsValidDeque(out) || value == 0 || !CheckHeaderValue(value, bitsize))
    {
        return 0;
    }

    value -= 1;

    Deque *bits = AllocateDeque();

    UInt8 i = 0;
    while (value != 0 && i < bitsize)
    {
        if(value & 1)
        {
            if(!AddBit(bits, DEQUE_INSERT_TAIL, 1, BIT_ONE))
            {
                FreeDeque(bits);
                return 0;
            }
        }else if(!AddBit(bits, DEQUE_INSERT_TAIL, 1, BIT_ZERO))
        {
            FreeDeque(bits);
            return 0;
        }

        value >>= 1;
        i++;
    }

    if ((bitsize - i > 0 && !AddBit(bits, DEQUE_INSERT_TAIL, bitsize - i, BIT_ZERO)) || !MergeDeques(out, bits))
    {
        FreeDeque(bits);
        return 0;
    }

    FreeDeque(bits);
    return 1;
}

char IntToBits(Int64 value, enum SignedIntegerSize bitsize, Deque *out, unsigned char has_header)
{
    enum IntegerHeaderSize hsize;
    if (!IsValidDeque(out) || !GetIntHeaderSize(value, bitsize, &hsize) || !hsize)
    {
        return 0;
    }

    Deque *header_bits = AllocateDeque(), *tmp = AllocateDeque();

    if(value < 0)
    {
        if(!AddBit(tmp, DEQUE_INSERT_TAIL, 1, SIGN_NEGATIVE))
        {
            FreeDeque(tmp);
            FreeDeque(header_bits);
            return 0;
        }
    }else if(!AddBit(tmp, DEQUE_INSERT_TAIL, 1, SIGN_POSITIVE))
    {
        FreeDeque(tmp);
        FreeDeque(header_bits);
        return 0;
    }

    value = llabs(value);
    deque_size_t i = 0;

    if (value == 0)
    {
        if (!AddBit(tmp, DEQUE_INSERT_TAIL, 1, BIT_ZERO))
        {
            FreeDeque(tmp);
            FreeDeque(header_bits);
            return 0;
        }
        i++;
    }
    else
    {
        while (value != 0 && i < bitsize)
        {
            if(value & 1)
            {
                if(!AddBit(tmp, DEQUE_INSERT_TAIL, 1, BIT_ONE))
                {
                    FreeDeque(tmp);
                    FreeDeque(header_bits);
                    return 0;
                }
            }else if(!AddBit(tmp, DEQUE_INSERT_TAIL, 1, BIT_ZERO))
            {
                FreeDeque(tmp);
                FreeDeque(header_bits);
                return 0;
            }

            value >>= 1;
            i++;
        }
    }

    if (has_header)
    {
        if (!SizeHeaderToBits(tmp->size - 1, hsize, header_bits) || !MergeDeques(out, header_bits))
        {
            FreeDeque(tmp);
            FreeDeque(header_bits);
            return 0;
        }
    }
    else
    {
        if (bitsize - i > 0 && !AddBit(tmp, DEQUE_INSERT_TAIL, bitsize - i, BIT_ZERO))
        {
            FreeDeque(tmp);
            FreeDeque(header_bits);
            return 0;
        }
    }

    FreeDeque(header_bits);

    if (!MergeDeques(out, tmp))
    {
        FreeDeque(tmp);
        return 0;
    }

    FreeDeque(tmp);

    return 1;
}

char UIntToBits(UInt64 value, enum UnsignedIntegerSize bitsize, Deque *out, unsigned char has_header)
{
    enum IntegerHeaderSize hsize;
    if (!IsValidDeque(out) || !GetUIntHeaderSize(value, bitsize, &hsize) || !hsize)
    {
        return 0;
    }

    Deque *header_bits = AllocateDeque(), *tmp = AllocateDeque();
    deque_size_t i = 0;
    if (value == 0)
    {
        if (!AddBit(tmp, DEQUE_INSERT_TAIL, 1, BIT_ZERO))
        {
            FreeDeque(tmp);
            FreeDeque(header_bits);
            return 0;
        }
        i++;
    }
    else
    {
        while (value != 0 && i < bitsize)
        {
            if(value & 1)
            {
                if(!AddBit(tmp, DEQUE_INSERT_TAIL, 1, BIT_ONE))
                {
                    FreeDeque(tmp);
                    FreeDeque(header_bits);
                    return 0;
                }
            }else if(!AddBit(tmp, DEQUE_INSERT_TAIL, 1, BIT_ZERO))
            {
                FreeDeque(tmp);
                FreeDeque(header_bits);
                return 0;
            }
            value >>= 1;
            i++;
        }
    }

    if (has_header)
    {
        if (!SizeHeaderToBits(tmp->size, hsize, header_bits) || !MergeDeques(out, header_bits))
        {
            FreeDeque(tmp);
            FreeDeque(header_bits);
            return 0;
        }
    }
    else
    {
        if (bitsize - i > 0 && !AddBit(tmp, DEQUE_INSERT_TAIL, bitsize - i, BIT_ZERO))
        {
            FreeDeque(tmp);
            FreeDeque(header_bits);
            return 0;
        }
    }

    FreeDeque(header_bits);

    if (!MergeDeques(out, tmp))
    {
        FreeDeque(tmp);
        return 0;
    }

    FreeDeque(tmp);
    return 1;
}

char UTF8StringToBits(char ** utf8_string, size_t slength, Deque *out)
{
    if (!IsValidDeque(out) || utf8_string == NULL || NULL == *(utf8_string))
    {
        return 0;
    }

    Deque *bits = AllocateDeque();
    if(!bits)
    {
        return 0;
    }

    UInt16 n_bytes = 0;
    const char * tmp = *(utf8_string);

    while (n_bytes < slength && tmp != NULL && *tmp != '\0')
    {
        if(!IntToBits(*tmp, INT8_SIZE, bits, 0))
        {
            FreeDeque(bits);
            return 0;
        }

        tmp++;
        n_bytes++;
    }

    Deque *header_bits = AllocateDeque();
    if (n_bytes != slength || !UIntToBits(n_bytes, UINT16_SIZE, header_bits, 1) || !MergeDeques(out, header_bits) || !MergeDeques(out, bits))
    {
        FreeDeque(header_bits);
        FreeDeque(bits);
        return 0;
    }

    FreeDeque(header_bits);
    FreeDeque(bits);
    return 1;
}


static char MantissaToBits(Double mantissa, Deque *out)
{
    Deque *bits = AllocateDeque();

    if(mantissa < 0)
    {
        if(!AddBit(bits, DEQUE_INSERT_TAIL, 1, SIGN_NEGATIVE))
        {
            FreeDeque(bits);
            return 0;
        }
    }else if(!AddBit(bits, DEQUE_INSERT_TAIL, 1, SIGN_POSITIVE))
    {
        FreeDeque(bits);
        return 0;
    }

    mantissa = fabs(mantissa);
    if (mantissa == 0.0)
    {
        if (!AddBit(bits, DEQUE_INSERT_TAIL, 1, BIT_ZERO))
        {
            FreeDeque(bits);
            return 0;
        }
    }
    else
    {
        while (mantissa != 0.0)
        {
            mantissa *= 2;
            if (mantissa >= 1)
            {
                if (!AddBit(bits, DEQUE_INSERT_TAIL, 1, BIT_ONE))
                {
                    FreeDeque(bits);
                    return 0;
                }
                mantissa -= 1;
            }
            else if (mantissa != 0.0)
            {
                if (!AddBit(bits, DEQUE_INSERT_TAIL, 1, BIT_ZERO))
                {
                    FreeDeque(bits);
                    return 0;
                }
            }
        }
    }
    if (!SizeHeaderToBits(bits->size - 1, HEADER64_SIZE, out) || !MergeDeques(out, bits))
    {
        FreeDeque(bits);
        return 0;
    }
    FreeDeque(bits);
    return 1;
}

char DoubleToBits(Double value, Deque *out)
{
    if (!IsValidDeque(out) || !CheckDoubleValue(value))
    {
        return 0;
    }

    Int32 exponent = 0;
    Double mantissa = frexp(value, &exponent);

    if (!MantissaToBits(mantissa, out) || !IntToBits(exponent, INT16_SIZE, out, 1))
    {
        return 0;
    }

    return 1;
}

char BooleanToBit(Boolean value, Deque *out)
{
    if (!IsValidDeque(out))
    {
        return 0;
    }

    if(value != 0)
    {
        if(!AddBit(out, DEQUE_INSERT_TAIL, 1, BIT_ONE))
        {
            return 0;
        }
    }else if(!AddBit(out, DEQUE_INSERT_TAIL, 1, BIT_ZERO))
    {
        return 0;
    }

    return 1;
}

// SERIALIZATION FUNCTIONS END

// DESERIALIZATION FUNCTIONS START

static char HasBits(deque_size_t bitsize, deque_size_t offset, deque_size_t expected_bits)
{
    if ((bitsize == 0 && offset != 0) || expected_bits == 0 || expected_bits > bitsize || offset > bitsize || bitsize - offset < expected_bits)
    {
        return 0;
    }
    return 1;
}

static deque_size_t IncrementOffset(Deque *deque, deque_size_t offset, deque_size_t increment)
{
    if (deque->size == 0 || increment == 0)
    {
        return offset;
    }
    else if (offset + increment >= deque->size - 1)
    {
        return deque->size - 1;
    }
    return offset + increment;
}

deque_size_t BitsToSizeHeader(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, enum IntegerHeaderSize header_bitsize, UInt8 *out)
{
    if (!IsValidDeque(bits) || !out || !off_start_node || NULL == *off_start_node || header_bitsize < HEADER8_SIZE || header_bitsize > HEADER64_SIZE || !HasBits(bits->size, off_start_index, header_bitsize))
    {
        return 0;
    }

    UInt8 res = 0, i = 0;

    while (off_start_node != NULL && *off_start_node != NULL && i < header_bitsize && i + off_start_index < bits->size)
    {
        res += (*off_start_node)->value.bit_v ? (1 << i) : 0;
        *off_start_node = (*off_start_node)->next_node;
        i++;
    }

    if (i == header_bitsize)
    {
        *out = res + 1;
        return IncrementOffset(bits, off_start_index, i);
    }
    return 0;
}

deque_size_t BitsToInt8(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, Int8 *out, unsigned char has_header)
{
    if (!IsValidDeque(bits) || !out || !off_start_node || NULL == *off_start_node || !HasBits(bits->size, off_start_index, HEADER8_SIZE))
    {
        return 0;
    }

    UInt8 header = 0;
    if(has_header)
    {
        if (!(off_start_index = BitsToSizeHeader(bits, off_start_node, off_start_index, HEADER8_SIZE, &header)) || NULL == off_start_node || NULL == *off_start_node)
        {
            return 0;
        }
    }
    else
    {
        header = INT8_SIZE;
    }

    unsigned char signbit = (*off_start_node)->value.bit_v;
    if (signbit != SIGN_NEGATIVE && signbit != SIGN_POSITIVE)
    {
        return 0;
    }

    *off_start_node = (*off_start_node)->next_node;
    off_start_index++;

    Int8 res = 0;
    UInt8 i = 0;
    while (off_start_node != NULL && *off_start_node != NULL && i < header && i + off_start_index < bits->size)
    {
        res += (*off_start_node)->value.bit_v ? (1 << i) : 0;
        *off_start_node = (*off_start_node)->next_node;
        i++;
    }

    if (i == header)
    {
        *out = signbit == SIGN_POSITIVE ? res : -(res);
        return IncrementOffset(bits, off_start_index, i);
    }

    return 0;
}

deque_size_t BitsToInt16(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, Int16 *out, unsigned char has_header)
{
    if (!IsValidDeque(bits) || !out || !off_start_node || NULL == *off_start_node || !HasBits(bits->size, off_start_index, HEADER16_SIZE))
    {
        return 0;
    }

    UInt8 header = 0;
    if(has_header)
    {
        if (!(off_start_index = BitsToSizeHeader(bits, off_start_node, off_start_index, HEADER16_SIZE, &header)) || NULL == off_start_node || NULL == *off_start_node)
        {
            return 0;
        }
    }
    else
    {
        header = INT16_SIZE;
    }

    unsigned char signbit = (*off_start_node)->value.bit_v;
    if (signbit != SIGN_NEGATIVE && signbit != SIGN_POSITIVE)
    {
        return 0;
    }

    *off_start_node = (*off_start_node)->next_node;
    off_start_index++;

    Int16 res = 0;
    UInt8 i = 0;
    while (off_start_node != NULL && *off_start_node != NULL && i < header && i + off_start_index < bits->size)
    {
        res += (*off_start_node)->value.bit_v ? (1 << i) : 0;
        *off_start_node = (*off_start_node)->next_node;
        i++;
    }

    if (i == header)
    {
        *out = signbit == SIGN_POSITIVE ? res : -(res);
        return IncrementOffset(bits, off_start_index, i);
    }

    return 0;
}

deque_size_t BitsToInt32(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, Int32 *out, unsigned char has_header)
{

    if (!IsValidDeque(bits) || !out || !off_start_node || NULL == *off_start_node || !HasBits(bits->size, off_start_index, HEADER32_SIZE))
    {
        return 0;
    }

    UInt8 header = 0;
    if(has_header)
    {
        if (!(off_start_index = BitsToSizeHeader(bits, off_start_node, off_start_index, HEADER32_SIZE, &header)) || NULL == off_start_node || NULL == *off_start_node)
        {
            return 0;
        }
    }
    else
    {
        header = INT32_SIZE;
    }

    unsigned char signbit = (*off_start_node)->value.bit_v;
    if (signbit != SIGN_NEGATIVE && signbit != SIGN_POSITIVE)
    {
        return 0;
    }

    *off_start_node = (*off_start_node)->next_node;
    off_start_index++;

    Int32 res = 0;
    UInt8 i = 0;
    while (off_start_node != NULL && *off_start_node != NULL && i < header && i + off_start_index < bits->size)
    {
        res += (*off_start_node)->value.bit_v ? (1 << i) : 0;
        *off_start_node = (*off_start_node)->next_node;
        i++;
    }

    if (i == header)
    {
        *out = signbit == SIGN_POSITIVE ? res : -(res);
        return IncrementOffset(bits, off_start_index, i);
    }

    return 0;
}

deque_size_t BitsToInt64(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, Int64 *out, unsigned char has_header)
{
    if (!IsValidDeque(bits) || !out || !off_start_node || NULL == *off_start_node || !HasBits(bits->size, off_start_index, HEADER64_SIZE))
    {
        return 0;
    }

    UInt8 header = 0;
    if(has_header)
    {
        if (!(off_start_index = BitsToSizeHeader(bits, off_start_node, off_start_index, HEADER64_SIZE, &header)) || NULL == off_start_node || NULL == *off_start_node)
        {
            return 0;
        }
    }
    else
    {
        header = INT64_SIZE;
    }

    unsigned char signbit = (*off_start_node)->value.bit_v;
    if (signbit != SIGN_NEGATIVE && signbit != SIGN_POSITIVE)
    {
        return 0;
    }

    *off_start_node = (*off_start_node)->next_node;
    off_start_index++;

    Int64 res = 0;
    UInt8 i = 0;
    while (off_start_node != NULL && *off_start_node != NULL && i < header && i + off_start_index < bits->size)
    {
        res += (*off_start_node)->value.bit_v ? (1 << i) : 0;
        *off_start_node = (*off_start_node)->next_node;
        i++;
    }

    if (i == header)
    {
        *out = signbit == SIGN_POSITIVE ? res : -(res);
        return IncrementOffset(bits, off_start_index, i);
    }

    return 0;
}

deque_size_t BitsToUInt8(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, UInt8 *out, unsigned char has_header)
{
    if (!IsValidDeque(bits) || !out || !off_start_node || NULL == *off_start_node || !HasBits(bits->size, off_start_index, HEADER8_SIZE))
    {
        return 0;
    }

    UInt8 header = 0;
    if(has_header)
    {
        if (!(off_start_index = BitsToSizeHeader(bits, off_start_node, off_start_index, HEADER8_SIZE, &header)) || NULL == off_start_node || NULL == *off_start_node)
        {
            return 0;
        }
    }
    else
    {
        header = UINT8_SIZE;
    }

    UInt8 res = 0, i = 0;
    while (off_start_node != NULL && *off_start_node != NULL && i < header && i + off_start_index < bits->size)
    {
        res += (*off_start_node)->value.bit_v ? (1 << i) : 0;
        *off_start_node = (*off_start_node)->next_node;
        i++;
    }

    if (i == header)
    {
        *out = res;
        return IncrementOffset(bits, off_start_index, i);
    }

    return 0;
}

deque_size_t BitsToUInt16(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, UInt16 *out, unsigned char has_header)
{
    if (!IsValidDeque(bits) || !out || !off_start_node || NULL == *off_start_node || !HasBits(bits->size, off_start_index, HEADER16_SIZE))
    {
        return 0;
    }

    UInt8 header = 0;
    if(has_header)
    {
        if (!(off_start_index = BitsToSizeHeader(bits, off_start_node, off_start_index, HEADER16_SIZE, &header)) || NULL == off_start_node || NULL == *off_start_node)
        {
            return 0;
        }
    }
    else
    {
        header = UINT16_SIZE;
    }

    UInt16 res = 0;
    UInt8 i = 0;
    while (off_start_node != NULL && *off_start_node != NULL && i < header && i + off_start_index < bits->size)
    {
        res += (*off_start_node)->value.bit_v ? (1 << i) : 0;
        *off_start_node = (*off_start_node)->next_node;
        i++;
    }

    if (i == header)
    {
        *out = res;
        return IncrementOffset(bits, off_start_index, i);
    }

    return 0;
}

deque_size_t BitsToUInt32(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, UInt32 *out, unsigned char has_header)
{
    if (!IsValidDeque(bits) || !out || !off_start_node || NULL == *off_start_node || !HasBits(bits->size, off_start_index, HEADER32_SIZE))
    {
        return 0;
    }

    UInt8 header = 0;
    if(has_header)
    {
        if (!(off_start_index = BitsToSizeHeader(bits, off_start_node, off_start_index, HEADER32_SIZE, &header)) || NULL == off_start_node || NULL == *off_start_node)
        {
            return 0;
        }
    }
    else
    {
        header = UINT32_SIZE;
    }

    UInt32 res = 0;
    UInt8 i = 0;
    while (off_start_node != NULL && *off_start_node != NULL && i < header && i + off_start_index < bits->size)
    {
        res += (*off_start_node)->value.bit_v ? (1 << i) : 0;
        *off_start_node = (*off_start_node)->next_node;
        i++;
    }

    if (i == header)
    {
        *out = res;
        return IncrementOffset(bits, off_start_index, i);
    }

    return 0;
}

deque_size_t BitsToUInt64(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, UInt64 *out, unsigned char has_header)
{
    if (!IsValidDeque(bits) || !out || !off_start_node || NULL == *off_start_node || !HasBits(bits->size, off_start_index, HEADER64_SIZE))
    {
        return 0;
    }

    UInt8 header = 0;
    if(has_header)
    {
        if (!(off_start_index = BitsToSizeHeader(bits, off_start_node, off_start_index, HEADER64_SIZE, &header)) || NULL == off_start_node || NULL == *off_start_node)
        {
            return 0;
        }
    }
    else
    {
        header = UINT64_SIZE;
    }

    UInt64 res = 0;
    UInt8 i = 0;
    while (off_start_node != NULL && *off_start_node != NULL && i < header && i + off_start_index < bits->size)
    {
        res += (*off_start_node)->value.bit_v ? (1 << i) : 0;
        *off_start_node = (*off_start_node)->next_node;
        i++;
    }

    if (i == header)
    {
        *out = res;
        return IncrementOffset(bits, off_start_index, i);
    }

    return 0;
}

deque_size_t BitsToUTF8String(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, UnicodeStringBucket *out)
{
    if (!IsValidDeque(bits) || !out || !off_start_node || NULL == *off_start_node || !HasBits(bits->size, off_start_index, HEADER16_SIZE))
    {
        return 0;
    }

    UInt16 string_length;

    if (!(off_start_index = BitsToUInt16(bits, off_start_node, off_start_index, &string_length, 1)))
    {
        return 0;
    }

    char *tmp = NULL, *res = (char *)calloc(string_length + 1, sizeof(char));
    if (!res)
    {
        return 0;
    }
    tmp = res;
    Int8 i8 = 0;

    for(size_t c = 0; tmp != NULL && c < string_length; c++)
    {
        if(off_start_node == NULL || *off_start_node == NULL || !(off_start_index = BitsToInt8(bits, off_start_node, off_start_index, &i8, 0)) || i8 == '\0')
        {
            free(res);
            return 0;
        }
        *tmp = i8 ; tmp++;
    }

    res[string_length] = '\0';

    out->length = string_length;
    out->u8string = res;

    return off_start_index;
}

static deque_size_t BitsToMantissa(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, Double *out)
{

    UInt8 header;
    if (!(off_start_index = BitsToSizeHeader(bits, off_start_node, off_start_index, HEADER64_SIZE, &header)) || !HasBits(bits->size, off_start_index, header))
    {
        return 0;
    }

    unsigned char signbit = (*off_start_node)->value.bit_v;
    if (signbit != SIGN_NEGATIVE && signbit != SIGN_POSITIVE)
    {
        return 0;
    }

    *off_start_node = (*off_start_node)->next_node;
    off_start_index++;

    Double mantissa = 0.0;
    UInt8 c = 0;
    while (c < header && *off_start_node != NULL)
    {
        mantissa += (*off_start_node)->value.bit_v ? ldexp(1, -(c + 1)) : 0;
        *off_start_node = (*off_start_node)->next_node;
        c++;
    }

    if (c == header)
    {
        *out = signbit == SIGN_POSITIVE ? mantissa : -(mantissa);
        return IncrementOffset(bits, off_start_index, c);
    }

    return 0;
}

deque_size_t BitsToDouble(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, Double *out)
{
    if (!IsValidDeque(bits) || !out || !off_start_node || NULL == *off_start_node)
    {
        return 0;
    }

    Double mantissa = 0;
    if (!(off_start_index = BitsToMantissa(bits, off_start_node, off_start_index, &mantissa)))
    {
        return 0;
    }

    Int16 exponent = 0;
    if (!(off_start_index = BitsToInt16(bits, off_start_node, off_start_index, &exponent, 1)))
    {
        return 0;
    }

    *out = ldexp(mantissa, exponent);

    return off_start_index;
}

deque_size_t BitToBoolean(Deque *bits, DequeNode **off_start_node, deque_size_t off_start_index, Boolean *out)
{
    if (!IsValidDeque(bits) || !out || !off_start_node || NULL == *off_start_node || !HasBits(bits->size, off_start_index, 1))
    {
        return 0;
    }

    *out = (*off_start_node)->value.bit_v;
    *off_start_node = (*off_start_node)->next_node;

    return off_start_index + 1;
}
