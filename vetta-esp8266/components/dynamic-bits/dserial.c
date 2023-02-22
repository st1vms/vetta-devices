#include <stdlib.h>
#include <math.h>
#include "dtypes.h"

static const unsigned char BIT_MASKS[8] = {
    (1U << 0),
    (1U << 1),
    (1U << 2),
    (1U << 3),
    (1U << 4),
    (1U << 5),
    (1U << 6),
    (1U << 7),
};

unsigned char GetIntBitsize(Int64 v)
{
    if (v == 0)
    {
        return 1;
    }

    UInt64 u = 0;
    if (v < 0)
    {
        u = (UInt64)llabs(v);
    }
    else
    {
        u = (UInt64)v;
    }

    unsigned char bitsize = 0;
    while (u != 0)
    {
        bitsize++;
        u >>= 1;
    }
    return bitsize;
}

unsigned char GetUIntBitsize(UInt64 v)
{
    if (v == 0)
    {
        return 1;
    }

    unsigned char bitsize = 0;
    while (v != 0)
    {
        bitsize++;
        v >>= 1;
    }
    return bitsize;
}

unsigned char GetDoubleMantissaBitsize(Double d)
{
    unsigned char bitsize = 0;

    Int32 exponent = 0;
    Double m = frexp(d, &exponent);

    if (m < 0)
    {
        m = fabs(m);
    }

    if (m == 0.0)
    {
        return 1;
    }

    while (m != 0.0)
    {
        m *= 2;
        if (m >= 1)
        {
            m--;
        }
        bitsize++;
    }

    return bitsize;
}

static void SizeIncrementCheck(size_t *size_off, size_t buffer_size, unsigned char *bit_off)
{

    if (*bit_off == 8 && *size_off < buffer_size)
    {
        *bit_off = 0;
        *size_off += 1;
    }
}
static void SetBufferBit_Serialize(unsigned char **buffer_p, size_t *size_off, unsigned char *bit_off)
{
    if (buffer_p != NULL)
    {
        (*buffer_p)[*size_off] |= (1 << *bit_off);
    }
}

char SerializeNumericalHeader(UInt8 header_value,
                              data_header_size_t header_size,
                              unsigned char *buffer_p,
                              size_t buffer_size,
                              size_t *size_off,
                              unsigned char *bit_off)
{
    if (header_value == 0 || header_size == NO_HEADER || header_size > HEADER64_SIZE ||
        NULL == buffer_p || NULL == size_off || NULL == bit_off || *bit_off > 8 || buffer_size == 0)
    {
        return 0;
    }

    header_value -= 1;

    for (int i = 0; i < header_size; i++)
    {

        SizeIncrementCheck(size_off, buffer_size, bit_off);

        if (header_value != 0)
        {
            if (header_value & 1)
            {
                SetBufferBit_Serialize(&buffer_p, size_off, bit_off);
            }
            header_value >>= 1;
        }

        *bit_off += 1;
    }

    return 1;
}

char SerializeUInt(UInt64 uval,
                   unsigned char *buffer_p,
                   size_t buffer_size,
                   size_t *size_off,
                   unsigned char *bit_off)
{

    if (NULL == buffer_p || buffer_size == 0 ||
        NULL == size_off || NULL == bit_off || *bit_off > 8)
    {
        return 0;
    }

    SizeIncrementCheck(size_off, buffer_size, bit_off);

    if (uval == 0)
    {
        *bit_off += 1;
        return 1;
    }

    while (uval != 0)
    {
        SizeIncrementCheck(size_off, buffer_size, bit_off);

        if (uval & 1)
        {
            SetBufferBit_Serialize(&buffer_p, size_off, bit_off);
        }
        uval >>= 1;
        *bit_off += 1;
    }

    return 1;
}

char SerializeInt(Int64 ival,
                  unsigned char *buffer_p,
                  size_t buffer_size,
                  size_t *size_off,
                  unsigned char *bit_off)
{
    if (NULL == buffer_p || buffer_size == 0 ||
        NULL == size_off || NULL == bit_off || *bit_off > 8)
    {
        return 0;
    }

    SizeIncrementCheck(size_off, buffer_size, bit_off);

    if (ival < 0)
    {
        // Set sign bit
        // 0 POSITIVE | 1 NEGATIVE
        SetBufferBit_Serialize(&buffer_p, size_off, bit_off);
    }
    *bit_off += 1;

    SizeIncrementCheck(size_off, buffer_size, bit_off);
    // Write unsigned repr.
    UInt64 uval = (UInt64)llabs(ival);

    if (uval == 0)
    {
        *bit_off += 1;
        return 1;
    }

    while (uval != 0)
    {
        SizeIncrementCheck(size_off, buffer_size, bit_off);

        if (uval != -1 && uval & 1)
        {
            SetBufferBit_Serialize(&buffer_p, size_off, bit_off);
        }
        else if (uval == -1)
        {
            break;
        }

        uval >>= 1;
        *bit_off += 1;
    }

    return 1;
}

