#ifndef __DTYPES_H

#include <stddef.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_STRING_LENGTH (0xff)

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum signed_int_size_t
    {
        INT_NOSIZE = 0x0,
        INT8_SIZE = 0x7,
        INT16_SIZE = 0xf,
        INT32_SIZE = 0x1f,
        INT64_SIZE = 0x3f
    } signed_int_size_t;

    typedef enum unsigned_int_size_t
    {
        UINT_NOSIZE = 0x0,
        UINT8_SIZE = 0x8,
        UINT16_SIZE = 0x10,
        UINT32_SIZE = 0x20,
        UINT64_SIZE = 0x40
    } unsigned_int_size_t;

    typedef enum data_header_size_t
    {
        NO_HEADER = 0x0,
        HEADER8_SIZE = 0x3,
        HEADER16_SIZE = 0x4,
        HEADER32_SIZE = 0x5,
        HEADER64_SIZE = 0x6
    } data_header_size_t;

    typedef unsigned char Boolean;

    typedef signed char Int8;
    typedef signed short Int16;
    typedef signed int Int32;
    typedef signed long long Int64;

    typedef unsigned char UInt8;
    typedef unsigned short UInt16;
    typedef unsigned int UInt32;
    typedef unsigned long long UInt64;

    typedef double Double;

    typedef struct utf8_string_t
    {
        size_t length;
        UInt8 utf8_string[MAX_STRING_LENGTH];
    } utf8_string_t;

    typedef union decimal_union_t
    {
        UInt8 u8_v;
        UInt16 u16_v;
        UInt32 u32_v;
        UInt64 u64_v;
        Int8 i8_v;
        Int16 i16_v;
        Int32 i32_v;
        Int64 i64_v;
    } decimal_union_t;

    typedef union data_union_t
    {
        Double double_v;

        Boolean boolean_v;

        utf8_string_t utf8_str_v;

        decimal_union_t decimal_v;

    } data_union_t;

    typedef enum serializable_type_t
    {
        NO_TYPE = 0x0,
        UINT8_STYPE = 0x1,
        UINT16_STYPE = 0x2,
        UINT32_STYPE = 0x3,
        UINT64_STYPE = 0x4,
        INT8_STYPE = 0x5,
        INT16_STYPE = 0x6,
        INT32_STYPE = 0x7,
        INT64_STYPE = 0x8,
        DOUBLE_STYPE = 0x9,
        BOOLEAN_STYPE = 0xa,
        UTF8_STRING_STYPE = 0xb,
    } serializable_type_t;

    typedef UInt8 packet_id_t;

#ifdef __cplusplus
}
#endif
#define __DTYPES_H
#endif // __DTYPES_H
