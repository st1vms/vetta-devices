#include <stdlib.h>
#include "dbits/sdeque.h"

SerializableDeque * AllocateSerializableDeque(void)
{
    SerializableDeque * _res = (SerializableDeque *) malloc(sizeof(SerializableDeque));
    if(!_res)
    {
        return NULL;
    }
    _res->size = 0;
    _res->head_node = NULL;
    _res->tail_node = NULL;

    return _res;
}

static void StringPointerDeallocationCheck(SerializableDequeNode * dnode)
{
    switch (dnode->value.data_type)
    {
    case UTF8_STRING_DTYPE:
        if(dnode->value.data_type == UTF8_STRING_DTYPE && NULL != dnode->value.unicode_str_data.u8string)
        {
            free(dnode->value.unicode_str_data.u8string);
            dnode->value.unicode_str_data.u8string = NULL;
        }
        break;
    default:
        break;
    }
}

void FreeSerializableDeque(SerializableDeque *deque_p, unsigned char deallocate_heap_strings)
{
    if (NULL != deque_p && NULL != deque_p->head_node)
    {
        SerializableDequeNode *tmp = deque_p->head_node;
        while (tmp != NULL)
        {
            deque_p->head_node = tmp->next_node;

            if(deallocate_heap_strings)
            {
                StringPointerDeallocationCheck(tmp);
            }
            free(tmp);
            tmp = deque_p->head_node;
        }
        deque_p->tail_node = NULL;
        deque_p->size = 0;
    }
}

char IsValidSerializableDeque(SerializableDeque *deque_p)
{
    if (!deque_p)
    {
        return 0;
    }
    else if (deque_p->size > 0 && (NULL == deque_p->head_node || NULL == deque_p->tail_node))
    {
        return 0;
    }
    else if (deque_p->size == 0 && (NULL != deque_p->head_node || NULL != deque_p->tail_node))
    {
        return 0;
    }

    return 1;
}

static char InsertNodeHead(SerializableDeque *deque_p, Serializable value)
{
    SerializableDequeNode *d = (SerializableDequeNode *)malloc(sizeof(SerializableDequeNode));
    if (!d)
    {
        return 0;
    }

    d->value = value;
    d->prev_node = NULL;
    d->next_node = deque_p->head_node;

    if (NULL == deque_p->head_node && deque_p->size == 0)
    {
        deque_p->head_node = d;
        deque_p->tail_node = deque_p->head_node;
        deque_p->size++;
        return 1;
    }
    else if (deque_p->size > 0)
    {
        deque_p->head_node->prev_node = d;
        deque_p->head_node = d;
        deque_p->size++;
        return 1;
    }

    free(d);

    return 0;
}

static char InsertNodeTail(SerializableDeque *deque_p, Serializable value)
{

    SerializableDequeNode *dnode = (SerializableDequeNode *)malloc(sizeof(SerializableDequeNode));
    if (!dnode)
    {
        return 0;
    }

    dnode->value = value;
    dnode->prev_node = NULL;
    dnode->next_node = NULL;

    if (NULL == deque_p->head_node && deque_p->size == 0)
    {
        deque_p->head_node = dnode;
        deque_p->tail_node = deque_p->head_node;
        deque_p->size++;
        return 1;
    }
    else if (deque_p->size > 0 && deque_p->tail_node != NULL)
    {
        SerializableDequeNode *tmp = deque_p->tail_node;
        tmp->next_node = dnode;
        dnode->prev_node = tmp;
        deque_p->tail_node = dnode;
        deque_p->size++;
        return 1;
    }

    free(dnode);

    return 0;
}

static char InsertStringNodeTail(SerializableDeque *deque_p, DataTypeEnum dtype, UnicodeStringBucket value)
{

    SerializableDequeNode *dnode = (SerializableDequeNode *)malloc(sizeof(SerializableDequeNode));
    if (!dnode)
    {
        return 0;
    }

    dnode->value.data_type = dtype;
    dnode->value.unicode_str_data.length = value.length;
    dnode->value.unicode_str_data.u8string = value.u8string;

    dnode->prev_node = NULL;
    dnode->next_node = NULL;

    if (NULL == deque_p->head_node && deque_p->size == 0)
    {
        deque_p->head_node = dnode;
        deque_p->tail_node = deque_p->head_node;
        deque_p->size++;
        return 1;
    }
    else if (deque_p->size > 0 && deque_p->tail_node != NULL)
    {
        SerializableDequeNode *tmp = deque_p->tail_node;
        tmp->next_node = dnode;
        dnode->prev_node = tmp;
        deque_p->tail_node = dnode;
        deque_p->size++;
        return 1;
    }

    free(dnode);

    return 0;
}

