//shmcache.c

#include <errno.h>
#include <pthread.h>
#include "shmcache.h"
#include "logger.h"
#include "shared_func.h"

int shmcache_init(struct shmcache_context *context,
		struct shmcache_config *config)
{
    return 0;
}

void shmcache_destroy(struct shmcache_context *context)
{
}

int shmcache_set(struct shmcache_context *context,
        const struct shmcache_buffer *key,
        const struct shmcache_buffer *value, const int ttl)
{
    return 0;
}

int shmcache_get(struct shmcache_context *context,
        const struct shmcache_buffer *key,
        struct shmcache_buffer *value)
{
    return 0;
}

int shmcache_delete(struct shmcache_context *context,
        const struct shmcache_buffer *key)
{
    return 0;
}
