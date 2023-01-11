#ifndef _DBITS_DEQUE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "dbits/dtypes.h"

typedef enum DequeNodeValueTypeEnum
{
    DEQUE_BIT_VALUE = 0x1,
    DEQUE_UCHAR_VALUE = 0x2,
    DEQUE_SERIALIZABLE_VALUE = 0x3
}DequeNodeValueTypeEnum;

typedef union DequeNodeValueType
{
    unsigned char uchar_v;
    unsigned char bit_v : 1;
    Serializable serializable_v;
}DequeNodeValueType;

typedef struct DequeNode
{
    DequeNodeValueTypeEnum value_type;
    DequeNodeValueType value;
    struct DequeNode *prev_node;
    struct DequeNode *next_node;
} DequeNode;

typedef unsigned long int deque_size_t;

typedef struct Deque
{
    deque_size_t size;
    DequeNode *head_node;
    DequeNode *tail_node;
} Deque;

enum DequeInsertModeEnum
{
    DEQUE_INSERT_TAIL = 0x1,
    DEQUE_INSERT_HEAD = 0x2,
};

enum DequeRemoveModeEnum
{
    DEQUE_REMOVE_TAIL = 0x3,
    DEQUE_REMOVE_HEAD = 0x4
};


extern Deque * AllocateDeque(void);
extern void FreeDeque(Deque *deque_p);

char IsValidDeque(Deque *deque_p);

char FillDeque(Deque *deque_p, enum DequeInsertModeEnum mode, deque_size_t n_nodes, DequeNodeValueType value, DequeNodeValueTypeEnum typev);

char RemoveDequeNodes(Deque *deque_p, enum DequeRemoveModeEnum rmode, deque_size_t n_nodes);
char ClearDeque(Deque *deque_p);

char MergeDeques(Deque *out_deque_p, Deque *in_deque_p);

DequeNode * WalkDequeNode(Deque * deque_p, DequeNode ** dnode_p);

extern Deque * AllocateDeque(void);
extern void FreeDeque(Deque *deque_p);

char IsValidDeque(Deque *deque_p);

char FillDeque(Deque *deque_p, enum DequeInsertModeEnum mode, deque_size_t n_nodes, DequeNodeValueType value, DequeNodeValueTypeEnum typev);

char RemoveDequeNodes(Deque *deque_p, enum DequeRemoveModeEnum rmode, deque_size_t n_nodes);
char ClearDeque(Deque *deque_p);

char MergeDeques(Deque *out_deque_p, Deque *in_deque_p);

DequeNode * WalkDequeNode(Deque * deque_p, DequeNode ** dnode_p);

#ifdef __cplusplus
}
#endif
#define _DBITS_DEQUE_H
#endif // _DBITS_DEQUE_H
