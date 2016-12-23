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
#include <time.h>
#include <pthread.h>
#include <sys/shm.h>
#include "common_define.h"
#include "shmcache_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
context init
parameters:
	context: the context pointer
    config: the config parameters
    create_segment: if create segment when segment not exist
return error no, 0 for success, != 0 for fail
*/
int shmcache_init(struct shmcache_context *context,
		struct shmcache_config *config,
        const bool create_segment);

/**
context init from config file
parameters:
	context: the context pointer
    config_filename: the config filename
    create_segment: if create segment when segment not exist
return error no, 0 for success, != 0 for fail
*/
int shmcache_init_from_file_ex(struct shmcache_context *context,
		const char *config_filename, const bool create_segment);

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
    return shmcache_init_from_file_ex(context, config_filename, true);
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
return none
*/
void shmcache_stats(struct shmcache_context *context, struct shmcache_stats *stats);

/**
clear stats
parameters:
	context: the context pointer
return none
*/
void shmcache_clear_stats(struct shmcache_context *context);

#ifdef __cplusplus
}
#endif

#endif

