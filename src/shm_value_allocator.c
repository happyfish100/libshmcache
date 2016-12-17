//shm_value_allocator.c

#include <errno.h>
#include "sched_thread.h"
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

    return ENOMEM;
}

static int shm_value_allocator_recycle(struct shmcache_context *context)
{
    int64_t allocator_offset;
    struct shm_striping_allocator *allocator;

    //TODO: scan done allocator to doing
    //
    allocator_offset = shm_object_pool_pop(&context->value_allocator.done);
    allocator = (struct shm_striping_allocator *)(context->segments.
            hashtable.base + allocator_offset);

    shm_object_pool_push(&context->value_allocator.doing, allocator_offset);

    return 0;
}

int shm_value_allocator_alloc(struct shmcache_context *context,
        const int size, struct shm_value *value)
{
    int result;
    bool recycle;
    int64_t allocator_offset;
    struct shm_striping_allocator *allocator;


    if (shm_value_allocator_do_alloc(context, size, value) == 0) {
        return 0;
    }

    if (context->memory->vm_info.segment.count.current >=
            context->memory->vm_info.segment.count.max)
    {
        recycle = true;
        if (shm_object_pool_is_empty(&context->value_allocator.done)) {
            allocator_offset = shm_object_pool_pop(&context->value_allocator.doing);
            shm_object_pool_push(&context->value_allocator.done, allocator_offset);
        }
    } else {
        allocator_offset = shm_object_pool_first(&context->value_allocator.done);
        if (allocator_offset > 0) {
            allocator = (struct shm_striping_allocator *)(context->segments.
                    hashtable.base + allocator_offset);
            recycle = (context->config.avg_key_ttl > 0 && get_current_time() -
                    allocator->first_alloc_time >= context->config.avg_key_ttl);
        } else {
            recycle = false;
        }
    }

    if (recycle && (result=shm_value_allocator_recycle(context)) != 0) {
        return result;
    } else if ((result=shmopt_create_value_segment(context)) != 0) {
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

