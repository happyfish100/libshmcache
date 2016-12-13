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
    time_t expires;
    struct shmcache_value value;
    int64_t next_offset;
};

struct shmcache_ring_queue {
    int head;  //for pop
    int tail;  //for push
};

struct shmcache_hentry_fifo_pool {
    struct {
        int64_t base_offset;
        int element_size;
    } object;
    struct {
        int total;
        int free;
    } count;
    struct shmcache_ring_queue index;
};

struct shmcache_hashtable
{
    int capacity;
    int count;
    int64_t buckets[0]; //entry offset
};

struct shm_allocator_info
{
    int status;
    struct {
        int64_t total;
        int64_t used;
    } size;

    struct {
        int64_t base;
        int64_t free;
        int64_t end;
    } offset;
};

struct shmcache_value_allocators
{
    struct shmcache_ring_queue free;
    struct shmcache_ring_queue doing;
    struct shmcache_ring_queue done;
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
    struct shmcache_value_allocators value_allocators;
    struct shmcache_hashtable hashtable;
};

struct shmcache_buffer
{
    char *data;
    int length;
};

struct shmcache_segment_info
{
    key_t key; //shm key
    int size;  //memory size
    void *base;
};

struct shmcache_segment_array
{
    struct shmcache_segment_info *segments;
    int alloc_size;
    int count;
};

struct shmcache_context
{
    struct shmcache_config config;
    struct shmcache_memory_info *memory;
    struct shmcache_segment_info key;
    struct shmcache_segment_array values;
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

