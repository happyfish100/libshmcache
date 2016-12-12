/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//shmcache_types.h

#ifndef _SHMCACHE_TYPES_H
#define _SHMCACHE_TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/shm.h>
#include "common_define.h"

#define SHMCACHE_MAX_KEY_SIZE  64

#define SHMCACHE_STATUS_INIT   0
#define SHMCACHE_STATUS_NORMAL 1

#define SHMCACHE_TYPE_SHM    1
#define SHMCACHE_TYPE_MMAP   2

struct shmcache_config
{
    char filename[MAX_PATH_SIZE];
    int64_t max_memory;
    int64_t segment_size;
    int max_key_count;
    int max_value_size;
    int hash_buckets_count;
    int type;  //shm or mmap
};

struct shmcache_value
{
    int64_t offset;
    int segment; //value segment index
    int length;
};

struct shmcache_hash_entry
{
    char key[SHMCACHE_MAX_KEY_SIZE];
    int key_len;
    struct shmcache_value value;
    time_t expires;
    int64_t next_offset;
};

struct shmcache_hentry_fifo_pool {
    int element_size;
    int total_count;
    int free_count;
    int head;  //OUT
    int tail;  //IN
};

struct shmcache_hashtable
{
    int count;
    int64_t buckets[0];
};

struct shmcache_value_size_info
{
    int64_t size;
    int count;
};

struct shmcache_value_memory_info { 
    struct shmcache_value_size_info segment;
    struct shmcache_value_size_info strip;
};

struct shmcache_memory_info
{
    int version;
    int status;
    pthread_mutex_t lock;
    struct shmcache_value_memory_info value_memory_info;
    struct shmcache_hentry_fifo_pool hentry_fifo_pool;
    struct shmcache_hashtable hashtable;
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
    struct shmcache_memory_info memory;
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

#ifdef __cplusplus
}
#endif

#endif

