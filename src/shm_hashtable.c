//shm_hashtable.c

#include <errno.h>
#include <pthread.h>
#include "shm_hashtable.h"
#include "logger.h"
#include "shared_func.h"

int shm_ht_init(struct shmcache_hashtable *ht,
		struct shmcache_config *config)
{
    return 0;
}

void shm_ht_destroy(struct shmcache_hashtable *ht)
{
}

int shm_ht_set(struct shmcache_hashtable *ht,
        const struct shmcache_buffer *key,
        const struct shmcache_buffer *value, const int ttl)
{
    return 0;
}

int shm_ht_get(struct shmcache_hashtable *ht,
        const struct shmcache_buffer *key,
        struct shmcache_buffer *value)
{
    return 0;
}

int shm_ht_delete(struct shmcache_hashtable *ht,
        const struct shmcache_buffer *key)
{
    return 0;
}
