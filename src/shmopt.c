//shmopt.c

#include <errno.h>
#include <pthread.h>
#include "logger.h"
#include "shm_op_wrapper.h"
#include "shm_striping_allocator.h"
#include "shm_object_pool.h"
#include "shmopt.h"

int shmopt_init_segment(struct shmcache_context *context,
        struct shmcache_segment_info *segment,
        const int proj_id, const int64_t size)
{
    int result;
    segment->proj_id = proj_id;
    segment->base = shm_mmap(context->config.type,
            context->config.filename, proj_id, size, &segment->key,
            context->create_segment, &result);
    if (segment->base == NULL) {
        return result;
    }

    segment->size = size;
    return 0;
}

static int shmopt_init_value_segment(struct shmcache_context *context,
        const int segment_index)
{
    int proj_id;
    struct shmcache_segment_info *segment;

    //proj_id 1 for hashtable segment, value segments start from 2
    proj_id = segment_index + 2;
    segment = context->segments.values.items + segment_index;
    return shmopt_init_segment(context, segment, proj_id,
            context->memory->vm_info.segment.size);
}

int shmopt_create_value_segment(struct shmcache_context *context)
{
    int result;
    int striping_count;
    int segment_index;
    int striping_index;
    int i;
    int64_t striping_offset;
    int64_t allocator_offset;
    struct shm_segment_striping_pair index_pair;
    struct shm_striping_allocator *allocator;

    if (context->memory->vm_info.segment.count.current >=
            context->memory->vm_info.segment.count.max)
    {
        logError("file: "__FILE__", line: %d, "
                "no value segments, reach max: %d", __LINE__,
                context->memory->vm_info.segment.count.max);
        return ENOSPC;
    }
    if (context->segments.values.count != context->memory->vm_info.
            segment.count.current)
    {
        logError("file: "__FILE__", line: %d, "
                "my value segments count: %d != that of shm: %d",
                __LINE__, context->segments.values.count,
                context->memory->vm_info.segment.count.current);
        return EINVAL;
    }

    segment_index = context->memory->vm_info.segment.count.current;
    if ((result=shmopt_init_value_segment(context, segment_index)) != 0) {
        return result;
    }
    context->memory->vm_info.segment.count.current++;

    striping_offset = 0;
    index_pair.segment = segment_index;
    striping_count = context->memory->vm_info.segment.size /
        context->memory->vm_info.striping.size;

    logInfo("file: "__FILE__", line: %d, "
            "striping_count: %d, striping.count.current: %d, "
            "context->memory->vm_info.segment.size: %"PRId64", "
            "context->memory->vm_info.striping.size: %"PRId64,
            __LINE__, striping_count, context->memory->vm_info.striping.count.current,
            context->memory->vm_info.segment.size, context->memory->vm_info.striping.size);

    for (i=0; i<striping_count; i++) {
        striping_index = context->memory->vm_info.striping.count.current++;
        allocator = context->value_allocator.allocators + striping_index;

        index_pair.striping = striping_index;
        shm_striping_allocator_init(allocator, &index_pair,
                striping_offset, context->memory->vm_info.striping.size);

        //add to doing queue
        allocator_offset = (char *)allocator - context->segments.hashtable.base;
        allocator->in_which_pool = SHMCACHE_STRIPING_ALLOCATOR_POOL_DOING;
        shm_object_pool_push(&context->value_allocator.doing, allocator_offset);

        striping_offset += context->memory->vm_info.striping.size;
    }
    context->segments.values.count = segment_index + 1;
    context->memory->usage.alloced += context->memory->vm_info.segment.size;

    logInfo("file: "__FILE__", line: %d, pid: %d, "
            "create value segment #%d, size: %"PRId64,
            __LINE__, context->pid, segment_index + 1,
            context->memory->vm_info.segment.size);

    return 0;
}

int shmopt_open_value_segments(struct shmcache_context *context)
{
    int result;
    int segment_index;

    for (segment_index = context->segments.values.count;
            segment_index < context->memory->vm_info.segment.count.current;
            segment_index++)
    {
        if ((result=shmopt_init_value_segment(context, segment_index)) != 0) {
            return result;
        }
        context->segments.values.count++;
    }

    return 0;
}

int shmopt_remove_all(struct shmcache_context *context)
{
    int result;
    int r;
    int segment_index;
    struct shmcache_segment_info *segment;

    result = shm_remove(context->config.type, context->config.filename,
            context->segments.hashtable.proj_id,
            context->segments.hashtable.size,
            context->segments.hashtable.key);

    for (segment_index=0; segment_index<context->memory->vm_info.segment.
            count.current; segment_index++)
    {
        segment = context->segments.values.items + segment_index;
        if ((result=shmopt_init_value_segment(context, segment_index)) != 0) {
            return result;
        }

        if ((r=shm_remove(context->config.type, context->config.filename,
                segment->proj_id, segment->size,
                segment->key)) != 0)
        {
            result = r;
        }
    }
    return result;
}

