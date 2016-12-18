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
#include "hash.h"
#include "common_define.h"

#define SHMCACHE_MAX_KEY_SIZE  64

#define SHMCACHE_STATUS_INIT   0
#define SHMCACHE_STATUS_NORMAL 0x12345678

#define SHMCACHE_TYPE_SHM    1
#define SHMCACHE_TYPE_MMAP   2

#define SHMCACHE_NEVER_EXPIRED_TTL  0

struct shmcache_config {
    char filename[MAX_PATH_SIZE];
    int64_t max_memory;
    int64_t segment_size;
    int max_key_count;
    int max_value_size;
    int type;  //shm or mmap

    struct {
        /* avg. key TTL threshold for recycling memory
         * unit: second
         * <= 0 for never recycle memory until reach memory limit (max_memory)
         */
        int avg_key_ttl;

        /* when the remain memory <= this parameter, discard it.
         * put this allocator in the doing queue to the done queue
         */
        int discard_memory_size;

        /* when a allocator in the doing queue allocate fail times > this parameter,
         * means it is almost full, put it to the done queue
         */
        int max_fail_times;
    } va_policy;   //value allocator policy

    struct {
        int trylock_interval_us;
        int detect_deadlock_interval_ms;
    } lock_policy;

    HashFunc hash_func;
};

struct shm_segment_striping_pair {
    short segment;  //shm segment index
    short striping; //shm striping index
};

struct shm_value {
    int64_t offset; //value segment offset
    int size;       //alloc size
    int length;     //value length
    struct shm_segment_striping_pair index;
};

struct shm_list {
    int64_t prev;
    int64_t next;
};

struct shm_hash_entry {
    struct shm_list list;  //for recycle, must be first

    char key[SHMCACHE_MAX_KEY_SIZE];
    int key_len;
    time_t expires;
    struct shm_value value;
    int64_t ht_next;  //for hashtable
};

struct shm_ring_queue {
    int capacity;
    int head;  //for pop
    int tail;  //for push
};

struct shm_object_pool_info {
    struct {
        int64_t base_offset;
        int element_size;
    } object;
    struct shm_ring_queue queue;
};

struct shm_hashtable {
    struct shm_list head; //for recycle
    int capacity;
    int count;
    int64_t buckets[0]; //entry offset
};

struct shm_striping_allocator {
    time_t first_alloc_time;  //record the timestamp of fist allocate
    int fail_times;   //allocate fail times
    struct shm_segment_striping_pair index;
    struct {
        int total;
        int used;
    } size;

    struct {
        int64_t base;
        int64_t free;
        int64_t end;
    } offset;
};

struct shm_value_allocator {
    struct shm_object_pool_info free;
    struct shm_object_pool_info doing;
    struct shm_object_pool_info done;
};

struct shm_value_size_info {
    int64_t size;
    struct {
        int current;
        int max;
    } count;
};

struct shm_value_memory_info {
    struct shm_value_size_info segment;
    struct shm_value_size_info striping;
};

struct shm_lock {
    pid_t pid;
    pthread_mutex_t mutex;
};

struct shm_memory_info {
    //int version;
    int status;
    struct shm_lock lock;
    struct shm_value_memory_info vm_info;  //value memory info
    struct shm_object_pool_info hentry_obj_pool;  //hash entry object pool
    struct shm_value_allocator value_allocator;
    struct shm_hashtable hashtable;   //must be last
};

struct shmcache_buffer {
    char *data;
    int length;
};

struct shmcache_segment_info {
    key_t key;     //shm key
    int64_t size;  //memory size
    char *base;
};

struct shmcache_object_pool_context {
    struct shm_object_pool_info *obj_pool_info;
    int64_t *offsets;  //object offset array
    int index;   //for iterator
};

struct shmcache_value_allocator_context {
    struct shmcache_object_pool_context doing; //doing queue
    struct shmcache_object_pool_context done;  //done queue
    struct shm_striping_allocator *allocators; //base address
};

struct shmcache_list {
    char *base;
    struct {
        struct shm_list *ptr;
        int64_t offset;
    } head;
};

struct shmcache_context {
    pid_t pid;
    int detect_deadlock_clocks;
    struct shmcache_config config;
    struct shm_memory_info *memory;
    struct {
        struct shmcache_segment_info hashtable;
        struct {
            int count;
            struct shmcache_segment_info *items;
        } values;
    } segments;

    struct shmcache_object_pool_context hentry_allocator;
    struct shmcache_value_allocator_context value_allocator;
    struct shmcache_list list;   //for value recycle
};

struct shmcache_stats {
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