char SerializeDouble(Double dval,
                     unsigned char *buffer_p,
                     size_t buffer_size,
                     size_t *size_off,
                     unsigned char *bit_off)
{
    if (NULL == buffer_p || buffer_size == 0 ||
        NULL == size_off || NULL == bit_off || *bit_off > 8)
    {
        return 0;
    }

    SizeIncrementCheck(size_off, buffer_size, bit_off);

    Int32 exponent = 0;
    Double m = frexp(dval, &exponent);

    // Serialize Mantissa
    if (m < 0)
    {
        m = fabs(m);
        // Set sign bit
        SetBufferBit_Serialize(&buffer_p, size_off, bit_off);
    }

    *bit_off += 1;

    SizeIncrementCheck(size_off, buffer_size, bit_off);

    if (m == 0.0)
    {
        *bit_off += 1;
    }
    else
    {
        while (m != 0.0)
        {
            SizeIncrementCheck(size_off, buffer_size, bit_off);
            m *= 2;
            if (m >= 1)
            {
                SetBufferBit_Serialize(&buffer_p, size_off, bit_off);
                m -= 1;
            }
            *bit_off += 1;
        }
    }

    // Serialize Exponent
    unsigned char exp_header_bitsize = GetIntBitsize(exponent);
    if (!exp_header_bitsize ||
        !SerializeNumericalHeader(exp_header_bitsize, HEADER16_SIZE, buffer_p, buffer_size, size_off, bit_off) ||
        !SerializeInt(exponent, buffer_p, buffer_size, size_off, bit_off))
    {
        return 0;
    }

    return 1;
}

char SerializeUTF8String(utf8_string_t dval,
                         unsigned char *buffer_p,
                         size_t buffer_size,
                         size_t *size_off,
                         unsigned char *bit_off)
{
    if (NULL == buffer_p || buffer_size == 0 ||
        NULL == size_off || NULL == bit_off || *bit_off > 8 ||
        dval.length >= MAX_STRING_LENGTH)
    {
        return 0;
    }

    SizeIncrementCheck(size_off, buffer_size, bit_off);

    UInt8 header_size = 0;
    // Serialize appropriate string length header
    if (MAX_STRING_LENGTH <= UINT8_MAX)
    {
        header_size = GetUIntBitsize(dval.length);
        if (header_size == 0 ||
            !SerializeNumericalHeader(header_size, HEADER8_SIZE, buffer_p, buffer_size, size_off, bit_off))
        {
            return 0;
        }
    }
    else if (MAX_STRING_LENGTH <= UINT16_MAX)
    {
        header_size = GetUIntBitsize(dval.length);
        if (header_size == 0 ||
            !SerializeNumericalHeader(header_size, HEADER16_SIZE, buffer_p, buffer_size, size_off, bit_off))
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }

    // Serialize string length
    if (!SerializeUInt(dval.length, buffer_p, buffer_size, size_off, bit_off))
    {
        return 0;
    }

    // Serialize string bytes
    UInt8 *v = dval.utf8_string;
    for (size_t i = 0; v != NULL && i < dval.length; v++, i++)
    {

        for (size_t b = 0; b < UINT8_SIZE; b++)
        {

            SizeIncrementCheck(size_off, buffer_size, bit_off);

            if (*v != 0)
            {
                if (*v & 1)
                {
                    SetBufferBit_Serialize(&buffer_p, size_off, bit_off);
                }
                *v >>= 1;
            }

            *bit_off += 1;
        }
    }

    return 1;
}

char SerializeBoolean(Boolean boolv,
                      unsigned char *buffer_p,
                      size_t buffer_size,
                      size_t *size_off,
                      unsigned char *bit_off)
{
    if (NULL == buffer_p || buffer_size == 0 ||
        NULL == size_off || NULL == bit_off || *bit_off > 8)
    {
        return 0;
    }

    SizeIncrementCheck(size_off, buffer_size, bit_off);

    if (boolv > 0)
    {
        SetBufferBit_Serialize(&buffer_p, size_off, bit_off);
    }

    *bit_off += 1;

    return 1;
}

