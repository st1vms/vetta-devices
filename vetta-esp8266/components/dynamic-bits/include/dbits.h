#ifndef _DBITS_CORE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "dbits/dtypes.h"
#include "dbits/deque.h"
#include "dbits/sdeque.h"
#include "dbits/bitconv.h"

typedef struct ByteBuffer
{
    unsigned char * buffer_p;
    size_t buffer_size;
}ByteBuffer;


extern char IsEmptyByteBuffer(ByteBuffer* buffer);
extern ByteBuffer* NewByteBuffer(void);
extern void FreeByteBuffer(ByteBuffer* buffer);

extern char SerializeDataF(ByteBuffer* buffer, const char *format, ...);
extern char DeserializeBufferF(const unsigned char * uchar_buffer_p, size_t buffer_size, const char *format, ...);

extern char AddUTF8Serializable(SerializableDeque* sdeque, char** _str, size_t slength);

extern char AddSerializable(SerializableDeque* sdeque, Serializable s);
extern char SerializeDataDeque(ByteBuffer* buffer, SerializableDeque * sdeque);
extern char DeserializeBufferToDataDeque(unsigned char * uchar_buffer_p, size_t buffer_size, const int * template_enum, size_t template_size, SerializableDeque *data_deque_pool);

#ifdef __cplusplus
}
#endif
#define _DBITS_CORE_H
#endif // _DBITS_CORE_H
