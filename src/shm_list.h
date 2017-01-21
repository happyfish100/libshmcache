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

#ifdef __cplusplus
extern "C" {
#endif

#define SHM_LIST_PTR(list, offset) ((struct shm_list *)((list)->base + offset))
#define SHM_LIST_TYPE_PTR(list, type, offset) ((type *)((list)->base + offset))

/**
list set
parameters:
	list: the list
return none
*/
static inline void shm_list_set(struct shmcache_list *list,
        char *base, struct shm_list *head)
{
    list->base = base;
    list->head.ptr = head;
    list->head.offset = (char *)head - list->base;
}

/**
init list to empty
parameters:
	list: the list
return none
*/
static inline void shm_list_init(struct shmcache_list *list)
{
    list->head.ptr->prev = list->head.ptr->next = list->head.offset;
}

#define SHM_LIST_ADD_TO_TAIL(list, node, obj_offset) \
    do { \
        node->next = list->head.offset;    \
        node->prev = list->head.ptr->prev; \
        SHM_LIST_PTR(list, list->head.ptr->prev)->next = obj_offset; \
        list->head.ptr->prev = obj_offset; \
    } while (0)

/**
add an element to list tail
parameters:
	list: the list
    obj_offset: the object offset
return none
*/
static inline void shm_list_add_tail(struct shmcache_list *list, int64_t obj_offset)
{
    struct shm_list *node;

    node = SHM_LIST_PTR(list, obj_offset);
    SHM_LIST_ADD_TO_TAIL(list, node, obj_offset);
}

/**
remove an element from list
parameters:
	list: the list
    obj_offset: the object offset
return none
*/
static inline void shm_list_delete(struct shmcache_list *list, int64_t obj_offset)
{
    struct shm_list *node;

    node = SHM_LIST_PTR(list, obj_offset);
    if (node->next == obj_offset) {
        logError("file: " __FILE__", line: %d, "
                "do NOT need remove from list, obj: %" PRId64,
                __LINE__, obj_offset);
        return;
    }

    SHM_LIST_PTR(list, node->prev)->next = node->next;
    SHM_LIST_PTR(list, node->next)->prev = node->prev;
    node->prev = node->next = obj_offset;
}

/**
move an element to tail
parameters:
	list: the list
    obj_offset: the object offset
return none
*/
static inline void shm_list_move_tail(struct shmcache_list *list, int64_t obj_offset)
{
    struct shm_list *node;

    node = SHM_LIST_PTR(list, obj_offset);
    SHM_LIST_PTR(list, node->prev)->next = node->next;
    SHM_LIST_PTR(list, node->next)->prev = node->prev;

    SHM_LIST_ADD_TO_TAIL(list, node, obj_offset);
}

/**
is empty
parameters:
	list: the list
return true for empty, false for NOT empty
*/
static inline bool shm_list_empty(struct shmcache_list *list)
{
    return (list->head.ptr->next == list->head.offset);
}

/**
get first element
parameters:
	list: the list
return first object offset
*/
static inline int64_t shm_list_first(struct shmcache_list *list)
{
    if (list->head.ptr->next == list->head.offset) {  //empty
        return 0;
    }

    return list->head.ptr->next;
}

/**
get next element
parameters:
	list: the list
return next object offset
*/
static inline int64_t shm_list_next(struct shmcache_list *list,
        const int64_t current_offset)
{
    struct shm_list *node;
    node = SHM_LIST_PTR(list, current_offset);
    if (node->next == list->head.offset) {
        return 0;
    }
    return node->next;
}

/**
get count
parameters:
	list: the list
return count
*/
static inline int shm_list_count(struct shmcache_list *list)
{
    int64_t offset;
    int count;

    count = 0;
    offset = list->head.ptr->next;
    while (offset != list->head.offset) {
        count++;
        offset = SHM_LIST_PTR(list, offset)->next;
    }

    return count;
}

#define SHM_LIST_FOR_EACH(list, current, member) \
    for (current=SHM_LIST_TYPE_PTR(list, typeof(*current),    \
                (list)->head.ptr->next);                      \
            &current->member != (list)->head.ptr;             \
            current=SHM_LIST_TYPE_PTR(list, typeof(*current), \
                current->member.next))

#ifdef __cplusplus
}
#endif

#endif

