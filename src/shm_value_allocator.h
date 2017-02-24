//shm_value_allocator.h

#ifndef _SHM_VALUE_ALLOCATOR_H
#define _SHM_VALUE_ALLOCATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common_define.h"
#include "shmcache_types.h"
#include "shmopt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
alloc memory from the allocator
parameters:
	context: the shm context
    key_len: the key length
    value_len: the value length
return error no, 0 for success, != 0 fail
*/
struct shm_hash_entry *shm_value_allocator_alloc(struct shmcache_context *context,
        const int key_len, const int value_len);

/**
free memory to the allocator
parameters:
	context: the shm context
    value:  the value to free
    recycled: if recycled
return error no, 0 for success, != 0 fail
*/
int shm_value_allocator_free(struct shmcache_context *context,
        struct shm_hash_entry *entry, bool *recycled);

/**
recycle oldest hashtable entries
parameters:
	context: the shm context
    recycle_stats: the recycle stats
    recycle_key_once: recycle key number once,
                       <= 0 for until recycle a value allocator
return error no, 0 for success, != 0 fail
*/
int shm_value_allocator_recycle(struct shmcache_context *context,
        struct shm_recycle_stats *recycle_stats, const int recycle_key_once);

static inline char *shm_get_value_ptr(struct shmcache_context *context,
        struct shm_hash_entry *entry)
{
    char *base;
    base = shmopt_get_value_segment(context, entry->memory.index.segment);
    if (base != NULL) {
        return base + entry->memory.offset + sizeof(struct shm_hash_entry)
            + MEM_ALIGN(entry->key_len);
    } else {
        return NULL;
    }
}

static inline struct shm_hash_entry *shm_get_hentry_ptr(struct shmcache_context *context,
        const int64_t offset)
{
    char *base;
    union shm_hentry_offset conv;

    conv.offset = offset;
    base = shmopt_get_value_segment(context, conv.segment.index & 0xFF);
    if (base != NULL) {
        return (struct shm_hash_entry *)(base + conv.segment.offset);
    } else {
        return NULL;
    }
}

static inline int64_t shm_get_hentry_offset(struct shm_hash_entry *entry)
{
    union shm_hentry_offset conv;
    conv.segment.index = entry->memory.index.segment | 0x4000;
    conv.segment.offset = entry->memory.offset;

    return conv.offset;
}

#ifdef __cplusplus
}
#endif

#endif
