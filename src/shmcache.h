/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//shmcache.h

#ifndef _SHMCACHE_H
#define _SHMCACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define SHMCACHE_MAX_KEY_SIZE  64

#define SHMCACHE_TYPE_SHM    1
#define SHMCACHE_TYPE_MMAP   2

struct shmcache_config
{
    int64_t max_memory;
    int hash_buckets_count;
    int type;  //shm or mmap
};

struct shmcache_memory
{
};

struct shmcache_manager
{
};

struct shmcache_buffer
{
    char *data;
    int length;
};

struct shmcache_context
{
    struct shmcache_config config;
    struct shmcache_memory memory;
    struct shmcache_manager manager;
};

struct shmcache_stats
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
context init
parameters:
	context: the context pointer
    config: the config parameters
return error no, 0 for success, != 0 for fail
*/
int shmcache_init(struct shmcache_context *context,
		struct shmcache_config *config);

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
    value: the value
    ttl: the time to live in seconds
return error no, 0 for success, != 0 for fail
*/
int shmcache_set(struct shmcache_context *context,
        const struct shmcache_buffer *key,
        const struct shmcache_buffer *value, const int ttl);

/**
get value
parameters:
	context: the context pointer
    key: the key
    value: store the returned value
return error no, 0 for success, != 0 for fail
*/
int shmcache_get(struct shmcache_context *context,
        const struct shmcache_buffer *key,
        struct shmcache_buffer *value);

/**
delte the key
parameters:
	context: the context pointer
    key: the key
return error no, 0 for success, != 0 for fail
*/
int shmcache_delete(struct shmcache_context *context,
        const struct shmcache_buffer *key);

/**
get stats
parameters:
	context: the context pointer
    stats: return the stats
*/
void shmcache_stats(struct shmcache_context *context, struct shmcache_stats *stats);

#ifdef __cplusplus
}
#endif

#endif

