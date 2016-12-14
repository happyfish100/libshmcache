//shm_object_pool.c

#include <errno.h>
#include <sys/resource.h>
#include <pthread.h>
#include <assert.h>
#include "shm_object_pool.h"
#include "logger.h"
#include "shared_func.h"
#include "pthread_func.h"
#include "sched_thread.h"

void shm_object_pool_set(struct shm_object_pool_context *op,
        struct shm_object_pool_info *obj_pool_info,
        int64_t *base)
{
    op->obj_pool_info = obj_pool_info;
    op->base = base;
}

void shm_object_pool_init_full(struct shm_object_pool_context *op)
{
    int64_t *p;
    int64_t *end;
    int64_t offset;

    end = op->base + op->obj_pool_info->queue.capacity;
    offset = op->obj_pool_info->object.base_offset;
    for (p=op->base; p<end; p++) {
        *p = offset;
        offset += op->obj_pool_info->object.element_size;
    }

    op->obj_pool_info->queue.head = 0;
    op->obj_pool_info->queue.tail = (op->obj_pool_info->queue.capacity - 1);
}

void shm_object_pool_init_empty(struct shm_object_pool_context *op)
{
    op->obj_pool_info->queue.head = op->obj_pool_info->queue.tail = 0;
}

int64_t shm_object_pool_alloc(struct shm_object_pool_context *op)
{
    int64_t obj_offset;
    if (op->obj_pool_info->queue.head == op->obj_pool_info->queue.tail) {
        return -1;
    }

    obj_offset = op->base[op->obj_pool_info->queue.head];
    op->obj_pool_info->queue.head = (op->obj_pool_info->queue.head + 1) %
        op->obj_pool_info->queue.capacity;
    return obj_offset;
}

int shm_object_pool_free(struct shm_object_pool_context *op, const int64_t obj_offset)
{
    int next_index;
    next_index = (op->obj_pool_info->queue.tail + 1) % op->obj_pool_info->queue.capacity;
    if (next_index == op->obj_pool_info->queue.head) {
        return ENOSPC;
    }
    op->base[next_index] = obj_offset;
    op->obj_pool_info->queue.tail = next_index;
    return 0;
}

