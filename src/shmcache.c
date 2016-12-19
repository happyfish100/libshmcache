//shmcache.c

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "logger.h"
#include "shared_func.h"
#include "shm_hashtable.h"
#include "shm_object_pool.h"
#include "shm_op_wrapper.h"
#include "shmopt.h"
#include "shm_list.h"
#include "shm_lock.h"
#include "shmcache.h"

#define SHMCACE_MEM_ALIGN(x, align)  (((x) + (align - 1)) & (~(align - 1)))

//HT for HashTable, VA for Value Allocator
#define OFFSETS_INDEX_HT_BUCKETS            0
#define OFFSETS_INDEX_HT_POOL_QUEUE         1
#define OFFSETS_INDEX_HT_POOL_OBJECT        2
#define OFFSETS_INDEX_VA_POOL_QUEUE_DOING   3
#define OFFSETS_INDEX_VA_POOL_QUEUE_DONE    4
#define OFFSETS_INDEX_VA_POOL_OBJECT        5
#define OFFSETS_COUNT                       6

static void get_value_segment_count_size(struct shmcache_config *config,
        struct shm_value_size_info *segment)
{
    int page_size;

    page_size = getpagesize();
    segment->size = SHMCACE_MEM_ALIGN(config->segment_size, page_size);
    segment->count.max = config->max_memory / segment->size;
    if (segment->count.max == 0) {
        segment->count.max = 1;
    }
    segment->count.current = 0;
}

static void get_value_striping_count_size(const int max_value_size,
        const struct shm_value_size_info *segment,
        struct shm_value_size_info *striping)
{
    int page_size;
    int mb_count;

    page_size = getpagesize();
    mb_count = segment->size / (128 * 1024 * 1024);
    if (mb_count == 0) {
        mb_count = 1;
    } else if (mb_count > 8) {
        mb_count = 8;
    } else {
        if (mb_count > 2 && mb_count < 4) {
            mb_count = 2;
        } else if (mb_count > 4 && mb_count < 8) {
            mb_count = 4;
        }
    }

    striping->size = mb_count * 1024 * 1024;
    if (striping->size < max_value_size * 2) {
        striping->size = max_value_size * 2;
    }
    striping->size = SHMCACE_MEM_ALIGN(striping->size, page_size);
    if (striping->size > segment->size) {
        striping->size = segment->size;
    }
    striping->count.max = segment->count.max * (segment->size / striping->size);
    striping->count.current = 0;
}

static int64_t shmcache_get_ht_segment_size(struct shmcache_context *context,
        struct shm_value_size_info *segment,
        struct shm_value_size_info *striping,
        int *ht_capacity, int64_t *ht_offsets)
{
    int64_t total_size;
    int64_t va_pool_queue_memory_size;

    get_value_segment_count_size(&context->config, segment);
    get_value_striping_count_size(context->config.max_value_size,
            segment, striping);

    *ht_capacity = shm_ht_get_capacity(context->config.max_key_count + 1);
    total_size = sizeof(struct shm_memory_info);

    logInfo("ht capacity: %d, sizeof(struct shm_memory_info): %d, "
            "context->config.max_key_count: %d",
            *ht_capacity, (int)sizeof(struct shm_memory_info),
            context->config.max_key_count);

    ht_offsets[OFFSETS_INDEX_HT_BUCKETS] = total_size;
    total_size += shm_ht_get_memory_size(*ht_capacity);

    ht_offsets[OFFSETS_INDEX_HT_POOL_QUEUE] = total_size;
    total_size += shm_object_pool_get_queue_memory_size(
            context->config.max_key_count + 1);

    ht_offsets[OFFSETS_INDEX_HT_POOL_OBJECT] = total_size;
    total_size += shm_object_pool_get_object_memory_size(
            sizeof(struct shm_hash_entry), context->config.max_key_count);

    va_pool_queue_memory_size = shm_object_pool_get_queue_memory_size(
            striping->count.max + 1);

    ht_offsets[OFFSETS_INDEX_VA_POOL_QUEUE_DOING] = total_size;
    total_size += va_pool_queue_memory_size;

    ht_offsets[OFFSETS_INDEX_VA_POOL_QUEUE_DONE] = total_size;
    total_size += va_pool_queue_memory_size;

    ht_offsets[OFFSETS_INDEX_VA_POOL_OBJECT] = total_size;
    total_size += shm_object_pool_get_object_memory_size(
            sizeof(struct shm_striping_allocator), striping->count.max);

    return total_size;
}

