/******************************************************************************
 * Implementation of a leaky queue.
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 * Author: Brian T. Toombs
 * Date:   08/04/2014
 *****************************************************************************/

#ifndef __PJ_LEAKY_QUEUE_H__
#define __PJ_LEAKY_QUEUE_H__

#include <pj/list.h>

PJ_BEGIN_DECL

/**
 * The leaky queue is implemented as a pj_list.  When adding a new node to the queue, if the queue's max_size
 * would be exceeded, the oldest node will be removed and returned.
 */

struct pj_leaky_queue
{
    pj_list list;
    pj_list_type *front_node;
    pj_uint32_t max_size;
    pj_uint32_t cur_size;
};


/**
 * Initialize the queue.
 * Initially, the queue will have no members, and function pj_leaky_queue_empty() will
 * always return nonzero (which indicates TRUE) for the newly initialized list.
 *
 * @param queue     The queue head.
 * @param max_size  The maximum number of nodes (excluding the head node) allowed in the queue.
 */
PJ_INLINE(void) pj_leaky_queue_init(pj_leaky_queue *queue, pj_uint32_t max_size)
{
    queue->front_node = NULL;
    queue->max_size = max_size;
    queue->cur_size = 0;
    pj_list_init(&queue->list);
}

/**
 * Add a node to the back of the queue.
 * If the queue is full, remove the oldest node (the one at the front),
 * add the new node, and return the one that was removed.
 *
 * @param queue     The queue head.
 * @param node      The node to add.
 *
 * @return          The removed node if the queue was full before we started this operation, NULL otherwise.
 */
PJ_INLINE(pj_list_type *) pj_leaky_queue_push_back(pj_leaky_queue *queue, pj_list_type *node)
{
    pj_list_type *old_node = NULL;
    if (queue->cur_size == queue->max_size)
    {
        old_node = queue->front_node;
        if (queue->max_size == 1)
        {   /* Size of 1 is a special case because front_node->next points to the queue, not to another node */
            queue->front_node = node;
        }
        else
        {
            queue->front_node = ((pj_list *)queue->front_node)->next;
        }
        pj_list_erase(old_node);
    }
    else
    {
        if (queue->front_node == NULL)
        {
            queue->front_node = node;
        }
        ++queue->cur_size;
    }

    pj_list_push_back(&queue->list, node);

    return old_node;
}

/**
 * Check whether the queue is empty.
 *
 * @param queue     The queue head.
 *
 * @return          Non-zero if the queue is empty, or zero if it is not empty.
 */
PJ_INLINE(int) pj_leaky_queue_empty(const pj_leaky_queue *queue)
{
    return (queue == NULL || (queue != NULL && pj_list_empty(&queue->list)));
}

/**
 * Get the first node in the queue.
 *
 * @param queue     The queue head.
 *
 * @return          Pointer to the first node in the queue, or NULL if empty.
 */
PJ_INLINE(pj_list_type *) pj_leaky_queue_front(const pj_leaky_queue *queue)
{
    return pj_leaky_queue_empty(queue) ? NULL : queue->front_node;
}

/**
 * Get the last node in the queue.
 *
 * @param queue     The queue head.
 *
 * @return          Pointer to the last node in the queue, or NULL if empty.
 */
PJ_INLINE(pj_list_type *) pj_leaky_queue_back(const pj_leaky_queue *queue)
{
    if (pj_leaky_queue_empty(queue))
        return NULL;

    if (queue->cur_size == 1)
        return queue->front_node;

    /* The front_node->prev pointer points to the list.  The list's prev ptr
     * points to the last node in the list, which is what we want */

    return ((pj_list *)((pj_list *)queue->front_node)->prev)->prev;
}

/**
 * Remove the first node in the queue.
 *
 * @param queue     The queue head.
 *
 * @return          Pointer to the first node in the queue, or NULL if empty.
 */
PJ_INLINE(pj_list_type *) pj_leaky_queue_pop_front(pj_leaky_queue *queue)
{
    pj_list_type *node = NULL;

    if (pj_leaky_queue_empty(queue))
        return NULL;

    node = queue->front_node;
    --queue->cur_size;
    if (queue->cur_size > 0)
    {
        queue->front_node = ((pj_list *)node)->next;
    }
    else
    {
        queue->front_node = NULL;
    }
    pj_list_erase(node);

    return node;
}

/**
 * Find and return the first node that does NOT match the given value.
 * The function iterates the nodes in the queue, starting at the front of the queue, and calls
 * the user supplied comparison function until the comparison function returns something other
 * than 0.
 *
 * @param queue     The queue head.
 * @param value     The value used for comparison, to be passed to the comparison function.
 * @param comp      The comparison function, which should return ZERO to indicate that the value searched is found
 *
 * @return          Pointer to the first non-matching node, or NULL if all nodes match or if the queue is empty.
 */
PJ_INLINE(pj_list_type *) pj_leaky_queue_first_non_matching(pj_leaky_queue *queue, const void *value, int (*comp)(const void *value, const pj_list_type *node))
{
    if (pj_leaky_queue_empty(queue))
        return NULL;
    else
    {
        pj_list *p = (pj_list *) queue->list.next;
        while (p != &queue->list && (*comp)(value, p) == 0)
            p = (pj_list *) p->next;

        return p == &queue->list ? NULL : p;
    }
}

/**
 * Find and return the first node that has a value greater than the given value.
 * The function iterates the nodes in the queue, starting at the front of the queue, and calls
 * the user supplied comparison function until the comparison function returns a value greater
 * than 0.
 *
 * @param queue     The queue head.
 * @param value     The value used for comparison, to be passed to the comparison function.
 * @param comp      The comparison function, which should return ZERO to indicate that the value searched
 *                  is found, < 0 if the value searched is less than the node's value, and > 0 if the
 *                  value searched is greater than the node's value.
 *
 * @return          Pointer to the first node with a value greater than the specified value, or NULL if all
 *                  nodes match or if the queue is empty.
 */
PJ_INLINE(pj_list_type *) pj_leaky_queue_first_greater(pj_leaky_queue *queue, const void *value, int (*comp)(const void *value, const pj_list_type *node))
{
    if (pj_leaky_queue_empty(queue))
        return NULL;
    else
    {
        pj_list *p = (pj_list *) queue->list.next;
        while (p != &queue->list && (*comp)(value, p) >= 0)
            p = (pj_list *) p->next;

        return p == &queue->list ? NULL : p;
    }
}

/**
 * Remove all nodes from the queue.
 *
 * @param queue     The queue head.
 */
PJ_INLINE(void) pj_leaky_queue_erase(pj_leaky_queue *queue)
{
    pj_list_type *node = NULL;

    while ((node = pj_leaky_queue_pop_front(queue)) != NULL)
    {   // No op
    }
}

PJ_END_DECL

#endif	/* __PJ_LEAKY_QUEUE_H__ */
