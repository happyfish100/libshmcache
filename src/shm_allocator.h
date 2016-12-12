/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//shm_allocator.h

#ifndef _SHM_ALLOCATOR_H
#define _SHM_ALLOCATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common_define.h"

/* malloc chain */
struct shm_allocator_malloc
{
	int alloc_size;
    char *base_ptr;
    char *end_ptr;
    char *free_ptr;
	struct shm_allocator_malloc *malloc_next;
	struct shm_allocator_malloc *free_next;
};

struct shm_allocator_man
{
	struct shm_allocator_malloc *malloc_chain_head; //malloc chain to be freed
	struct shm_allocator_malloc *free_chain_head;   //free node chain
	int alloc_size_once;  //alloc size once, default: 1MB
	int discard_size;     //discard size, default: 64 bytes
};

struct shm_allocator_stats
{
    int64_t total_bytes;
    int64_t free_bytes;
    int total_trunk_count;
    int free_trunk_count;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
mpool init
parameters:
	mpool: the mpool pointer
	alloc_size_once: malloc elements once, 0 for malloc 1MB memory once
    discard_size: discard when remain size <= discard_size, 0 for 64 bytes
return error no, 0 for success, != 0 fail
*/
int shm_allocator_init(struct shm_allocator_man *mpool,
		const int alloc_size_once, const int discard_size);

/**
mpool destroy
parameters:
	mpool: the mpool pointer
*/
void shm_allocator_destroy(struct shm_allocator_man *mpool);

/**
reset for recycle use
parameters:
	mpool: the mpool pointer
*/
void shm_allocator_reset(struct shm_allocator_man *mpool);

/**
alloc a node from the mpool
parameters:
	mpool: the mpool pointer
    size: alloc bytes
return the alloced ptr, return NULL if fail
*/
void *shm_allocator_alloc(struct shm_allocator_man *mpool, const int size);

/**
get stats
parameters:
	mpool: the mpool pointer
    stats: return the stats
*/
void shm_allocator_stats(struct shm_allocator_man *mpool, struct shm_allocator_stats *stats);

#ifdef __cplusplus
}
#endif

#endif