static char DeserializeArgCheck(const unsigned char *buffer,
                                size_t *m_bytes,
                                size_t *bit_count,
                                int bitsize)
{
    if (buffer == NULL || bit_count == NULL ||
        bitsize == 0 || m_bytes == NULL || *m_bytes == 0)
    {
        return 0;
    }
    return 1;
}

static void ByteIncrementCheck(unsigned char **buffer, size_t *bit_count, size_t *m_bytes)
{
    if (buffer != NULL && bit_count != NULL && m_bytes != NULL &&
        *bit_count >= 8 && *m_bytes > 0)
    {
        *buffer += 1;
        *m_bytes -= 1;
        *bit_count = 0;
    }
}

unsigned char *DeserializeBoolean(unsigned char *buffer,
                                  size_t *m_bytes,
                                  size_t *bit_count,
                                  Boolean *out)
{
    if (out == NULL || !DeserializeArgCheck(buffer, m_bytes, bit_count, 1))
    {
        return NULL;
    }

    ByteIncrementCheck(&buffer, bit_count, m_bytes);

    *out = (*buffer & BIT_MASKS[*bit_count]) ? 1 : 0;
    *bit_count += 1;
    return buffer;
}

unsigned char *DeserializeUInt8(unsigned char *buffer,
                                size_t *m_bytes,
                                size_t *bit_count,
                                unsigned_int_size_t uint_size,
                                unsigned char is_header,
                                UInt8 *out)
{
    *out = is_header ? 1 : 0;

    if (out == NULL || !DeserializeArgCheck(buffer, m_bytes, bit_count, uint_size))
    {
        return NULL;
    }

    for (unsigned char bit_pos = 0;
         bit_pos < uint_size && buffer != NULL && bit_count != NULL;
         bit_pos++)
    {
        ByteIncrementCheck(&buffer, bit_count, m_bytes);

        if (NULL == buffer || bit_count == NULL)
        {
            return NULL;
        }

        if (*buffer & BIT_MASKS[*bit_count])
        {
            *out += (1ULL << bit_pos);
        }
        *bit_count += 1;
    }

    return buffer;
}

unsigned char *DeserializeUInt16(unsigned char *buffer,
                                 size_t *m_bytes,
                                 size_t *bit_count,
                                 unsigned_int_size_t uint_size,
                                 UInt16 *out)
{
    *out = 0;
    if (out == NULL || !DeserializeArgCheck(buffer, m_bytes, bit_count, uint_size))
    {
        return NULL;
    }

    for (unsigned char bit_pos = 0;
         bit_pos < uint_size && buffer != NULL && bit_count != NULL;
         bit_pos++)
    {
        ByteIncrementCheck(&buffer, bit_count, m_bytes);

        if (NULL == buffer || bit_count == NULL)
        {
            return NULL;
        }

        if (*buffer & BIT_MASKS[*bit_count])
        {
            *out += (1ULL << bit_pos);
        }
        *bit_count += 1;
    }

    return buffer;
}

unsigned char *DeserializeUInt32(unsigned char *buffer,
                                 size_t *m_bytes,
                                 size_t *bit_count,
                                 unsigned_int_size_t uint_size,
                                 UInt32 *out)
{
    *out = 0;
    if (out == NULL || !DeserializeArgCheck(buffer, m_bytes, bit_count, uint_size))
    {
        return NULL;
    }

    for (unsigned char bit_pos = 0;
         bit_pos < uint_size && buffer != NULL && bit_count != NULL;
         bit_pos++)
    {
        ByteIncrementCheck(&buffer, bit_count, m_bytes);

        if (NULL == buffer || bit_count == NULL)
        {
            return NULL;
        }

        if (*buffer & BIT_MASKS[*bit_count])
        {
            *out += (1ULL << bit_pos);
        }
        *bit_count += 1;
    }

    return buffer;
}

unsigned char *DeserializeUInt64(unsigned char *buffer,
                                 size_t *m_bytes,
                                 size_t *bit_count,
                                 unsigned_int_size_t uint_size,
                                 UInt64 *out)
{
    *out = 0;
    if (out == NULL || !DeserializeArgCheck(buffer, m_bytes, bit_count, uint_size))
    {
        return NULL;
    }

    for (unsigned char bit_pos = 0;
         bit_pos < uint_size && buffer != NULL && bit_count != NULL;
         bit_pos++)
    {
        ByteIncrementCheck(&buffer, bit_count, m_bytes);

        if (NULL == buffer || bit_count == NULL)
        {
            return NULL;
        }

        if (*buffer & BIT_MASKS[*bit_count])
        {
            *out += (1ULL << bit_pos);
        }
        *bit_count += 1;
    }

    return buffer;
}

