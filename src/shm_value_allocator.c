//shm_value_allocator.c

#include <errno.h>
#include "shm_object_pool.h"
#include "shm_striping_allocator.h"
#include "shmopt.h"
#include "shm_value_allocator.h"

static int shm_value_striping_alloc(struct shm_striping_allocator
        *allocator, const int size, struct shm_value *value)
{
    value->offset = shm_striping_allocator_alloc(allocator, size);
    if (value->offset < 0) {
        return ENOMEM;
    }

    value->index = allocator->index;
    value->size = size;
    return 0;
}

static int shm_value_allocator_do_alloc(struct shmcache_context *context,
        const int size, struct shm_value *value)
{
    int64_t allocator_offset;
    struct shm_striping_allocator *allocator;

    allocator_offset = shm_object_pool_first(&context->value_allocator.doing);
    while (allocator_offset > 0) {
        allocator = (struct shm_striping_allocator *)(context->segments.
                hashtable.base + allocator_offset);
        if (shm_value_striping_alloc(allocator, size, value) == 0) {
            return 0;
        }

        allocator_offset = shm_object_pool_next(&context->value_allocator.doing);
    }

    if (shm_object_pool_get_count(&context->value_allocator.free) > 1) {
        allocator_offset = shm_object_pool_pop(&context->value_allocator.free);
        shm_object_pool_push(&context->value_allocator.doing, allocator_offset);

        allocator = (struct shm_striping_allocator *)(context->segments.
                hashtable.base + allocator_offset);
        return shm_value_striping_alloc(allocator, size, value);
    }

    return ENOMEM;
}

int shm_value_allocator_alloc(struct shmcache_context *context,
        const int size, struct shm_value *value)
{
    int result;

    if (shm_value_allocator_do_alloc(context, size, value) == 0) {
        return 0;
    }

    //TODO: scan done allocator to free
    
    if ((result=shmopt_create_value_segment(context)) != 0) {
        return result;
    }
    return shm_value_allocator_do_alloc(context, size, value);
}

int shm_value_allocator_free(struct shmcache_context *context,
        struct shm_value *value)
{
    struct shm_striping_allocator *allocator;

    allocator = context->value_allocator.allocators + value->index.striping;
    shm_striping_allocator_free(allocator, value->size);
    return 0;
}

