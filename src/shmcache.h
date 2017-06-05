//shmcache.h

#ifndef _SHMCACHE_H
#define _SHMCACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/shm.h>
#include "common_define.h"
#include "shmcache_types.h"
#include "shm_hashtable.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
context init
parameters:
	context: the context pointer
    config: the config parameters
    create_segment: if create segment when segment not exist
    check_segment: if check segment
return error no, 0 for success, != 0 for fail
*/
int shmcache_init(struct shmcache_context *context,
		struct shmcache_config *config, const bool create_segment,
        const bool check_segment);

/**
context init from config file
parameters:
	context: the context pointer
    config_filename: the config filename
    create_segment: if create segment when segment not exist
    check_segment: if check segment
return error no, 0 for success, != 0 for fail
*/
int shmcache_init_from_file_ex(struct shmcache_context *context,
		const char *config_filename, const bool create_segment,
        const bool check_segment);

/**
context init from config file
parameters:
	context: the context pointer
    config_filename: the config filename
return error no, 0 for success, != 0 for fail
*/
static inline int shmcache_init_from_file(struct shmcache_context *context,
		const char *config_filename)
{
    return shmcache_init_from_file_ex(context, config_filename, true, true);
}

/**
load config from file
parameters:
    config: the config pointer
    config_filename: the config filename
return error no, 0 for success, != 0 for fail
*/
int shmcache_load_config(struct shmcache_config *config,
		const char *config_filename);

/**
context destroy
parameters:
	context: the context pointer
*/
void shmcache_destroy(struct shmcache_context *context);

/**
set value
parameters:
	context: the context pointer
    key: the key
    value: the value, include expire filed
return error no, 0 for success, != 0 for fail
*/
int shmcache_set_ex(struct shmcache_context *context,
        const struct shmcache_key_info *key,
        const struct shmcache_value_info *value);

/**
set value
parameters:
	context: the context pointer
    key: the key
    data: the value string ptr
    data_len: the value length
    ttl: the time to live in seconds
return error no, 0 for success, != 0 for fail
*/
int shmcache_set(struct shmcache_context *context,
        const struct shmcache_key_info *key,
        const char *data, const int data_len, const int ttl);

/**
set TTL
parameters:
	context: the context pointer
    key: the key
    ttl: the time to live in seconds
return error no, 0 for success, != 0 for fail
*/
int shmcache_set_ttl(struct shmcache_context *context,
        const struct shmcache_key_info *key, const int ttl);


/**
set expires timestamp
parameters:
	context: the context pointer
    key: the key
    expires: the expires timestamp
return error no, 0 for success, != 0 for fail
*/
int shmcache_set_expires(struct shmcache_context *context,
        const struct shmcache_key_info *key, const int expires);

/**
increase integer value
parameters:
	context: the context pointer
    key: the key
    incr: the incremental number
    ttl: the time to live in seconds
    value: return the new value
return error no, 0 for success, != 0 for fail
*/
int shmcache_incr(struct shmcache_context *context,
        const struct shmcache_key_info *key,
        const int64_t increment,
        const int ttl, int64_t *new_value);

/**
get value
parameters:
	context: the context pointer
    key: the key
    value: store the returned value
return error no, 0 for success, != 0 for fail
*/
int shmcache_get(struct shmcache_context *context,
        const struct shmcache_key_info *key,
        struct shmcache_value_info *value);

/**
delte the key
parameters:
	context: the context pointer
    key: the key
return error no, 0 for success, != 0 for fail
*/
int shmcache_delete(struct shmcache_context *context,
        const struct shmcache_key_info *key);


/**
remove all share memory
parameters:
	context: the context pointer
return error no, 0 for success, != 0 for fail
*/
int shmcache_remove_all(struct shmcache_context *context);

/**
get stats
parameters:
	context: the context pointer
    stats: return the stats
    calc_hit_ratio: if calculate hit_ratio
return none
*/
void shmcache_stats_ex(struct shmcache_context *context, struct shmcache_stats *stats,
        const bool calc_hit_ratio);

static inline void shmcache_stats(struct shmcache_context *context,
        struct shmcache_stats *stats)
{
    shmcache_stats_ex(context, stats, true);
}

/**
clear stats
parameters:
	context: the context pointer
return none
*/
void shmcache_clear_stats(struct shmcache_context *context);

/**
get serializer label
parameters:
	serializer: the serializer
return serializer label
*/
const char *shmcache_get_serializer_label(const int serializer);

/**
clear hashtable
parameters:
	context: the context pointer
return error no, 0 for success, != 0 for fail
*/
int shmcache_clear(struct shmcache_context *context);

#ifdef __cplusplus
}
#endif

#endif

