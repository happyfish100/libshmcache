//shm_list.h

#ifndef _SHM_LIST_H
#define _SHM_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common_define.h"
#include "logger.h"
#include "shmcache_types.h"
#include "shm_value_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline struct shm_list *shm_list_ptr(struct shmcache_context *context,
        int64_t obj_offset)
{
    if (obj_offset == context->list.head.offset) {
        return context->list.head.ptr;
    } else {
        return (struct shm_list *)shm_get_hentry_ptr(context, obj_offset);
    }
}

#define SHM_LIST_TYPE_PTR(context, type, offset) ((type *)shm_list_ptr(context, offset))

/**
list set
parameters:
	list: the list
return none
*/
static inline void shm_list_set(struct shmcache_context *context,
        char *base, struct shm_list *head)
{
    union shm_hentry_offset conv;
    conv.segment.index = -1;
    conv.segment.offset = (char *)head - base;

    context->list.head.ptr = head;
    context->list.head.offset = conv.offset;
}

/**
init list to empty
parameters:
	list: the list
return none
*/
static inline void shm_list_init(struct shmcache_context *context)
{
    context->list.head.ptr->prev = context->list.head.offset;
    context->list.head.ptr->next = context->list.head.offset;
}

#define SHM_LIST_ADD_TO_TAIL(context, node, obj_offset) \
    do { \
        node->next = context->list.head.offset;    \
        node->prev = context->list.head.ptr->prev; \
        shm_list_ptr(context, context->list.head.ptr->prev)->next = obj_offset; \
        context->list.head.ptr->prev = obj_offset; \
    } while (0)

/**
add an element to list tail
parameters:
	list: the list
    obj_offset: the object offset
return none
*/
static inline void shm_list_add_tail(struct shmcache_context *context, int64_t obj_offset)
{
    struct shm_list *node;

    node = shm_list_ptr(context, obj_offset);
    SHM_LIST_ADD_TO_TAIL(context, node, obj_offset);
}

/**
remove an element from list
parameters:
	list: the list
    obj_offset: the object offset
return none
*/
static inline void shm_list_delete(struct shmcache_context *context, int64_t obj_offset)
{
    struct shm_list *node;

    node = shm_list_ptr(context, obj_offset);
    if (node->next == obj_offset) {
        logError("file: " __FILE__", line: %d, "
                "do NOT need remove from list, obj: %" PRId64,
                __LINE__, obj_offset);
        return;
    }

    shm_list_ptr(context, node->prev)->next = node->next;
    shm_list_ptr(context, node->next)->prev = node->prev;
    node->prev = node->next = obj_offset;
}

/**
move an element to tail
parameters:
	list: the list
    obj_offset: the object offset
return none
*/
static inline void shm_list_move_tail(struct shmcache_context *context, int64_t obj_offset)
{
    struct shm_list *node;

    node = shm_list_ptr(context, obj_offset);
    shm_list_ptr(context, node->prev)->next = node->next;
    shm_list_ptr(context, node->next)->prev = node->prev;

    SHM_LIST_ADD_TO_TAIL(context, node, obj_offset);
}

/**
is empty
parameters:
	list: the list
return true for empty, false for NOT empty
*/
static inline bool shm_list_empty(struct shmcache_context *context)
{
    return (context->list.head.ptr->next == context->list.head.offset);
}

/**
get first element
parameters:
	list: the list
return first object offset
*/
static inline int64_t shm_list_first(struct shmcache_context *context)
{
    if (context->list.head.ptr->next == context->list.head.offset) {  //empty
        return -1;
    }

    return context->list.head.ptr->next;
}

/**
get next element
parameters:
	list: the list
return next object offset
*/
static inline int64_t shm_list_next(struct shmcache_context *context,
        const int64_t current_offset)
{
    struct shm_list *node;
    node = shm_list_ptr(context, current_offset);
    if (node->next == context->list.head.offset) {
        return -1;
    }
    return node->next;
}

/**
get count
parameters:
	list: the list
return count
*/
static inline int shm_list_count(struct shmcache_context *context)
{
    int64_t offset;
    int count;

    count = 0;
    offset = context->list.head.ptr->next;
    while (offset != context->list.head.offset) {
        count++;
        offset = shm_list_ptr(context, offset)->next;
    }

    return count;
}

#define SHM_LIST_FOR_EACH(context, current, member) \
    for (current=SHM_LIST_TYPE_PTR(context, typeof(*current),    \
                context->list.head.ptr->next);                      \
            &current->member != context->list.head.ptr;             \
            current=SHM_LIST_TYPE_PTR(context, typeof(*current), \
                current->member.next))

#ifdef __cplusplus
}
#endif

#endif

