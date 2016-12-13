//shm_ring_queue.c

#include <errno.h>
#include "shm_ring_queue.h"

void shm_ring_queue_init_empty(struct shm_ring_queue *queue,
        const int capacity)
{
    queue->capacity = capacity;
    queue->head = queue->tail = 0;
}

void shm_ring_queue_init_full(struct shm_ring_queue *queue,
        const int capacity)
{
    queue->capacity = capacity;
    queue->head = 0;
    queue->tail = (queue->capacity - 1);
}

int shm_ring_queue_pop(struct shm_ring_queue *queue)
{
    int index;
    if (queue->head == queue->tail) {
        return -1;
    }

    index = queue->head;
    queue->head = (queue->head + 1) %
        queue->capacity;
    return index;
}

int shm_ring_queue_push(struct shm_ring_queue *queue)
{
    int next_index;
    next_index = (queue->tail + 1) % queue->capacity;
    if (next_index == queue->head) {
        return -1;
    }
    queue->tail = next_index;
    return queue->tail;
}

