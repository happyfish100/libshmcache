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

void shm_object_pool_set(struct shm_object_pool *op,
        struct shmcache_hentry_fifo_pool *hentry_fifo_pool,
        int64_t *base)
{
    op->hentry_fifo_pool = hentry_fifo_pool;
    op->base = base;
}

void shm_object_pool_init(struct shm_object_pool *op)
{
    int64_t *p;
    int64_t *end;
    int64_t offset;

    end = op->base + op->hentry_fifo_pool->count.total;
    offset = op->hentry_fifo_pool->object.base_offset;
    for (p=op->base; p<end; p++) {
        *p = offset;
        offset += op->hentry_fifo_pool->object.element_size;
    }

    op->hentry_fifo_pool->index.head = 0;
    op->hentry_fifo_pool->index.tail = (op->hentry_fifo_pool->count.total - 1);
}

int64_t shm_object_pool_alloc(struct shm_object_pool *op)
{
    int64_t obj_offset;
    if (op->hentry_fifo_pool->index.head == op->hentry_fifo_pool->index.tail) {
        return -1;
    }

    obj_offset = op->base[op->hentry_fifo_pool->index.head];
    op->hentry_fifo_pool->index.head = (op->hentry_fifo_pool->index.head + 1) %
        op->hentry_fifo_pool->count.total;
    return obj_offset;
}

int shm_object_pool_free(struct shm_object_pool *op, const int64_t obj_offset)
{
    int next_index;
    next_index = (op->hentry_fifo_pool->index.tail + 1) % op->hentry_fifo_pool->count.total;
    if (next_index == op->hentry_fifo_pool->index.head) {
        return ENOSPC;
    }
    op->base[next_index] = obj_offset;
    op->hentry_fifo_pool->index.tail = next_index;
    return 0;
}