unsigned char *DeserializeInt8(unsigned char *buffer,
                               size_t *m_bytes,
                               size_t *bit_count,
                               unsigned_int_size_t int_size,
                               Int8 *out)
{
    if (out == NULL || !DeserializeArgCheck(buffer, m_bytes, bit_count, int_size))
    {
        return NULL;
    }

    ByteIncrementCheck(&buffer, bit_count, m_bytes);

    // 0 POSITIVE | 1 NEGATIVE
    const unsigned char sign = (*buffer & BIT_MASKS[*bit_count]) ? 1 : 0;
    *bit_count += 1;

    UInt8 tmp = 0;

    for (unsigned char bit_pos = 0;
         bit_pos < int_size && buffer != NULL && bit_count != NULL;
         bit_pos++)
    {
        ByteIncrementCheck(&buffer, bit_count, m_bytes);

        if (NULL == buffer || bit_count == NULL || out == NULL)
        {
            return NULL;
        }

        if (*buffer & BIT_MASKS[*bit_count])
        {
            tmp += (1ULL << bit_pos);
        }
        *bit_count += 1;
    }

    if (sign)
    {
        *out = -(tmp);
    }
    else
    {
        *out = (Int8)tmp;
    }

    return buffer;
}

unsigned char *DeserializeInt16(unsigned char *buffer,
                                size_t *m_bytes,
                                size_t *bit_count,
                                unsigned_int_size_t int_size,
                                Int16 *out)
{
    if (out == NULL || !DeserializeArgCheck(buffer, m_bytes, bit_count, int_size))
    {
        return NULL;
    }

    ByteIncrementCheck(&buffer, bit_count, m_bytes);

    // 0 POSITIVE | 1 NEGATIVE
    const unsigned char sign = (*buffer & BIT_MASKS[*bit_count]) ? 1 : 0;
    *bit_count += 1;

    UInt16 tmp = 0;

    for (unsigned char bit_pos = 0;
         bit_pos < int_size && buffer != NULL && bit_count != NULL;
         bit_pos++)
    {
        ByteIncrementCheck(&buffer, bit_count, m_bytes);

        if (NULL == buffer || bit_count == NULL || out == NULL)
        {
            return NULL;
        }

        if (*buffer & BIT_MASKS[*bit_count])
        {
            tmp += (1ULL << bit_pos);
        }
        *bit_count += 1;
    }

    if (sign)
    {
        *out = -(tmp);
    }
    else
    {
        *out = (Int16)tmp;
    }

    return buffer;
}

unsigned char *DeserializeInt32(unsigned char *buffer,
                                size_t *m_bytes,
                                size_t *bit_count,
                                unsigned_int_size_t int_size,
                                Int32 *out)
{
    if (out == NULL || !DeserializeArgCheck(buffer, m_bytes, bit_count, int_size))
    {
        return NULL;
    }

    ByteIncrementCheck(&buffer, bit_count, m_bytes);

    // 0 POSITIVE | 1 NEGATIVE
    const unsigned char sign = (*buffer & BIT_MASKS[*bit_count]) ? 1 : 0;
    *bit_count += 1;

    UInt32 tmp = 0;

    for (unsigned char bit_pos = 0;
         bit_pos < int_size && buffer != NULL && bit_count != NULL;
         bit_pos++)
    {
        ByteIncrementCheck(&buffer, bit_count, m_bytes);

        if (NULL == buffer || bit_count == NULL || out == NULL)
        {
            return NULL;
        }

        if (*buffer & BIT_MASKS[*bit_count])
        {
            tmp += (1ULL << bit_pos);
        }
        *bit_count += 1;
    }

    if (sign)
    {
        *out = -(tmp);
    }
    else
    {
        *out = (Int32)tmp;
    }

    return buffer;
}

unsigned char *DeserializeInt64(unsigned char *buffer,
                                size_t *m_bytes,
                                size_t *bit_count,
                                unsigned_int_size_t int_size,
                                Int64 *out)
{
    if (out == NULL || !DeserializeArgCheck(buffer, m_bytes, bit_count, int_size))
    {
        return NULL;
    }

    ByteIncrementCheck(&buffer, bit_count, m_bytes);

    // 0 POSITIVE | 1 NEGATIVE
    const unsigned char sign = (*buffer & BIT_MASKS[*bit_count]) ? 1 : 0;
    *bit_count += 1;

    UInt64 tmp = 0;

    for (unsigned char bit_pos = 0;
         bit_pos < int_size && buffer != NULL && bit_count != NULL;
         bit_pos++)
    {
        ByteIncrementCheck(&buffer, bit_count, m_bytes);

        if (NULL == buffer || bit_count == NULL || out == NULL)
        {
            return NULL;
        }

        if (*buffer & BIT_MASKS[*bit_count])
        {
            tmp += (1ULL << bit_pos);
        }
        *bit_count += 1;
    }

    if (sign)
    {
        *out = -(tmp);
    }
    else
    {
        *out = (Int64)tmp;
    }

    return buffer;
}

