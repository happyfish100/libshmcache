//shmcache.c

#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include "logger.h"
#include "shm_hashtable.h"
#include "shm_object_pool.h"
#include "shm_op_wrapper.h"
#include "shmcache.h"

#define SHMCACE_MEM_ALIGN(x, align)  (((x) + (align - 1)) & (~(align - 1)))

//HT for HashTable, VA for Value Allocator
#define OFFSETS_INDEX_HT_BUCKETS            0
#define OFFSETS_INDEX_HT_POOL_QUEUE         1
#define OFFSETS_INDEX_HT_POOL_OBJECT        2
#define OFFSETS_INDEX_VA_POOL_QUEUE_FREE    3
#define OFFSETS_INDEX_VA_POOL_QUEUE_DOING   4
#define OFFSETS_INDEX_VA_POOL_QUEUE_DONE    5
#define OFFSETS_INDEX_VA_POOL_OBJECT        6
#define OFFSETS_COUNT                       7

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

    *ht_capacity = shm_ht_get_capacity(context->config.max_key_count);
    total_size = sizeof(struct shm_memory_info);

    ht_offsets[OFFSETS_INDEX_HT_BUCKETS] = total_size;
    total_size += shm_ht_get_memory_size(*ht_capacity);

    ht_offsets[OFFSETS_INDEX_HT_POOL_QUEUE] = total_size;
    total_size += shm_object_pool_get_queue_memory_size(
            context->config.max_key_count);

    ht_offsets[OFFSETS_INDEX_HT_POOL_OBJECT] = total_size;
    total_size += shm_object_pool_get_object_memory_size(
            sizeof(struct shm_hash_entry), context->config.max_key_count);

    va_pool_queue_memory_size = shm_object_pool_get_queue_memory_size(
            striping->count.max);

    ht_offsets[OFFSETS_INDEX_VA_POOL_QUEUE_FREE] = total_size;
    total_size += va_pool_queue_memory_size;

    ht_offsets[OFFSETS_INDEX_VA_POOL_QUEUE_DOING] = total_size;
    total_size += va_pool_queue_memory_size;

    ht_offsets[OFFSETS_INDEX_VA_POOL_QUEUE_DONE] = total_size;
    total_size += va_pool_queue_memory_size;

    ht_offsets[OFFSETS_INDEX_VA_POOL_OBJECT] = total_size;
    total_size += shm_object_pool_get_object_memory_size(
            sizeof(struct shm_striping_allocator), striping->count.max);

    return total_size;
}

static int shmcache_init_pthread_mutex(pthread_mutex_t *plock)
{
	pthread_mutexattr_t mat;
	int result;

	if ((result=pthread_mutexattr_init(&mat)) != 0) {
		logError("file: "__FILE__", line: %d, "
			"call pthread_mutexattr_init fail, "
			"errno: %d, error info: %s",
			__LINE__, result, STRERROR(result));
		return result;
	}
	if ((result=pthread_mutexattr_setpshared(&mat,
			PTHREAD_PROCESS_SHARED)) != 0)
	{
		logError("file: "__FILE__", line: %d, "
			"call pthread_mutexattr_setpshared fail, "
			"errno: %d, error info: %s",
			__LINE__, result, STRERROR(result));
		return result;
	}
	if ((result=pthread_mutexattr_settype(&mat,
			PTHREAD_MUTEX_ERRORCHECK)) != 0)
	{
		logError("file: "__FILE__", line: %d, "
			"call pthread_mutexattr_settype fail, "
			"errno: %d, error info: %s",
			__LINE__, result, STRERROR(result));
		return result;
	}
	if ((result=pthread_mutex_init(plock, &mat)) != 0) {
		logError("file: "__FILE__", line: %d, "
			"call pthread_mutex_init fail, "
			"errno: %d, error info: %s",
			__LINE__, result, STRERROR(result));
		return result;
	}
	if ((result=pthread_mutexattr_destroy(&mat)) != 0) {
		logError("file: "__FILE__", line: %d, "
			"call thread_mutexattr_destroy fail, "
			"errno: %d, error info: %s",
			__LINE__, result, STRERROR(result));
		return result;
	}

	return 0;
}

static int shmcache_do_init(struct shmcache_context *context,
        int64_t *ht_offsets)
{
	int result;
    int64_t *offsets;

    if ((result=shmcache_init_pthread_mutex(&context->memory->lock)) != 0) {
        return result;
    }

    context->memory->hentry_obj_pool.object.element_size =
        sizeof(struct shm_hash_entry);
    context->memory->hentry_obj_pool.object.base_offset =
        ht_offsets[OFFSETS_INDEX_HT_POOL_OBJECT];
    context->memory->hentry_obj_pool.queue.capacity =
        context->config.max_key_count;

    offsets = (int64_t *)(context->hashtable.base +
            ht_offsets[OFFSETS_INDEX_HT_POOL_QUEUE]);
    shm_object_pool_set(&context->hash_op_context,
            &context->memory->hentry_obj_pool, offsets);
    shm_object_pool_init_full(&context->hash_op_context);

	return 0;
}

int shmcache_init(struct shmcache_context *context,
		struct shmcache_config *config)
{
    int ht_capacity;
    int64_t ht_segment_size;
    struct shm_value_size_info segment;
    struct shm_value_size_info striping;
	int result;
    int64_t ht_offsets[OFFSETS_COUNT];

    memset(context, 0, sizeof(*context));
    context->config = *config;

    ht_segment_size = shmcache_get_ht_segment_size(context,
            &segment, &striping, &ht_capacity, ht_offsets);
  
    context->hashtable.base = shm_mmap(config->type, config->filename,
             1, ht_segment_size, &context->hashtable.key);
    if (context->hashtable.base == NULL) {
        return ENOMEM;
    }

    context->memory = (struct shm_memory_info *)context->hashtable.base;
    if (context->memory->status == SHMCACHE_STATUS_INIT) {
        context->memory->vm_info.segment = segment;
        context->memory->vm_info.striping = striping;

        shm_ht_init(context, ht_capacity);
        if ((result=shmcache_do_init(context, ht_offsets)) != 0) {
            return result;
        }
        context->memory->status = SHMCACHE_STATUS_NORMAL;
    } else if (context->memory->status != SHMCACHE_STATUS_NORMAL) {
        logError("file: "__FILE__", line: %d, "
                "share memory is invalid because status: 0x%08x != 0x%08x",
                __LINE__, context->memory->status, SHMCACHE_STATUS_NORMAL);
        return EINVAL;
    }

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
