//shm_value_allocator.c

#include <errno.h>
#include "shm_value_allocator.h"

void shm_value_allocator_init(struct shm_value_allocator *allocator,
		struct shmcache_context *context)
{
}

int shm_value_allocator_alloc(struct shm_value_allocator *allocator,
        const int size, struct shm_value *value)
{
    return 0;
}

int shm_value_allocator_free(struct shm_value_allocator *allocator,
        struct shm_value *value)
{
    return 0;
}

