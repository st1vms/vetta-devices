#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include "dbits.h"

// BUFFER FUNCTIONS


static char IsValidByteBuffer(ByteBuffer* buffer)
{
    if (!buffer || (NULL == buffer->buffer_p && buffer->buffer_size > 0) || (NULL != buffer->buffer_p && buffer->buffer_size == 0))
    {
        return 0;
    }
    return 1;
}

char IsEmptyByteBuffer(ByteBuffer* buffer)
{
    if(IsValidByteBuffer(buffer) && buffer->buffer_size > 0)
    {
        return 0;
    }
    return 1;
}

static void DoFreeByteBuffer(ByteBuffer * buffer)
{
    if(buffer != NULL)
    {
        if(buffer->buffer_p != NULL)
        {
            free(buffer->buffer_p);
            buffer->buffer_p = NULL;
        }
        buffer->buffer_p = NULL;
        buffer->buffer_size = 0;
        free(buffer);
    }
    buffer = NULL;
}

void FreeByteBuffer(ByteBuffer * buffer)
{
    DoFreeByteBuffer(buffer);
}

static ByteBuffer* AllocateByteBuffer(void)
{
    ByteBuffer * _res = (ByteBuffer *) malloc(sizeof(ByteBuffer));
    if(!_res)
    {
        return NULL;
    }
    _res->buffer_p = NULL;
    _res->buffer_size = 0;

    return _res;
}

ByteBuffer* NewByteBuffer(void)
{
    return AllocateByteBuffer();
}

static char BitsToByteBuffer(Deque *bits, ByteBuffer* buffer)
{
    if (!IsValidByteBuffer(buffer) || buffer->buffer_size != 0 || !IsValidDeque(bits) || bits->size == 0)
    {
        return 0;
    }

    size_t padding_bitsize = ((UInt8)UINT8_SIZE - ((bits->size + HEADER8_SIZE) % (UInt8)UINT8_SIZE)) % (UInt8)UINT8_SIZE;

    Deque* padded = AllocateDeque();
    if (!SizeHeaderToBits(padding_bitsize + 1, HEADER8_SIZE, padded) || !MergeDeques(padded, bits) || (padding_bitsize > 0 && !AddBit(padded, DEQUE_INSERT_TAIL, padding_bitsize, BIT_ZERO)))
    {
        FreeDeque(padded);
        return 0;
    }

    Deque* new_buffer = AllocateDeque();
    DequeNode *node = padded->head_node;

    size_t bytesize = (padded->size) >> HEADER8_SIZE;
    UInt8 tmp = 0;
    size_t i = 0;

    while (i < bytesize)
    {
        tmp = 0;
        for (UInt8 c = 0; node != NULL && c < UINT8_SIZE; c++)
        {
            tmp += node->value.bit_v ? (1 << c) : 0;
            node = node->next_node;
        }

        if (!FillDeque(new_buffer, DEQUE_INSERT_TAIL, 1, (DequeNodeValueType) {.uchar_v = tmp}, DEQUE_UCHAR_VALUE))
        {
            FreeDeque(padded);
            FreeDeque(new_buffer);
            return 0;
        }
        i++;
    }


    if (i != bytesize)
    {
        FreeDeque(padded);
        FreeDeque(new_buffer);
        return 0;
    }

    FreeDeque(padded);

    buffer->buffer_p = (unsigned char *)calloc(new_buffer->size, sizeof(unsigned char));
    if (NULL == buffer->buffer_p)
    {
        FreeDeque(new_buffer);
        return 0;
    }
    unsigned char *bufp = buffer->buffer_p;

    DequeNode *unit = new_buffer->head_node;
    i = 0;
    while (bufp != NULL && unit != NULL && i < new_buffer->size)
    {
        *bufp = unit->value.uchar_v;
        unit = unit->next_node;
        i++;
        bufp++;
    }

    if (i != new_buffer->size)
    {
        FreeByteBuffer(buffer);
        FreeDeque(new_buffer);
        return 0;
    }

    FreeDeque(new_buffer);

    buffer->buffer_size = i;
    return 1;
}

