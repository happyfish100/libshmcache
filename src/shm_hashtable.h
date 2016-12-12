/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//shm_hashtable.h

#ifndef _SHM_HASH_TABLE_H
#define _SHM_HASH_TABLE_H

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
ht init
parameters:
	ht: the ht pointer
    config: the config parameters
return error no, 0 for success, != 0 for fail
*/
int shm_ht_init(struct shmcache_hashtable *ht,
		struct shmcache_config *config);

/**
ht destroy
parameters:
	ht: the ht pointer
*/
void shm_ht_destroy(struct shmcache_hashtable *ht);

/**
set value
parameters:
	ht: the ht pointer
    key: the key
    value: the value
    ttl: the time to live in seconds
return error no, 0 for success, != 0 for fail
*/
int shm_ht_set(struct shmcache_hashtable *ht,
        const struct shmcache_buffer *key,
        const struct shmcache_buffer *value, const int ttl);

/**
get value
parameters:
	ht: the ht pointer
    key: the key
    value: store the returned value
return error no, 0 for success, != 0 for fail
*/
int shm_ht_get(struct shmcache_hashtable *ht,
        const struct shmcache_buffer *key,
        struct shmcache_buffer *value);

/**
delte the key
parameters:
	ht: the ht pointer
    key: the key
return error no, 0 for success, != 0 for fail
*/
int shm_ht_delete(struct shmcache_hashtable *ht,
        const struct shmcache_buffer *key);

#ifdef __cplusplus
}
#endif

#endif

