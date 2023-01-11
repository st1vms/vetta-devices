#include <math.h>
#include <ctype.h>
#include <stddef.h>
#include "dbits/dtypes.h"

char GetIntHeaderSize(Int64 integer, UInt8 intsize, enum IntegerHeaderSize * out_size_enum)
{
    switch (intsize)
    {
    case INT8_SIZE:
        if (integer < -__INT8_MAX__ || integer > __INT8_MAX__)
        {
            return 0;
        }
        *out_size_enum = HEADER8_SIZE;
        break;
    case INT16_SIZE:
        if (integer < -__INT16_MAX__ || integer > __INT16_MAX__)
        {
            return 0;
        }
        *out_size_enum = HEADER16_SIZE;
        break;
    case INT32_SIZE:
        if (integer < -__INT32_MAX__ || integer > __INT32_MAX__)
        {
            return 0;
        }
        *out_size_enum = HEADER32_SIZE;
        break;
    case INT64_SIZE:
        if (integer < -__INT64_MAX__ || integer > __INT64_MAX__)
        {
            return 0;
        }
        *out_size_enum = HEADER64_SIZE;
        break;
    default:
        return 0;
    }
    return 1;
}

char GetUIntHeaderSize(UInt64 uinteger, UInt8 intsize, enum IntegerHeaderSize * out_size_enum)
{

    switch (intsize)
    {
    case UINT8_SIZE:
        if (uinteger > __UINT8_MAX__)
        {
            return 0;
        }
        *out_size_enum = HEADER8_SIZE;
        break;
    case UINT16_SIZE:
        if (uinteger > __UINT16_MAX__)
        {
            return 0;
        }
        *out_size_enum = HEADER16_SIZE;
        break;
    case UINT32_SIZE:
        if (uinteger > __UINT32_MAX__)
        {
            return 0;
        }
        *out_size_enum = HEADER32_SIZE;
        break;
    case UINT64_SIZE:
        if (uinteger > __UINT64_MAX__)
        {
            return 0;
        }
        *out_size_enum = HEADER64_SIZE;
        break;
    default:
        return 0;
    }
    return 1;
}

char CheckIntValue(Int64 integer, UInt8 intsize)
{
    switch (intsize)
    {
    case INT8_SIZE:
        if (integer < -__INT8_MAX__ || integer > __INT8_MAX__)
        {
            return 0;
        }
        break;
    case INT16_SIZE:
        if (integer < -__INT16_MAX__ || integer > __INT16_MAX__)
        {
            return 0;
        }
        break;
    case INT32_SIZE:
        if (integer < -__INT32_MAX__ || integer > __INT32_MAX__)
        {
            return 0;
        }
        break;
    case INT64_SIZE:
        if (integer < -__INT64_MAX__ || integer > __INT64_MAX__)
        {
            return 0;
        }
        break;
    default:
        return 0;
    }
    return 1;
}

char CheckUIntValue(UInt64 uinteger, UInt8 intsize)
{

    switch (intsize)
    {
    case UINT8_SIZE:
        if (uinteger > __UINT8_MAX__)
        {
            return 0;
        }
        break;
    case UINT16_SIZE:
        if (uinteger > __UINT16_MAX__)
        {
            return 0;
        }
        break;
    case UINT32_SIZE:
        if (uinteger > __UINT32_MAX__)
        {
            return 0;
        }
        break;
    case UINT64_SIZE:
        if (uinteger > __UINT64_MAX__)
        {
            return 0;
        }
        break;
    default:
        return 0;
    }
    return 1;
}

unsigned char CheckDoubleValue(Double value)
{
    switch (fpclassify(value))
    {
    case FP_INFINITE:
    case FP_NAN:
        return 0;
    default:
        break;
    }
    return 1;
}

unsigned char CheckHeaderValue(UInt64 header, UInt8 hsize)
{
    if (header == 0)
    {
        return 0;
    }

    switch (hsize)
    {
    case HEADER8_SIZE:
        if (header > UINT8_SIZE)
        {
            return 0;
        }
        break;
    case HEADER16_SIZE:
        if (header > UINT16_SIZE)
        {
            return 0;
        }
        break;
    case HEADER32_SIZE:
        if (header > UINT32_SIZE)
        {
            return 0;
        }
        break;
    case HEADER64_SIZE:
        if (header > UINT64_SIZE)
        {
            return 0;
        }
        break;
    default:
        return 0;
    }
    return 1;
}


char GetUTF8ByteLength(char ** _s, size_t * size_out)
{
    if(!_s || !(*_s) || !size_out)
    {
        return 0;
    }

    const char * s = *_s;

    *size_out = 0;
    while( s != NULL && '\0' != *s && *size_out < __UINT16_MAX__){
        *size_out = *size_out + 1;
        s++;
    }
    return *size_out >= __UINT16_MAX__ ? 0 : 1;
}
