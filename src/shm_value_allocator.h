/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//shm_value_allocator.h

#ifndef _SHM_VALUE_ALLOCATOR_H
#define _SHM_VALUE_ALLOCATOR_H

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
allocator init
parameters:
	allocator: the allocator pointer
    context: the context
return error no, 0 for success, != 0 fail
*/
void shm_value_allocator_init(struct shmcache_value_allocator_context *acontext);

/**
alloc memory from the allocator
parameters:
	allocator: the allocator pointer
    size: alloc bytes
    value: return the value
return error no, 0 for success, != 0 fail
*/
int shm_value_allocator_alloc(struct shmcache_value_allocator_context *acontext,
        const int size, struct shm_value *value);

/**
free memory to the allocator
parameters:
	allocator: the allocator pointer
    value:  the value to free
return error no, 0 for success, != 0 fail
*/
int shm_value_allocator_free(struct shmcache_value_allocator_context *acontext,
        struct shm_value *value);

#ifdef __cplusplus
}
#endif

#endif

