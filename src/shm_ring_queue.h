/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//shm_ring_queue.h

#ifndef _SHM_RING_QUEUE_H
#define _SHM_RING_QUEUE_H

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
init ring queue to empty
parameters:
	queue: the queue
    capacity: queue capacity
return error no, 0 for success, != 0 fail
*/
void shm_ring_queue_init_empty(struct shm_ring_queue *queue,
        const int capacity);

/**
init ring queue to full
parameters:
	queue: the queue
    capacity: queue capacity
return error no, 0 for success, != 0 fail
*/
void shm_ring_queue_init_full(struct shm_ring_queue *queue,
        const int capacity);

/**
queue pop
parameters:
	queue: the queue
return the head object index, return -1 if fail
*/
int shm_ring_queue_pop(struct shm_ring_queue *queue);

/**
queue push
parameters:
	queue: the queue
return the tail object index after push, return -1 if fail
*/
int shm_ring_queue_push(struct shm_ring_queue *queue);

#ifdef __cplusplus
}
#endif

#endif

