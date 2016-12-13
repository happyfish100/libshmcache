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

struct shm_object_pool {
    struct shmcache_hentry_fifo_pool *hentry_fifo_pool;
    int64_t *base;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
set object pool
parameters:
	op: the object pool
    hentry_fifo_pool: fifo pool in share memory
    base: the array base
return error no, 0 for success, != 0 fail
*/
void shm_object_pool_set(struct shm_object_pool *op,
        struct shmcache_hentry_fifo_pool *hentry_fifo_pool,
        int64_t *base);

/**
init object pool
parameters:
	op: the object pool
return error no, 0 for success, != 0 fail
*/
void shm_object_pool_init(struct shm_object_pool *op);

/**
op destroy
parameters:
	op: the object pool
*/
void shm_object_pool_destroy(struct shm_object_pool *op);

/**
alloc a node from the object pool
parameters:
	op: the object pool
return the alloced object offset, return -1 if fail
*/
int64_t shm_object_pool_alloc(struct shm_object_pool *op);

/**
free  a node to the object pool
parameters:
	op: the object pool
    obj_offset: the object offset
return 0 for success, != 0 fail
*/
int shm_object_pool_free(struct shm_object_pool *op, const int64_t obj_offset);

#ifdef __cplusplus
}
#endif

#endif

