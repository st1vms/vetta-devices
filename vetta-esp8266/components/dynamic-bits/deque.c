#include <stdlib.h>
#include "dbits/deque.h"

Deque * AllocateDeque(void)
{
    Deque * _res = (Deque *) malloc(sizeof(Deque));
    if(!_res)
    {
        return NULL;
    }
    _res->size = 0;
    _res->head_node = NULL;
    _res->tail_node = NULL;

    return _res;
}

void FreeDeque(Deque *deque_p)
{
    if (NULL != deque_p && NULL != deque_p->head_node)
    {
        DequeNode *tmp = deque_p->head_node;
        while (tmp != NULL)
        {
            deque_p->head_node = tmp->next_node;
            
            free(tmp);
            tmp = deque_p->head_node;
        }
        deque_p->tail_node = NULL;
        deque_p->size = 0;
    }
}

char IsValidDeque(Deque *deque_p)
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

static char InsertNodeHead(Deque *deque_p, DequeNodeValueType value, DequeNodeValueTypeEnum typev)
{
    DequeNode *d = (DequeNode *)malloc(sizeof(DequeNode));
    if (!d)
    {
        return 0;
    }

    d->value_type = typev;
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

static char InsertNodeTail(Deque *deque_p, DequeNodeValueType value, DequeNodeValueTypeEnum typev)
{

    DequeNode *dnode = (DequeNode *)malloc(sizeof(DequeNode));
    if (!dnode)
    {
        return 0;
    }

    dnode->value_type = typev;
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
        DequeNode *tmp = deque_p->tail_node;
        tmp->next_node = dnode;
        dnode->prev_node = tmp;
        deque_p->tail_node = dnode;
        deque_p->size++;
        return 1;
    }

    free(dnode);

    return 0;
}

char FillDeque(Deque *deque_p, enum DequeInsertModeEnum imode, deque_size_t n_nodes, DequeNodeValueType value, DequeNodeValueTypeEnum typev)
{
    if (!IsValidDeque(deque_p))
    {
        return 0;
    }


    switch (imode)
    {
    case DEQUE_INSERT_HEAD:
        for (deque_size_t i = 0; i < n_nodes; i++)
        {
            if (!InsertNodeHead(deque_p, value, typev))
            {
                return 0;
            }
        }
        return 1;
    case DEQUE_INSERT_TAIL:
        for (deque_size_t i = 0; i < n_nodes; i++)
        {
            if (!InsertNodeTail(deque_p, value, typev))
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

char RemoveDequeNodes(Deque *deque_p, enum DequeRemoveModeEnum rmode, deque_size_t n_nodes)
{
    if (!IsValidDeque(deque_p) || deque_p->size == 0)
    {
        return 0;
    }

    DequeNode *tmp = NULL;

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

char ClearDeque(Deque *deque_p)
{
    if (!deque_p)
    {
        return 0;
    }
    return RemoveDequeNodes(deque_p, (enum DequeRemoveModeEnum)DEQUE_REMOVE_TAIL, (deque_size_t)deque_p->size);
}

char MergeDeques(Deque *out_deque_p, Deque *in_deque_p)
{
    if (!IsValidDeque(out_deque_p) || !IsValidDeque(in_deque_p))
    {
        return 0;
    }

    if(in_deque_p->size == 0)
    {return 1;}

    deque_size_t i = 0;
    for (DequeNode *node = in_deque_p->head_node; node != NULL && i < in_deque_p->size; node = node->next_node, i++)
    {
        if (!InsertNodeTail(out_deque_p, node->value, node->value_type))
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

DequeNode * WalkDequeNode(Deque * deque_p, DequeNode ** dnode_p)
{
    if(!IsValidDeque(deque_p) || deque_p->size == 0)
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