static char ByteBufferToBits(unsigned char * buffer, size_t buffer_size, Deque *out_bits)
{
    if (!buffer || buffer_size == 0 || !IsValidDeque(out_bits))
    {
        return 0;
    }

    Deque* bits = AllocateDeque();
    unsigned char * tmp = buffer;
    UInt64 c = 0;
    UInt8 i = 0;
    while (c < buffer_size && tmp != NULL)
    {
        i = 0;

        while (tmp != NULL && *tmp != '\0' && i < UINT8_SIZE)
        {
            if(*tmp & 1)
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

            *tmp >>= 1;
            i++;
        }

        if (UINT8_SIZE - i > 0 && !AddBit(bits, DEQUE_INSERT_TAIL, UINT8_SIZE - i, BIT_ZERO))
        {
            FreeDeque(bits);
            return 0;
        }

        c++;
        tmp++;
    }

    UInt8 pad = 0;
    DequeNode *node = bits->head_node;

    deque_size_t offset = 0;
    if (!(offset = BitsToSizeHeader(bits, &node, offset, HEADER8_SIZE, &pad)) || !RemoveDequeNodes(bits, DEQUE_REMOVE_HEAD, HEADER8_SIZE))
    {
        FreeDeque(bits);
        return 0;
    }

    pad -= 1;

    if (pad > 0)
    {
        if (pad >= UINT8_SIZE || !RemoveDequeNodes(bits, DEQUE_REMOVE_TAIL, pad))
        {
            FreeDeque(bits);
            return 0;
        }
    }

    if (!MergeDeques(out_bits, bits))
    {
        FreeDeque(bits);
        return 0;
    }

    FreeDeque(bits);
    return 1;
}

static enum DataTypeEnum ParseFormatVar(char **format_p)
{
    if (!format_p || NULL == *format_p || !(strlen(*format_p)))
    {
        return NULL_DTYPE;
    }

    switch (**format_p)
    {
    case 'u':
        (*format_p)++;
        if (!format_p || NULL == *format_p)
        {
            return NULL_DTYPE;
        }

        switch (**format_p)
        {
        case '8':
            (*format_p)++;
            return UINT8_DTYPE;
        case '1':
            (*format_p)++;
            if (!format_p || NULL == *format_p || **format_p != '6')
            {
                return NULL_DTYPE;
            }
            (*format_p)++;
            return UINT16_DTYPE;
        case '3':
            (*format_p)++;
            if (!format_p || NULL == *format_p || **format_p != '2')
            {
                return NULL_DTYPE;
            }
            (*format_p)++;
            return UINT32_DTYPE;
        case '6':
            (*format_p)++;
            if (!format_p || NULL == *format_p || **format_p != '4')
            {
                return NULL_DTYPE;
            }
            (*format_p)++;
            return UINT64_DTYPE;
        default:
            break;
        }
        break;
    case 'i':
        (*format_p)++;
        if (!format_p || NULL == *format_p)
        {
            return NULL_DTYPE;
        }

        switch (**format_p)
        {
        case '8':
            (*format_p)++;
            return INT8_DTYPE;
        case '1':
            (*format_p)++;
            if (!format_p || NULL == *format_p || **format_p != '6')
            {
                return NULL_DTYPE;
            }
            (*format_p)++;
            return INT16_DTYPE;
        case '3':
            (*format_p)++;
            if (!format_p || NULL == *format_p || **format_p != '2')
            {
                return NULL_DTYPE;
            }
            (*format_p)++;
            return INT32_DTYPE;
        case '6':
            (*format_p)++;
            if (!format_p || NULL == *format_p || **format_p != '4')
            {
                return NULL_DTYPE;
            }
            (*format_p)++;
            return INT64_DTYPE;
        default:
            break;
        }
        break;
    case 'd':
        (*format_p)++;
        return DOUBLE_DTYPE;
    case 's':
        (*format_p)++;
        return UTF8_STRING_DTYPE;

    case 'b':
        (*format_p)++;
        return BOOLEAN_DTYPE;
    default:
        break;
    }

    return NULL_DTYPE;
}

