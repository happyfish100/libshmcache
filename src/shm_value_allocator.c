//shm_value_allocator.c

#include <errno.h>
#include "shm_value_allocator.h"

void shm_value_allocator_init(struct shmcache_value_allocator_context *acontext)
{
}

int shm_value_allocator_alloc(struct shmcache_value_allocator_context *acontext,
        const int size, struct shm_value *value)
{
    return 0;
}

int shm_value_allocator_free(struct shmcache_value_allocator_context *acontext,
        struct shm_value *value)
{
    return 0;
}

