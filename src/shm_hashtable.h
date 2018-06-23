//shm_hashtable.h

#ifndef _SHM_HASH_TABLE_H
#define _SHM_HASH_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/shm.h>
#include "fastcommon/common_define.h"
#include "shmcache_types.h"
#include "shm_list.h"
#include "shm_value_allocator.h"

#define HT_CALC_EXPIRES(current_time, ttl) \
    (ttl == SHMCACHE_NEVER_EXPIRED ? 0 : current_time + ttl)

#define HT_ENTRY_IS_VALID(entry, current_time) \
    (entry->expires == 0 || entry->expires >= current_time)

#define SHMCACHE_MATCH_KEY_OP_EXACT    0
#define SHMCACHE_MATCH_KEY_OP_LEFT     1
#define SHMCACHE_MATCH_KEY_OP_RIGHT    2
#define SHMCACHE_MATCH_KEY_OP_ANYWHERE \
    (SHMCACHE_MATCH_KEY_OP_LEFT | SHMCACHE_MATCH_KEY_OP_RIGHT)

struct shmcache_match_key_info {
    char *key;
    int length;
    int op_type;
};

#ifdef __cplusplus
extern "C" {
#endif

static inline int64_t shm_ht_get_memory_size(const int capacity)
{
    return sizeof(int64_t) * (int64_t)capacity;
}

/**
get hashtable capacity
parameters:
    max_count: max entry counti
return hashtable capacity
*/
int shm_ht_get_capacity(const int max_count);

/**
ht init
parameters:
	context: the context pointer
    capacity: the ht capacity
return none
*/
void shm_ht_init(struct shmcache_context *context, const int capacity);

/**
set value
parameters:
	context: the context pointer
    key: the key
    value: the value, include expires field
return error no, 0 for success, != 0 for fail
*/
int shm_ht_set(struct shmcache_context *context,
        const struct shmcache_key_info *key,
        const struct shmcache_value_info *value);

/**
set expires
parameters:
	context: the context pointer
    key: the key
    value: the expires timestamp
return error no, 0 for success, != 0 for fail
*/
int shm_ht_set_expires(struct shmcache_context *context,
        const struct shmcache_key_info *key, const time_t expires);

/**
get value
parameters:
	context: the context pointer
    key: the key
    value: store the returned value
return error no, 0 for success, != 0 for fail
*/
int shm_ht_get(struct shmcache_context *context,
        const struct shmcache_key_info *key,
        struct shmcache_value_info *value);

/**
delete the key for internal usage
parameters:
	context: the context pointer
    key: the key
    recycled: if recycled
return error no, 0 for success, != 0 for fail
*/
int shm_ht_delete_ex(struct shmcache_context *context,
        const struct shmcache_key_info *key, bool *recycled);

/**
hashtable entry to array
parameters:
	context: the context pointer
    array: the array, should call shm_ht_free_array after use
    key_info: the match key info, NULL for match all
    offset: start offset, based 0
    count: row count limit
return error no, 0 for success, != 0 for fail
*/
int shm_ht_to_array_ex(struct shmcache_context *context,
        struct shmcache_hentry_array *array,
        struct shmcache_match_key_info *key_info,
        const int offset, const int count);

/**
hashtable entry to array
parameters:
	context: the context pointer
    array: the array, should call shm_ht_free_array after use
return error no, 0 for success, != 0 for fail
*/
static inline int shm_ht_to_array(struct shmcache_context *context,
        struct shmcache_hentry_array *array)
{
    return shm_ht_to_array_ex(context, array, NULL, 0, 0);
}

/**
free the array return by shm_ht_to_array
parameters:
	context: the context pointer
    array: the array to free
return none
*/
void shm_ht_free_array(struct shmcache_hentry_array *array);


/**
delete the key
parameters:
	context: the context pointer
    key: the key
return error no, 0 for success, != 0 for fail
*/
static inline int shm_ht_delete(struct shmcache_context *context,
        const struct shmcache_key_info *key)
{
    bool recycled;
    return shm_ht_delete_ex(context, key, &recycled);
}

/**
free hashtable entry
parameters:
	context: the context pointer
    entry: the hashtable entry
    entry_offset: the entry offset
    recycled: if recycled
return none
*/
void shm_ht_free_entry(struct shmcache_context *context,
        struct shm_hash_entry *entry, const int64_t entry_offset,
        bool *recycled);

/**
remove all hashtable entries
parameters:
	context: the context pointer
return cleared entries count
*/
int shm_ht_clear(struct shmcache_context *context);

static inline int shm_ht_count(struct shmcache_context *context)
{
    return context->memory->hashtable.count;
}

#ifdef __cplusplus
}
#endif

#endif