char FillSerializableDeque(SerializableDeque *deque_p, enum DequeInsertModeEnum imode, deque_size_t n_nodes, Serializable value)
{
    if (!IsValidSerializableDeque(deque_p))
    {
        return 0;
    }

    switch (imode)
    {
    case DEQUE_INSERT_HEAD:
        for (deque_size_t i = 0; i < n_nodes; i++)
        {
            if (!InsertNodeHead(deque_p, value))
            {
                return 0;
            }
        }
        return 1;
    case DEQUE_INSERT_TAIL:
        for (deque_size_t i = 0; i < n_nodes; i++)
        {
            if (!InsertNodeTail(deque_p, value))
            {
                return 0;
            }
        }
        return 1;
    default:
        break;
    }

    return 0;
}

char FillUnicodeSerializableDeque(SerializableDeque *deque_p, deque_size_t n_nodes, DataTypeEnum dtype, UnicodeStringBucket value)
{
    if (!IsValidSerializableDeque(deque_p))
    {
        return 0;
    }

    for (deque_size_t i = 0; i < n_nodes; i++)
    {
        if (!InsertStringNodeTail(deque_p, dtype, value))
        {
            return 0;
        }
    }

    return 1;
}

char RemoveSerializableDequeNodes(SerializableDeque *deque_p, enum DequeRemoveModeEnum rmode, deque_size_t n_nodes)
{
    if (!IsValidSerializableDeque(deque_p) || deque_p->size == 0)
    {
        return 0;
    }

    SerializableDequeNode *tmp = NULL;

    switch (rmode)
    {
    case DEQUE_REMOVE_HEAD:
        tmp = deque_p->head_node;

        for (deque_size_t i = 0; i < n_nodes && tmp != NULL; i++)
        {
            if (deque_p->size == 1)
            {
                free(deque_p->head_node);
                deque_p->head_node = NULL;
                deque_p->tail_node = NULL;
                deque_p->size--;
                return 1;
            }
            tmp->next_node->prev_node = NULL;
            deque_p->head_node = tmp->next_node;
            free(tmp);
            tmp = deque_p->head_node;
            deque_p->size--;
        }

        deque_p->tail_node = deque_p->size > 0 ? (deque_p->size == 1 ? deque_p->head_node : deque_p->tail_node) : NULL;
        return 1;
    case DEQUE_REMOVE_TAIL:
        tmp = deque_p->tail_node;

        for (deque_size_t i = 0; i < n_nodes && tmp != NULL; i++)
        {
            if (deque_p->size == 1)
            {
                free(deque_p->tail_node);
                deque_p->head_node = NULL;
                deque_p->tail_node = NULL;
                deque_p->size--;
                return 1;
            }
            tmp->prev_node->next_node = NULL;
            deque_p->tail_node = tmp->prev_node;
            free(tmp);
            tmp = deque_p->tail_node;
            deque_p->size--;
        }
        deque_p->head_node = deque_p->size > 0 ? (deque_p->size == 1 ? deque_p->tail_node : deque_p->head_node) : NULL;
        return 1;
    default:
        break;
    }

    return 0;
}

char ClearSerializableDeque(SerializableDeque *deque_p)
{
    if (!deque_p)
    {
        return 0;
    }
    return RemoveSerializableDequeNodes(deque_p, (enum DequeRemoveModeEnum)DEQUE_REMOVE_TAIL, (deque_size_t)deque_p->size);
}

char MergeSerializableDeques(SerializableDeque *out_deque_p, SerializableDeque *in_deque_p)
{
    if (!IsValidSerializableDeque(out_deque_p) || !IsValidSerializableDeque(in_deque_p))
    {
        return 0;
    }

    if(in_deque_p->size == 0)
    {return 1;}

    deque_size_t i = 0;
    for (SerializableDequeNode *node = in_deque_p->head_node; node != NULL && i < in_deque_p->size; node = node->next_node, i++)
    {
        if (!InsertNodeTail(out_deque_p, node->value))
        {
            return 0;
        }
    }

    if (i != in_deque_p->size)
    {
        return 0;
    }

    return 1;
}

SerializableDequeNode * WalkSerializableDequeNode(SerializableDeque * deque_p, SerializableDequeNode ** dnode_p)
{
    if(!IsValidSerializableDeque(deque_p) || deque_p->size == 0)
    {
        return NULL;
    }

    if (dnode_p == NULL && deque_p->head_node != NULL)
    {
        return deque_p->head_node;
    }
    else if ((*dnode_p)->next_node != NULL){
        return (*dnode_p)->next_node;
    }
    return NULL;
}