static void shmcache_set_object_pool_context(struct shmcache_object_pool_context
        *context, struct shm_object_pool_info *op,
        const int element_size, const int64_t obj_base_offset,
        const int queue_capacity, int64_t *queue_base,
        const bool init_full)
{
    op->object.element_size = element_size;
    op->object.base_offset = obj_base_offset;
    op->queue.capacity = queue_capacity;

    shm_object_pool_set(context, op, queue_base);
    if (init_full) {
        shm_object_pool_init_full(context);
    } else {
        shm_object_pool_init_empty(context);
    }
}

static int shmcache_do_init(struct shmcache_context *context,
        int64_t *ht_offsets)
{
	int result;
    int64_t *queue_base;

    if ((result=shm_lock_init(context)) != 0) {
        return result;
    }

    queue_base = (int64_t *)(context->segments.hashtable.base +
            ht_offsets[OFFSETS_INDEX_HT_POOL_QUEUE]);
    shmcache_set_object_pool_context(&context->hentry_allocator,
            &context->memory->hentry_obj_pool,
            sizeof(struct shm_hash_entry),
            ht_offsets[OFFSETS_INDEX_HT_POOL_OBJECT],
            context->config.max_key_count, queue_base, true);

    queue_base = (int64_t *)(context->segments.hashtable.base +
            ht_offsets[OFFSETS_INDEX_VA_POOL_QUEUE_DOING]);
    shmcache_set_object_pool_context(&context->value_allocator.doing,
            &context->memory->value_allocator.doing,
            sizeof(struct shm_striping_allocator),
            ht_offsets[OFFSETS_INDEX_VA_POOL_OBJECT],
            context->memory->vm_info.striping.count.max,
            queue_base, false);

    queue_base = (int64_t *)(context->segments.hashtable.base +
            ht_offsets[OFFSETS_INDEX_VA_POOL_QUEUE_DONE]);
    shmcache_set_object_pool_context(&context->value_allocator.done,
            &context->memory->value_allocator.done,
            sizeof(struct shm_striping_allocator),
            ht_offsets[OFFSETS_INDEX_VA_POOL_OBJECT],
            context->memory->vm_info.striping.count.max,
            queue_base, false);

	return 0;
}

static int shmcache_do_lock_init(struct shmcache_context *context,
        const int ht_capacity, struct shm_value_size_info *segment,
        struct shm_value_size_info *striping, int64_t *ht_offsets)
{
    int result;

    if ((result=shm_lock_file(context)) != 0) {
        return result;
    }

    do {
        if (context->memory->status == SHMCACHE_STATUS_NORMAL) {
            result = -EEXIST;
            break;
        }

        context->memory->vm_info.segment = *segment;
        context->memory->vm_info.striping = *striping;

        shm_ht_init(context, ht_capacity);
        shm_list_init(&context->list);
        if ((result=shmcache_do_init(context, ht_offsets)) != 0) {
            break;
        }

        if ((result=shmopt_create_value_segment(context)) != 0) {
            break;
        }
        context->memory->status = SHMCACHE_STATUS_NORMAL;

        logInfo("file: "__FILE__", line: %d, pid: %d, "
                "init share memory first time, "
                "hashtable segment size: %"PRId64, __LINE__,
                context->pid, context->segments.hashtable.size);
    } while (0);

    shm_unlock_file(context);
    return result;
}

