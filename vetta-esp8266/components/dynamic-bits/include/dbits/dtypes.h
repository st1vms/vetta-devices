#ifndef _DBITS_DTYPES_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

enum BitValueEnum
{
    BIT_ZERO = 0x0,
    BIT_ONE = 0x1
};

enum BitEndianessEnum
{
    BIG_ENDIAN_BITS = 0x0,
    LITTLE_ENDIAN_BITS = 0x1
};

enum SignBitEnum
{
    SIGN_POSITIVE = BIT_ZERO,
    SIGN_NEGATIVE = BIT_ONE
};

enum SignedIntegerSize
{
    INT8_SIZE = 0x7,
    INT16_SIZE = 0xf,
    INT32_SIZE = 0x1f,
    INT64_SIZE = 0x3f
};

enum UnsignedIntegerSize
{
    UINT8_SIZE = 0x8,
    UINT16_SIZE = 0x10,
    UINT32_SIZE = 0x20,
    UINT64_SIZE = 0x40
};

enum IntegerHeaderSize
{
    HEADER8_SIZE = 0x3,
    HEADER16_SIZE = 0x4,
    HEADER32_SIZE = 0x5,
    HEADER64_SIZE = 0x6
};

typedef signed char Int8;
typedef signed short int Int16;
typedef signed int Int32;
typedef signed long long int Int64;

typedef unsigned char UInt8;
typedef unsigned short int UInt16;
typedef unsigned int UInt32;
typedef unsigned long long int UInt64;

typedef double Double;
typedef UInt8 Boolean;

char GetIntHeaderSize(Int64 integer, UInt8 intsize, enum IntegerHeaderSize * out_size_enum);
char GetUIntHeaderSize(UInt64 uinteger, UInt8 intsize, enum IntegerHeaderSize * out_size_enum);

char CheckIntValue(Int64 integer, UInt8 intsize);
char CheckUIntValue(UInt64 uinteger, UInt8 intsize);
unsigned char CheckDoubleValue(Double value);
unsigned char CheckHeaderValue(UInt64 header, UInt8 hsize);

typedef struct UTF8String
{
    size_t length;
    char ** utf8_string;
}UTF8String;

char GetUTF8ByteLength(char ** _s, size_t * size_out);

typedef enum DataTypeEnum
{
    NULL_DTYPE = 0x0,
    UINT8_DTYPE = 0x1,
    UINT16_DTYPE = 0x2,
    UINT32_DTYPE = 0x3,
    UINT64_DTYPE = 0x4,
    INT8_DTYPE = 0x5,
    INT16_DTYPE = 0x6,
    INT32_DTYPE = 0x7,
    INT64_DTYPE = 0x8,
    DOUBLE_DTYPE = 0x9,
    UTF8_STRING_DTYPE = 0xa,
    BOOLEAN_DTYPE = 0xb
}DataTypeEnum;

typedef union DataTypeUnion
{
    UInt8 u8;
    UInt16 u16;
    UInt32 u32;
    UInt64 u64;
    Int8 i8;
    Int16 i16;
    Int32 i32;
    Int64 i64;
    Double d;
    UTF8String u8s;
    Boolean b;
}DataTypeUnion;

typedef struct UnicodeStringBucket
{
    size_t length;
    char * u8string;
} UnicodeStringBucket;

typedef struct Serializable
{
    DataTypeEnum data_type;
    DataTypeUnion primitive_data;
    UnicodeStringBucket unicode_str_data;
}Serializable;


#ifdef __cplusplus
}
#endif
#define _DBITS_DTYPES_H
#endif //_DBITS_DTYPES_H
