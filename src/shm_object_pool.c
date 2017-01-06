//shm_object_pool.c

#include <errno.h>
#include <pthread.h>
#include "logger.h"
#include "shm_object_pool.h"

void shm_object_pool_set(struct shmcache_object_pool_context *op,
        struct shm_object_pool_info *obj_pool_info,
        int64_t *offsets)
{
    op->obj_pool_info = obj_pool_info;
    op->offsets = offsets;
    op->index = -1;
}

void shm_object_pool_init_full(struct shmcache_object_pool_context *op)
{
    int64_t *p;
    int64_t *end;
    int64_t offset;

    end = op->offsets + (op->obj_pool_info->queue.capacity - 1);
    offset = op->obj_pool_info->object.base_offset;
    for (p=op->offsets; p<end; p++) {
        *p = offset;
        offset += op->obj_pool_info->object.element_size;
    }

    op->obj_pool_info->queue.head = 0;
    op->obj_pool_info->queue.tail = op->obj_pool_info->queue.capacity - 1;
}

void shm_object_pool_init_empty(struct shmcache_object_pool_context *op)
{
    op->obj_pool_info->queue.head = op->obj_pool_info->queue.tail = 0;
}

int shm_object_pool_get_count(struct shmcache_object_pool_context *op)
{
    if (op->obj_pool_info->queue.head == op->obj_pool_info->queue.tail) {
        return 0;
    } else if (op->obj_pool_info->queue.head < op->obj_pool_info->queue.tail) {
        return op->obj_pool_info->queue.tail - op->obj_pool_info->queue.head;
    } else {
        return (op->obj_pool_info->queue.capacity - op->obj_pool_info->queue.head)
            +  op->obj_pool_info->queue.tail;
    }
}

int64_t shm_object_pool_alloc(struct shmcache_object_pool_context *op)
{
    int64_t obj_offset;
    if (op->obj_pool_info->queue.head == op->obj_pool_info->queue.tail) {
        return -1;
    }

    obj_offset = op->offsets[op->obj_pool_info->queue.head];
    op->obj_pool_info->queue.head = (op->obj_pool_info->queue.head + 1) %
        op->obj_pool_info->queue.capacity;

    return obj_offset;
}

int shm_object_pool_free(struct shmcache_object_pool_context *op,
        const int64_t obj_offset)
{
    int next_tail;
    next_tail = (op->obj_pool_info->queue.tail + 1) %
        op->obj_pool_info->queue.capacity;
    if (next_tail == op->obj_pool_info->queue.head) {
        return ENOSPC;
    }
    op->offsets[op->obj_pool_info->queue.tail] = obj_offset;
    op->obj_pool_info->queue.tail = next_tail;

    return 0;
}

#define SHM_OP_DEC_INDEX(index, capacity)       \
    (index == 0 ? (capacity - 1) : (index - 1))

int64_t shm_object_pool_remove(struct shmcache_object_pool_context *op)
{
    int index;
    int previous;
    int current;

    if (op->obj_pool_info->queue.head == op->obj_pool_info->queue.tail) {
        logError("file: "__FILE__", line: %d, "
                "object pool is empty", __LINE__);
        return -1;
    }
    if (op->index < 0) {
        logError("file: "__FILE__", line: %d, "
                "current index: %d < 0", __LINE__, op->index);
        return -1;
    }

    index = op->index;
    current = index;
    while (current != op->obj_pool_info->queue.head) {
        previous = SHM_OP_DEC_INDEX(current, op->obj_pool_info->queue.capacity);
        op->offsets[current] = op->offsets[previous];
        current = previous;
    }

    op->obj_pool_info->queue.head = (op->obj_pool_info->queue.head + 1) %
        op->obj_pool_info->queue.capacity;
    return op->offsets[index];
}

int64_t shm_object_pool_remove_by(struct shmcache_object_pool_context *op,
        const int64_t obj_offset)
{
    int64_t current_offset;

    current_offset = shm_object_pool_first(op);
    while (current_offset > 0) {
        if (current_offset == obj_offset) {
            return shm_object_pool_remove(op);
        }
        current_offset = shm_object_pool_next(op);
    }

    return -1;
}

