/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//shm_list.h

#ifndef _SHM_LIST_H
#define _SHM_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common_define.h"
#include "shmcache_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SHM_LIST_PTR(list, offset) \
    ((struct shm_list *)(list->base + offset))

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
    SHM_LIST_PTR(list, list->head.ptr->prev)->next = obj_offset;
    node->prev = list->head.ptr->prev;
    node->next = list->head.offset;
    list->head.ptr->prev = obj_offset;
}

/**
remove an element from list
parameters:
	list: the list
    obj_offset: the object offset
return none
*/
static inline void shm_list_remove(struct shmcache_list *list, int64_t obj_offset)
{
    struct shm_list *node;

    node = SHM_LIST_PTR(list, obj_offset);
    SHM_LIST_PTR(list, node->prev)->next = node->next;
    SHM_LIST_PTR(list, node->next)->prev = node->prev;
    node->prev = node->next = obj_offset;
}

/**
pop an element from list
parameters:
	list: the list
return first object offset
*/
static inline int64_t shm_list_pop(struct shmcache_list *list)
{
    int64_t obj_offset;
    if (list->head.ptr->next == list->head.ptr->prev) {  //empty
        return 0;
    }

    obj_offset = list->head.ptr->next;
    shm_list_remove(list, obj_offset);
    return obj_offset;
}

/**
get first element
parameters:
	list: the list
return first object offset
*/
static inline int64_t shm_list_first(struct shmcache_list *list)
{
    if (list->head.ptr->next == list->head.ptr->prev) {  //empty
        return 0;
    }

    return list->head.ptr->next;
}

#ifdef __cplusplus
}
#endif

#endif

