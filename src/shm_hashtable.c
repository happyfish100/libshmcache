//shm_hashtable.c

#include <errno.h>
#include <pthread.h>
#include "shm_hashtable.h"
#include "logger.h"
#include "shared_func.h"

int shm_ht_get_capacity(const int max_count)
{
    unsigned int *capacity;
    capacity = hash_get_prime_capacity(max_count);
    if (capacity == NULL) {
        return max_count;
    }
    return *capacity;
}

void shm_ht_init(struct shmcache_context *context, const int capacity)
{
    context->memory->hashtable.capacity = capacity;
    context->memory->hashtable.count = 0;
    //context->memory->hashtable.list_head = 0;
}

int shm_ht_set(struct shmcache_context *context,
        const struct shmcache_buffer *key,
        const struct shmcache_buffer *value, const int ttl)
{
    //context->memory->hashtable.capacity
    return 0;
}

int shm_ht_get(struct shmcache_context *context,
        const struct shmcache_buffer *key,
        struct shmcache_buffer *value)
{
    return 0;
}

int shm_ht_delete(struct shmcache_context *context,
        const struct shmcache_buffer *key)
{
    return 0;
}