static void shmcache_set_obj_allocators(struct shmcache_context *context,
        const int64_t *ht_offsets)
{
    int64_t *queue_base;

    queue_base = (int64_t *)(context->segments.hashtable.base +
            ht_offsets[OFFSETS_INDEX_HT_POOL_QUEUE]);
    shm_object_pool_set(&context->hentry_allocator,
            &context->memory->hentry_obj_pool, queue_base);

    queue_base = (int64_t *)(context->segments.hashtable.base +
            ht_offsets[OFFSETS_INDEX_VA_POOL_QUEUE_DOING]);
    shm_object_pool_set(&context->value_allocator.doing,
            &context->memory->value_allocator.doing, queue_base);

    queue_base = (int64_t *)(context->segments.hashtable.base +
            ht_offsets[OFFSETS_INDEX_VA_POOL_QUEUE_DONE]);
    shm_object_pool_set(&context->value_allocator.done,
            &context->memory->value_allocator.done, queue_base);

    context->value_allocator.allocators = (struct shm_striping_allocator *)
        (context->segments.hashtable.base +
        ht_offsets[OFFSETS_INDEX_VA_POOL_OBJECT]);
}

int shmcache_init(struct shmcache_context *context,
		struct shmcache_config *config)
{
	int result;
    int ht_capacity;
    int bytes;
    int64_t ht_segment_size;
    struct shm_value_size_info segment;
    struct shm_value_size_info striping;
    int64_t ht_offsets[OFFSETS_COUNT];

    memset(context, 0, sizeof(*context));
    context->config = *config;
    context->pid = getpid();
    context->lock_fd = -1;

    ht_segment_size = shmcache_get_ht_segment_size(context,
            &segment, &striping, &ht_capacity, ht_offsets);
    if ((result=shmopt_init_segment(context, &context->segments.hashtable,
                    1, ht_segment_size)) != 0)
    {
        return result;
    }

    bytes = sizeof(struct shmcache_segment_info) * segment.count.max;
    context->segments.values.items = (struct shmcache_segment_info *)malloc(bytes);
    if (context->segments.values.items == NULL) {
        logError("file: "__FILE__", line: %d, "
                "malloc %d bytes fail", __LINE__, bytes);
        return ENOMEM;
    }
    memset(context->segments.values.items, 0, bytes);

    context->memory = (struct shm_memory_info *)context->segments.hashtable.base;
    shmcache_set_obj_allocators(context, ht_offsets);
    shm_list_set(&context->list, context->segments.hashtable.base,
            &context->memory->hashtable.head);

    if (context->memory->status == SHMCACHE_STATUS_INIT) {
        result = shmcache_do_lock_init(context, ht_capacity, &segment,
                &striping, ht_offsets);
        if (!(result == 0 || result == -EEXIST)) {
            return result;
        }
    } else if (context->memory->status != SHMCACHE_STATUS_NORMAL) {
        logError("file: "__FILE__", line: %d, "
                "share memory is invalid because status: 0x%08x != 0x%08x",
                __LINE__, context->memory->status, SHMCACHE_STATUS_NORMAL);
        return EINVAL;
    }

    if (context->config.lock_policy.trylock_interval_us > 0) {
        context->detect_deadlock_clocks = 1000 * context->config.
            lock_policy.detect_deadlock_interval_ms / context->config.
            lock_policy.trylock_interval_us;
    }
    result = shmopt_open_value_segments(context);

    logInfo("file: "__FILE__", line: %d, "
            "doing count: %d, done count: %d", __LINE__,
            shm_object_pool_get_count(&context->value_allocator.doing),
            shm_object_pool_get_count(&context->value_allocator.done));

    return result;
}

void shmcache_destroy(struct shmcache_context *context)
{
}

int shmcache_set(struct shmcache_context *context,
        const struct shmcache_buffer *key,
        const struct shmcache_buffer *value, const int ttl)
{
    int result;

    if ((result=shm_lock(context)) != 0) {
        return result;
    }
    result = shm_ht_set(context, key, value, ttl);
    shm_unlock(context);
    return result;
}

int shmcache_get(struct shmcache_context *context,
        const struct shmcache_buffer *key,
        struct shmcache_buffer *value)
{
    return shm_ht_get(context, key, value);
}

int shmcache_delete(struct shmcache_context *context,
        const struct shmcache_buffer *key)
{
    int result;
    if ((result=shm_lock(context)) != 0) {
        return result;
    }
    result = shm_ht_delete(context, key);
    shm_unlock(context);
    return result;
}

