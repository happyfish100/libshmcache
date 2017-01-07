//shm_striping_allocator.h

#ifndef _SHM_STRIPING_ALLOCATOR_H
#define _SHM_STRIPING_ALLOCATOR_H

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
    ssp_index: segment and striping pair
    base_offset: the base offset
	total_size: the memory size
return error no, 0 for success, != 0 fail
*/
void shm_striping_allocator_init(struct shm_striping_allocator *allocator,
		const struct shm_segment_striping_pair *ssp_index,
        const int64_t base_offset, const int total_size);

/**
reset for recycle use
parameters:
	allocator: the allocator pointer
*/
static inline void shm_striping_allocator_reset(struct shm_striping_allocator *allocator)
{
    allocator->last_alloc_time = 0;
    allocator->fail_times = 0;
    allocator->size.used = 0;
    allocator->offset.free = allocator->offset.base;
}

/**
alloc memory from the allocator
parameters:
	allocator: the allocator pointer
    size: alloc bytes
return the alloced memory offset, return -1 if fail
*/
int64_t shm_striping_allocator_alloc(struct shm_striping_allocator *allocator,
        const int size);

/**
free memory to the allocator
parameters:
	allocator: the allocator pointer
    size: alloc bytes
return used size
*/
static inline int64_t shm_striping_allocator_free(struct shm_striping_allocator
        *allocator, const int size)
{
    allocator->size.used -= size;
    return allocator->size.used;
}

/**
get free memory size
parameters:
	allocator: the allocator pointer
return free memory size 
*/
static inline int64_t shm_striping_allocator_free_size(struct shm_striping_allocator
        *allocator)
{
    return allocator->offset.end - allocator->offset.free;
}

/**
get used memory size
parameters:
	allocator: the allocator pointer
return used memory size 
*/
static inline int64_t shm_striping_allocator_used_size(struct shm_striping_allocator
        *allocator)
{
    return allocator->size.used;
}

#ifdef __cplusplus
}
#endif

#endif

