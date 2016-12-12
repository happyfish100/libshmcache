/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//shm_object_pool.h

#ifndef _SHM_OBJECT_POOL_H
#define _SHM_OBJECT_POOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common_define.h"
#include "shmcache_types.h"

struct shm_opman {
    struct shmcache_hentry_fifo_pool *hentry_fifo_pool;
    int64_t *base;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
op init
parameters:
	op: the op pointer
	alloc_size_once: malloc elements once, 0 for malloc 1MB memory once
    discard_size: discard when remain size <= discard_size, 0 for 64 bytes
return error no, 0 for success, != 0 fail
*/
int shm_object_pool_init(struct shm_opman *op,
        struct shmcache_hentry_fifo_pool *hentry_fifo_pool,
        int64_t *base, const int64_t base_offset,
        const int element_size, const int count);

/**
op destroy
parameters:
	op: the object pool
*/
void shm_object_pool_destroy(struct shm_opman *op);

/**
alloc a node from the object pool
parameters:
	op: the object pool
return the alloced object offset, return -1 if fail
*/
int64_t shm_object_pool_alloc(struct shm_opman *op);

/**
free  a node to the object pool
parameters:
	op: the object pool
    obj_offset: the object offset
return the alloced node offset, return -1 if fail
*/
void shm_object_pool_free(struct shm_opman *op, const int64_t obj_offset);

#ifdef __cplusplus
}
#endif

#endif