char DoSerializeData(ByteBuffer *buffer, const char *format, va_list args)
{

    DataTypeEnum dtype;
    union DataTypeUnion dunion;
    UnicodeStringBucket unicode_bucket = (UnicodeStringBucket) {.u8string = NULL, .length = 0};

    Deque* bits = AllocateDeque();

    while (*format != '\0')
    {
        if (!(dtype = ParseFormatVar((char **)&format)))
        {

            FreeDeque(bits);
            return 0;
        }

        switch (dtype)
        {
        case INT8_DTYPE:
            dunion.i64 = (Int64)va_arg(args, Int32);
            if (!IntToBits(dunion.i64, INT8_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case INT16_DTYPE:
            dunion.i64 = (Int64)va_arg(args, Int32);
            if (!IntToBits(dunion.i64, INT16_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case INT32_DTYPE:
            dunion.i64 = (Int64)va_arg(args, Int32);
            if (!IntToBits(dunion.i64, INT32_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case INT64_DTYPE:
            dunion.i64 = (Int64)va_arg(args, Int64);
            if (!IntToBits(dunion.i64, INT64_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case UINT8_DTYPE:
            dunion.u64 = (UInt64)va_arg(args, UInt32);
            if (!UIntToBits(dunion.u64, UINT8_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case UINT16_DTYPE:
            dunion.u64 = (UInt64)va_arg(args, UInt32);
            if (!UIntToBits(dunion.u64, UINT16_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }

            break;
        case UINT32_DTYPE:
            dunion.u64 = (UInt64)va_arg(args, UInt32);
            if (!UIntToBits(dunion.u64, UINT32_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case UINT64_DTYPE:
            dunion.u64 = (UInt64)va_arg(args, UInt64);
            if (!UIntToBits(dunion.u64, UINT64_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case DOUBLE_DTYPE:
            dunion.d = (Double)va_arg(args, Double);
            if (!DoubleToBits(dunion.d, bits))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case UTF8_STRING_DTYPE:
            unicode_bucket.u8string = (char *) va_arg(args, char*);
            if (unicode_bucket.u8string == NULL || !GetUTF8ByteLength(&(unicode_bucket.u8string), &(unicode_bucket.length)) || !UTF8StringToBits(&(unicode_bucket.u8string), unicode_bucket.length, bits))
            {
                FreeDeque(bits);
                return 0;
            }
            break;
        case BOOLEAN_DTYPE:
            dunion.b = (Boolean)va_arg(args, UInt32);
            if (!BooleanToBit(dunion.b, bits))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        default:

            FreeDeque(bits);
            return 0;
        }
    }

    if (bits->size == 0 || !BitsToByteBuffer(bits, buffer))
    {
        FreeDeque(bits);
        buffer = NULL;
        return 0;
    }

    FreeDeque(bits);
    return 1;
}

char SerializeDataF(ByteBuffer* buffer, const char *format, ...)
{
    if(!IsValidByteBuffer(buffer) || !format || !(strlen(format)))
    {
        return 0;
    }

    va_list args;
    va_start(args, format);
    char c = DoSerializeData(buffer, format, args);
    va_end(args);

    return c;
}

char DoDeserializeBuffer(ByteBuffer * buffer, const char *format, va_list args)
{
    union VargUnion
    {
        intptr_t iptr;
        uintptr_t uptr;
        Double * dptr;
    } varg;

    Deque* bitdeque = AllocateDeque();
    if (!ByteBufferToBits(buffer->buffer_p, buffer->buffer_size, bitdeque))
    {
        FreeDeque(bitdeque);
        return 0;
    }

    DataTypeEnum dtype;

    union DataTypeUnion dunion;

    DequeNode *node = bitdeque->head_node;
    deque_size_t bitoffset = 0;
    while (*format != '\0')
    {
        if (!(dtype = ParseFormatVar((char **)&format)))
        {
            FreeDeque(bitdeque);
            return 0;
        }

        switch (dtype)
        {
        case INT8_DTYPE:
            varg.iptr = (intptr_t)va_arg(args, intptr_t);
            if (NULL == ((Int8*) (varg.iptr)) || (bitoffset = BitsToInt8(bitdeque, &node, bitoffset, &(dunion.i8), 1)))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            *((Int8*) (varg.iptr)) = dunion.i8;
            break;
        case INT16_DTYPE:
            varg.iptr = (intptr_t)va_arg(args, intptr_t);
            if (NULL == ((Int16*) (varg.iptr)) || !(bitoffset = BitsToInt16(bitdeque, &node, bitoffset, &(dunion.i16), 1)))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            *((Int16*) (varg.iptr)) = dunion.i16;
            break;
        case INT32_DTYPE:
            varg.iptr = (intptr_t)va_arg(args, intptr_t);
            if (NULL == ((Int32*) (varg.iptr)) || !(bitoffset = BitsToInt32(bitdeque, &node, bitoffset, &(dunion.i32), 1)))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            *((Int32*) (varg.iptr)) = dunion.i32;
            break;
        case INT64_DTYPE:
            varg.iptr = (intptr_t)va_arg(args, intptr_t);
            if (NULL == ((Int64*) (varg.iptr)) || !(bitoffset = BitsToInt64(bitdeque, &node, bitoffset, &(dunion.i64), 1)))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            *((Int64*) (varg.iptr)) = dunion.i64;
            break;
        case UINT8_DTYPE:
            varg.uptr = (uintptr_t)va_arg(args, uintptr_t);
            if (NULL == ((UInt8*) (varg.uptr)) || !(bitoffset = BitsToUInt8(bitdeque, &node, bitoffset, &(dunion.u8), 1)))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            *((UInt8*) (varg.uptr)) = dunion.u8;
            break;
        case UINT16_DTYPE:
            varg.uptr = (uintptr_t)va_arg(args, uintptr_t);
            if (NULL == ((UInt16*) (varg.uptr)) || !(bitoffset = BitsToUInt16(bitdeque, &node, bitoffset, &(dunion.u16), 1)))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            *((UInt16*) (varg.uptr)) = dunion.u16;
            break;
        case UINT32_DTYPE:
            varg.uptr = (uintptr_t)va_arg(args, uintptr_t);
            if (NULL == ((UInt32*) (varg.uptr)) || !(bitoffset = BitsToUInt32(bitdeque, &node, bitoffset, &(dunion.u32), 1)))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            *((UInt32*) (varg.uptr)) = dunion.u32;
            break;
        case UINT64_DTYPE:
            varg.uptr = (uintptr_t)va_arg(args, uintptr_t);
            if (NULL == ((UInt64*) (varg.uptr)) || !(bitoffset = BitsToUInt64(bitdeque, &node, bitoffset, &(dunion.u64), 1)))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            *((UInt64*) (varg.uptr)) = dunion.u64;
            break;
        case DOUBLE_DTYPE:
            varg.dptr = (Double*)va_arg(args, Double*);
            if (NULL == ((Double*) (varg.dptr)) || !(bitoffset = BitsToDouble(bitdeque, &node, bitoffset, &(dunion.d))))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            *((Double*) (varg.dptr)) = dunion.d;
            break;
        case BOOLEAN_DTYPE:
            varg.uptr = (uintptr_t)va_arg(args, uintptr_t);
            if (NULL == ((Boolean*) (varg.uptr)) || !(bitoffset = BitToBoolean(bitdeque, &node, bitoffset, &(dunion.b))))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            *((Boolean*) (varg.uptr)) = dunion.b;
            break;
        default:
            FreeDeque(bitdeque);
            return 0;
        }
    }

    FreeDeque(bitdeque);
    return 1;
}

char DeserializeBufferF(const unsigned char * uchar_buffer_p, size_t buffer_size, const char *format, ...)
{
    if(!uchar_buffer_p || buffer_size == 0 || !format || !(strlen(format)))
    {
        return 0;
    }

    ByteBuffer * _b = NewByteBuffer();
    if(!_b)
    {return 0;}

    _b->buffer_p = (unsigned char *) uchar_buffer_p;
    _b->buffer_size = buffer_size;

    va_list args;
    va_start(args, format);
    char c = DoDeserializeBuffer(_b, format, args);
    va_end(args);

    if(_b)
    {
        DoFreeByteBuffer(_b);
    }

    return c;
}

char AddUTF8Serializable(SerializableDeque* sdeque, char ** _str, size_t slength)
{
    if(!IsValidSerializableDeque(sdeque) || !_str || !(*_str))
    {
        return 0;
    }

    if(!FillUnicodeSerializableDeque(sdeque, 1, UTF8_STRING_DTYPE, (UnicodeStringBucket) {.u8string = *_str, .length = slength}))
    {
        return 0;
    }
    return 1;
}

char AddSerializable(SerializableDeque* sdeque, Serializable s)
{
    if(!IsValidSerializableDeque(sdeque) ||
        s.data_type <= NULL_DTYPE ||
        s.data_type > BOOLEAN_DTYPE)
    {
        return 0;
    }

    switch(s.data_type)
    {
        case UINT8_DTYPE:
            if(!CheckUIntValue(s.primitive_data.u8, UINT8_SIZE))
            {return 0;}
            break;
        case UINT16_DTYPE:
            if(!CheckUIntValue(s.primitive_data.u16, UINT16_SIZE))
            {return 0;}
            break;
        case UINT32_DTYPE:
            if(!CheckUIntValue(s.primitive_data.u32, UINT32_SIZE))
            {return 0;}
            break;
        case UINT64_DTYPE:
            if(!CheckUIntValue(s.primitive_data.u64, UINT64_SIZE))
            {return 0;}
            break;
        case INT8_DTYPE:
            if(!CheckIntValue(s.primitive_data.i8, INT8_SIZE))
            {return 0;}
            break;
        case INT16_DTYPE:
            if(!CheckIntValue(s.primitive_data.i16, INT16_SIZE))
            {return 0;}
            break;
        case INT32_DTYPE:
            if(!CheckIntValue(s.primitive_data.i32, INT32_SIZE))
            {return 0;}
            break;
        case INT64_DTYPE:
            if(!CheckIntValue(s.primitive_data.i64, INT64_SIZE))
            {return 0;}
            break;
        case DOUBLE_DTYPE:
            if(!CheckDoubleValue(s.primitive_data.d))
            {return 0;}
            break;
        case UTF8_STRING_DTYPE:
            if(NULL == s.unicode_str_data.u8string)
            {
                return 0;
            }else if(*(s.unicode_str_data.u8string) == 0 && s.unicode_str_data.length != 0)
            {
                return 0;
            }else  if(!FillUnicodeSerializableDeque(sdeque, 1, UTF8_STRING_DTYPE,
                (UnicodeStringBucket){
                .u8string = s.unicode_str_data.u8string,
                .length = s.unicode_str_data.length,
                }))
            {
                return 0;
            }
            return 1;
        case BOOLEAN_DTYPE:
            if(!CheckUIntValue(s.primitive_data.u8, UINT8_SIZE))
            {return 0;}
            break;
        default:
            return 0;
    }

    if(!FillSerializableDeque(sdeque, DEQUE_INSERT_TAIL, 1, s))
    {
        return 0;
    }

    return 1;
}



char SerializeDataDeque(ByteBuffer *buffer, SerializableDeque * sdeque)
{
    if(!IsValidSerializableDeque(sdeque) || sdeque->size == 0 || !IsValidByteBuffer(buffer) || buffer->buffer_size > 0)
    {
        return 0;
    }

    Deque* bits = AllocateDeque();
    SerializableDequeNode * node = sdeque->head_node;

    deque_size_t i = 0;
    while (node != NULL && i < sdeque->size)
    {
        switch (node->value.data_type)
        {
        case INT8_DTYPE:
            if (!IntToBits(node->value.primitive_data.i8, INT8_SIZE, bits, 1))
            {
                FreeDeque(bits);
                return 0;
            }
            break;
        case INT16_DTYPE:
            if (!IntToBits(node->value.primitive_data.i16, INT16_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case INT32_DTYPE:
            if (!IntToBits(node->value.primitive_data.i32, INT32_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case INT64_DTYPE:
            if (!IntToBits(node->value.primitive_data.i64, INT64_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case UINT8_DTYPE:
            if (!UIntToBits(node->value.primitive_data.u8, UINT8_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case UINT16_DTYPE:
            if (!UIntToBits(node->value.primitive_data.u16, UINT16_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }

            break;
        case UINT32_DTYPE:
            if (!UIntToBits(node->value.primitive_data.u32, UINT32_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case UINT64_DTYPE:
            if (!UIntToBits(node->value.primitive_data.u64, UINT64_SIZE, bits, 1))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case DOUBLE_DTYPE:
            if (!DoubleToBits(node->value.primitive_data.d, bits))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        case UTF8_STRING_DTYPE:
            if(!UTF8StringToBits(&(node->value.unicode_str_data.u8string), node->value.unicode_str_data.length, bits))
            {
                FreeDeque(bits);
                return 0;
            }
            break;
        case BOOLEAN_DTYPE:
            if (!BooleanToBit(node->value.primitive_data.b, bits))
            {

                FreeDeque(bits);
                return 0;
            }
            break;
        default:
            FreeDeque(bits);
            return 0;
        }
        node = node->next_node;
        i++;
    }

    if (i != sdeque->size || bits->size == 0 || !BitsToByteBuffer(bits, buffer))
    {
        FreeDeque(bits);
        buffer = NULL;
        return 0;
    }

    FreeDeque(bits);
    return 1;
}

char DeserializeBufferToDataDeque(unsigned char * uchar_buffer_p, size_t buffer_size, const int * template_enum, size_t template_size, SerializableDeque *data_deque_pool)
{
    if(!uchar_buffer_p || buffer_size == 0 || !template_enum || template_size == 0 || !IsValidSerializableDeque(data_deque_pool))
    {
        return 0;
    }

    Deque *bitdeque = AllocateDeque();
    if (!bitdeque)
    {
        return 0;
    }

    if(!ByteBufferToBits(uchar_buffer_p, buffer_size, bitdeque))
    {
        FreeDeque(bitdeque);
        return 0;
    }

    DequeNode * bnode = bitdeque->head_node;

    int dtype = NULL_DTYPE;

    DataTypeUnion dvalue = (DataTypeUnion) {.i8 = dvalue.i8};
    UnicodeStringBucket unicode_bucket = {.length = 0, .u8string = NULL};

    Serializable s = (Serializable){
        .unicode_str_data = unicode_bucket,
        .data_type = NULL_DTYPE,
        .primitive_data = dvalue
    };

    for(deque_size_t bitoffset = 0; template_size > 0 && template_enum != NULL; template_size--)
    {
        dtype = *(template_enum++);
        if(dtype <= NULL_DTYPE || dtype > BOOLEAN_DTYPE)
        {
            FreeDeque(bitdeque);
            return 0;
        }

        s.data_type = dtype;
        switch ((DataTypeEnum)dtype)
        {
        case INT8_DTYPE:
            if (!(bitoffset = BitsToInt8(bitdeque, &bnode, bitoffset, &(s.primitive_data.i8),1)) ||
                !AddSerializable(data_deque_pool, s))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            break;
        case INT16_DTYPE:
            if (!(bitoffset = BitsToInt16(bitdeque, &bnode, bitoffset, &(s.primitive_data.i16),1)) ||
                !AddSerializable(data_deque_pool, s))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            break;
        case INT32_DTYPE:
            if (!(bitoffset = BitsToInt32(bitdeque, &bnode, bitoffset, &(s.primitive_data.i32),1)) ||
                !AddSerializable(data_deque_pool, s))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            break;
        case INT64_DTYPE:
            if (!(bitoffset = BitsToInt64(bitdeque, &bnode, bitoffset, &(s.primitive_data.i64),1)) ||
                !AddSerializable(data_deque_pool, s))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            break;
        case UINT8_DTYPE:
            if (!(bitoffset = BitsToUInt8(bitdeque, &bnode, bitoffset, &(s.primitive_data.u8),1)) ||
                !AddSerializable(data_deque_pool, s))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            break;
        case UINT16_DTYPE:
            if (!(bitoffset = BitsToUInt16(bitdeque, &bnode, bitoffset, &(s.primitive_data.u16),1)) ||
                !AddSerializable(data_deque_pool, s))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            break;
        case UINT32_DTYPE:
            if (!(bitoffset = BitsToUInt32(bitdeque, &bnode, bitoffset, &(s.primitive_data.u32),1)) ||
                 !AddSerializable(data_deque_pool, s))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            break;
        case UINT64_DTYPE:
            if (!(bitoffset = BitsToUInt64(bitdeque, &bnode, bitoffset, &(s.primitive_data.u64),1)) ||
                 !AddSerializable(data_deque_pool, s))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            break;
        case DOUBLE_DTYPE:
            if (!(bitoffset = BitsToDouble(bitdeque, &bnode, bitoffset, &(s.primitive_data.d))) ||
                 !AddSerializable(data_deque_pool, s))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            break;
        case UTF8_STRING_DTYPE:
            if (!(bitoffset = BitsToUTF8String(bitdeque, &bnode, bitoffset, &(s.unicode_str_data))) ||
                !AddSerializable(data_deque_pool, s))
            {
                FreeDeque(bitdeque);
                return 0;
            }

            break;
        case BOOLEAN_DTYPE:
            if (!(bitoffset = BitToBoolean(bitdeque, &bnode, bitoffset, &(s.primitive_data.b))) ||
                 !AddSerializable(data_deque_pool, s))
            {
                FreeDeque(bitdeque);
                return 0;
            }
            break;
        default:
            FreeDeque(bitdeque);
            return 0;
        }

    }

    FreeDeque(bitdeque);

    return 1;
}