unsigned char *DeserializeDouble(unsigned char *buffer,
                                 size_t *m_bytes,
                                 size_t *bit_count,
                                 Double *out)
{
    if (out == NULL || !DeserializeArgCheck(buffer, m_bytes, bit_count, 1))
    {
        return NULL;
    }

    ByteIncrementCheck(&buffer, bit_count, m_bytes);

    UInt8 header_size = 0;
    if (NULL == (buffer = DeserializeUInt8(buffer, m_bytes, bit_count, (int)HEADER64_SIZE, 1, &header_size)))
    {
        return NULL;
    }

    ByteIncrementCheck(&buffer, bit_count, m_bytes);

    // 0 POSITIVE | 1 NEGATIVE
    const unsigned char sign = (*buffer & BIT_MASKS[*bit_count]) ? 1 : 0;
    *bit_count += 1;

    Double v = 0.0, m = 0.0;
    for (unsigned char pos = 0;
         buffer != NULL && bit_count != NULL && m_bytes != NULL && pos < header_size;
         pos++)
    {
        ByteIncrementCheck(&buffer, bit_count, m_bytes);
        if (*buffer & BIT_MASKS[*bit_count])
        {
            m += ldexp(1, -(pos + 1));
        }
        *bit_count += 1;
    }
    v = m; // double precision change on Exp deser. fix

    Int16 exponent = 0;
    if (NULL == (buffer = DeserializeUInt8(buffer, m_bytes, bit_count, (int)HEADER16_SIZE, 1, &header_size)) ||
        NULL == (buffer = DeserializeInt16(buffer, m_bytes, bit_count, header_size, &exponent)))
    {
        return NULL;
    }
    // NOTE
    // m (mantissa) changes value here, v doesn't ...

    if (sign)
    {
        *out = -(ldexp(v, exponent));
    }
    else
    {
        *out = ldexp(v, exponent);
    }

    return buffer;
}

unsigned char *DeserializeUTF8String(unsigned char *buffer,
                                     size_t *m_bytes,
                                     size_t *bit_count,
                                     utf8_string_t *out)
{
    if (out == NULL || !DeserializeArgCheck(buffer, m_bytes, bit_count, 1))
    {
        return NULL;
    }

    ByteIncrementCheck(&buffer, bit_count, m_bytes);

    // Deserialize appropriate string length
    UInt16 header_value_16 = 0;
    UInt8 header_value_8 = 0;
    UInt8 header_value = 0;

    if (MAX_STRING_LENGTH <= UINT8_MAX)
    {
        if (!(buffer = DeserializeUInt8(buffer, m_bytes, bit_count, (int)HEADER8_SIZE, 1, &header_value)) ||
            !(buffer = DeserializeUInt8(buffer, m_bytes, bit_count, header_value, 0, &header_value_8)))
        {
            return NULL;
        }
        out->length = header_value_8;
    }
    else if (MAX_STRING_LENGTH <= UINT16_MAX)
    {
        if (!(buffer = DeserializeUInt8(buffer, m_bytes, bit_count, (int)HEADER16_SIZE, 1, &header_value)) ||
            !(buffer = DeserializeUInt16(buffer, m_bytes, bit_count, header_value, &header_value_16)))
        {
            return NULL;
        }
        out->length = header_value_16;
    }
    else
    {
        return NULL;
    }

    if (out->length >= MAX_STRING_LENGTH)
    {
        return NULL;
    }

    // Deserialize string bytes
    UInt8 v = 0;
    for (size_t i = 0; i < out->length; i++)
    {

        for (size_t b = 0; b < UINT8_SIZE; b++)
        {
            if (NULL == buffer || bit_count == NULL || out == NULL)
            {
                return NULL;
            }

            ByteIncrementCheck(&buffer, bit_count, m_bytes);

            if (*buffer & BIT_MASKS[*bit_count])
            {
                v += (1U << b);
            }

            *bit_count += 1;
        }

        out->utf8_string[i] = v;
        v = 0;
    }

    return buffer;
}
