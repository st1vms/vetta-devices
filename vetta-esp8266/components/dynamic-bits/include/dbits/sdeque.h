#ifndef _DBITS_SDEQUE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "dbits/deque.h"

typedef struct SerializableDequeNode
{
    Serializable value;
    struct SerializableDequeNode *prev_node;
    struct SerializableDequeNode *next_node;
} SerializableDequeNode;

typedef struct SerializableDeque
{
    deque_size_t size;
    SerializableDequeNode *head_node;
    SerializableDequeNode *tail_node;
} SerializableDeque;

extern SerializableDeque * AllocateSerializableDeque(void);
extern void FreeSerializableDeque(SerializableDeque *deque_p, unsigned char deallocate_heap_strings);

char IsValidSerializableDeque(SerializableDeque *deque_p);

char FillSerializableDeque(SerializableDeque *deque_p, enum DequeInsertModeEnum mode, deque_size_t n_nodes, Serializable value);
char FillUnicodeSerializableDeque(SerializableDeque *deque_p, deque_size_t n_nodes, DataTypeEnum dtype, UnicodeStringBucket value);

char RemoveSerializableDequeNodes(SerializableDeque *deque_p, enum DequeRemoveModeEnum rmode, deque_size_t n_nodes);
char ClearSerializableDeque(SerializableDeque *deque_p);

char MergeSerializableDeques(SerializableDeque *out_deque_p, SerializableDeque *in_deque_p);

SerializableDequeNode * WalkSerializableDequeNode(SerializableDeque * deque_p, SerializableDequeNode ** dnode_p);

#ifdef __cplusplus
}
#endif
#define _DBITS_SDEQUE_H
#endif // _DBITS_SDEQUE_H
