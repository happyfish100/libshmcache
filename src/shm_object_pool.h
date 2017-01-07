//shm_object_pool.h

#ifndef _SHM_OBJECT_POOL_H
#define _SHM_OBJECT_POOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common_define.h"
#include "shmcache_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
get object pool memory size for queue
parameters:
    max_count: the max count
return memory size
*/
static inline int64_t shm_object_pool_get_queue_memory_size(const int max_count)
{
    return sizeof(int64_t) * (int64_t)max_count;
}

/**
get object pool memory size for object
parameters:
    element_size: the element size
    max_count: the max count
return memory size
*/
static inline int64_t shm_object_pool_get_object_memory_size(
        const int element_size, const int max_count)
{
    return (int64_t)element_size * (int64_t)max_count;
}

/**
set object pool
parameters:
	op: the object pool
    obj_pool_info: fifo pool in share memory
    offsets: the array offsets
return error no, 0 for success, != 0 fail
*/
void shm_object_pool_set(struct shmcache_object_pool_context *op,
        struct shm_object_pool_info *obj_pool_info,
        int64_t *offsets);

/**
init object pool to empty queue
parameters:
	op: the object pool
return error no, 0 for success, != 0 fail
*/
void shm_object_pool_init_empty(struct shmcache_object_pool_context *op);

/**
init object pool to full queue
parameters:
	op: the object pool
return error no, 0 for success, != 0 fail
*/
void shm_object_pool_init_full(struct shmcache_object_pool_context *op);

/**
get object count in object pool
parameters:
	op: the object pool
return object count
*/
int shm_object_pool_get_count(struct shmcache_object_pool_context *op);

/**
alloc a node from the object pool
parameters:
	op: the object pool
return the alloced object offset, return -1 if fail
*/
int64_t shm_object_pool_alloc(struct shmcache_object_pool_context *op);

#define shm_object_pool_pop(op) shm_object_pool_alloc(op)

/**
free  a node to the object pool
parameters:
	op: the object pool
    obj_offset: the object offset
return 0 for success, != 0 fail
*/
int shm_object_pool_free(struct shmcache_object_pool_context *op,
        const int64_t obj_offset);

#define shm_object_pool_push(op, obj_offset) shm_object_pool_free(op, obj_offset)

/**
is object pool empty
parameters:
	op: the object pool
return return true if empty, otherwise false
*/
static inline bool shm_object_pool_is_empty(struct shmcache_object_pool_context *op)
{
    return (op->obj_pool_info->queue.head == op->obj_pool_info->queue.tail);
}

/**
get first object
parameters:
	op: the object pool
return object offset, return -1 if empty
*/
static inline int64_t shm_object_pool_first(struct shmcache_object_pool_context *op)
{
    if (op->obj_pool_info->queue.head == op->obj_pool_info->queue.tail) {
        op->index = -1;
        return -1;
    }
    op->index = op->obj_pool_info->queue.head;
    return op->offsets[op->index];
}

/**
get next object
parameters:
	op: the object pool
return object offset, return -1 if empty
*/
static inline int64_t shm_object_pool_next(struct shmcache_object_pool_context *op)
{
    if (op->index == -1) {
        return -1;
    }
    if (op->index == op->obj_pool_info->queue.tail) {
        op->index = -1;
        return -1;
    }

    op->index = (op->index + 1) % op->obj_pool_info->queue.capacity;
    if (op->index != op->obj_pool_info->queue.tail) {
        return op->offsets[op->index];
    } else {
        return -1;
    }
}

/**
remove the object, if op->index is -1, remove the tail
parameters:
	op: the object pool
return the removed object offset, return -1 if empty
*/
int64_t shm_object_pool_remove(struct shmcache_object_pool_context *op);

/**
remove the object
parameters:
	op: the object pool
    obj_offset: the object offset
return the removed object offset, return -1 if empty
*/
int64_t shm_object_pool_remove_by(struct shmcache_object_pool_context *op,
        const int64_t obj_offset);

#ifdef __cplusplus
}
#endif

#endif

