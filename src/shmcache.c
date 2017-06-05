//shmcache.c

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <pthread.h>
#include "logger.h"
#include "shared_func.h"
#include "ini_file_reader.h"
#include "sched_thread.h"
#include "shm_object_pool.h"
#include "shm_striping_allocator.h"
#include "shm_op_wrapper.h"
#include "shmopt.h"
#include "shm_list.h"
#include "shm_lock.h"
#include "shmcache.h"

#define SHMCACE_MEM_ALIGN(x, align)  (((x) + (align - 1)) & (~(align - 1)))

//HT for HashTable, VA for Value Allocator
#define OFFSETS_INDEX_HT_BUCKETS            0
#define OFFSETS_INDEX_HT_POOL_QUEUE         1
#define OFFSETS_INDEX_VA_POOL_QUEUE_DOING   2
#define OFFSETS_INDEX_VA_POOL_QUEUE_DONE    3
#define OFFSETS_INDEX_VA_POOL_OBJECT        4
#define OFFSETS_COUNT                       5

#define SHM_HASH_TABLE_PROJ_ID      1

#define MAX_KEYS_IN_SHM(context) context->memory->max_key_count

static void get_value_segment_count_size(struct shmcache_config *config,
        const int64_t value_max_memory, struct shm_value_size_info *segment)
{
    int page_size;

    page_size = getpagesize();
    segment->size = SHMCACE_MEM_ALIGN(config->segment_size, page_size);
    segment->count.max = value_max_memory / segment->size;
    if (segment->count.max == 0) {
        segment->count.max = 1;
    }
    segment->count.current = 0;
}

static void get_value_striping_count_size(
        struct shmcache_config *config,
        const int64_t value_max_memory,
        struct shm_value_size_info *segment,
        struct shm_value_size_info *striping)
{
    int page_size;
    int mb_count;

    get_value_segment_count_size(config, value_max_memory, segment);

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
    if (striping->size < config->max_value_size * 2) {
        striping->size = config->max_value_size * 2;
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

    get_value_striping_count_size(&context->config, context->config.max_memory,
            segment, striping);

    *ht_capacity = shm_ht_get_capacity(context->config.max_key_count + 1);
    total_size = sizeof(struct shm_memory_info);

    logDebug("ht capacity: %d, sizeof(struct shm_memory_info): %d, "
            "context->config.max_key_count: %d, striping->count.max: %d",
            *ht_capacity, (int)sizeof(struct shm_memory_info),
            context->config.max_key_count, striping->count.max);

    ht_offsets[OFFSETS_INDEX_HT_BUCKETS] = total_size;
    total_size += shm_ht_get_memory_size(*ht_capacity);

    ht_offsets[OFFSETS_INDEX_HT_POOL_QUEUE] = total_size;
    total_size += shm_object_pool_get_queue_memory_size(
            context->config.max_key_count + 1);

    va_pool_queue_memory_size = shm_object_pool_get_queue_memory_size(
            striping->count.max + 1);

    ht_offsets[OFFSETS_INDEX_VA_POOL_QUEUE_DOING] = total_size;
    total_size += va_pool_queue_memory_size;

    ht_offsets[OFFSETS_INDEX_VA_POOL_QUEUE_DONE] = total_size;
    total_size += va_pool_queue_memory_size;

    ht_offsets[OFFSETS_INDEX_VA_POOL_OBJECT] = total_size;
    total_size += shm_object_pool_get_object_memory_size(
            sizeof(struct shm_striping_allocator), striping->count.max);

    get_value_striping_count_size(&context->config,
            context->config.max_memory - total_size,
            segment, striping);
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
            ht_offsets[OFFSETS_INDEX_VA_POOL_QUEUE_DOING]);
    shmcache_set_object_pool_context(&context->value_allocator.doing,
            &context->memory->value_allocator.doing,
            sizeof(struct shm_striping_allocator),
            ht_offsets[OFFSETS_INDEX_VA_POOL_OBJECT],
            context->memory->vm_info.striping.count.max + 1,
            queue_base, false);

    queue_base = (int64_t *)(context->segments.hashtable.base +
            ht_offsets[OFFSETS_INDEX_VA_POOL_QUEUE_DONE]);
    shmcache_set_object_pool_context(&context->value_allocator.done,
            &context->memory->value_allocator.done,
            sizeof(struct shm_striping_allocator),
            ht_offsets[OFFSETS_INDEX_VA_POOL_OBJECT],
            context->memory->vm_info.striping.count.max + 1,
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
        shm_list_init(context);
        if ((result=shmcache_do_init(context, ht_offsets)) != 0) {
            break;
        }

        context->memory->init_time = context->memory->stats.init_time =
            context->memory->stats.last.calc_time = get_current_time();
        context->memory->usage.alloced = context->segments.hashtable.size;
        context->memory->usage.used.common = context->segments.hashtable.size;
        if ((result=shmopt_create_value_segment(context)) != 0) {
            break;
        }
        context->memory->size = sizeof(struct shm_memory_info);
        context->memory->max_key_count = context->config.max_key_count;
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

#if 0
    {
        struct shm_striping_allocator *allocator;
        struct shm_striping_allocator *end;
        int64_t bytes;

    bytes = 0;
        end = context->value_allocator.allocators +
            context->memory->vm_info.striping.count.current;
        for (allocator=context->value_allocator.allocators; allocator<end; allocator++) {
            logInfo("#%d  allocator %ld in which: %d, base: %"PRId64", free: %"PRId64", used: %"PRId64,
                    (int)(allocator - context->value_allocator.allocators) + 1,
                    (char *)allocator - context->segments.hashtable.base,
                    allocator->in_which_pool, allocator->offset.base,
                    shm_striping_allocator_free_size(allocator),
                    shm_striping_allocator_used_size(allocator));
            bytes += allocator->size.used;
        }
        logInfo("striping used bytes: %"PRId64, bytes);
    }
#endif
}

#if 0
static void print_value_allocator(struct shmcache_context *context,
        struct shmcache_object_pool_context *op)
{
    int64_t allocator_offset;
    struct shm_striping_allocator *allocator;

    allocator_offset = shm_object_pool_first(op);
    while (allocator_offset > 0) {
        allocator = (struct shm_striping_allocator *)(context->segments.
                hashtable.base + allocator_offset);

        logInfo("allocator %"PRId64" last_alloc_time: %d, fail_times: %d, in_which_pool: %d, "
                "segment: %d, striping: %d, base: %"PRId64
                ", total: %d, used: %d",
                allocator_offset, allocator->last_alloc_time,
                allocator->fail_times, allocator->in_which_pool,
                allocator->index.segment, allocator->index.striping,
                allocator->offset.base, allocator->size.total,
                allocator->size.used);

        allocator_offset = shm_object_pool_next(op);
    }
}
#endif


static int shmcache_check_segement(struct shm_value_size_info *shm,
        struct shm_value_size_info *cfg, const char *label)
{
    if (shm->size != cfg->size) {
        logError("file: "__FILE__", line: %d, "
                "shm %s size: %"PRId64" != calculated by "
                "config: %"PRId64", maybe config max_memory "
                "or segment_size changed", __LINE__, label,
                shm->size, cfg->size);
        return EINVAL;
    }

    if (shm->count.max != cfg->count.max) {
        logError("file: "__FILE__", line: %d, "
                "shm %s max count: %d !=  calculated by "
                "config count: %d, maybe config max_memory "
                "or segment_size changed", __LINE__, label,
                shm->count.max, cfg->count.max);
        return EINVAL;
    }
    return 0;
}

static int shmcache_check(struct shmcache_context *context,
        struct shm_value_size_info *segment,
        struct shm_value_size_info *striping)
{
    int result;

    if (context->memory->size != (int)sizeof(struct shm_memory_info)) {
        logError("file: "__FILE__", line: %d, "
                "share memory is invalid because size: %d != "
                "sizeof(struct shm_memory_info): %d",
                __LINE__, context->memory->size,
                (int)sizeof(struct shm_memory_info));
        return EINVAL;
    }

    if (context->memory->status != SHMCACHE_STATUS_NORMAL) {
        logError("file: "__FILE__", line: %d, "
                "share memory is invalid because status: 0x%08x != 0x%08x",
                __LINE__, context->memory->status, SHMCACHE_STATUS_NORMAL);
        return EINVAL;
    }

    if (context->config.max_key_count != MAX_KEYS_IN_SHM(context)) {
        logError("file: "__FILE__", line: %d, "
                "shm prealloced entry count: %d != max_key_count: %d",
                __LINE__, MAX_KEYS_IN_SHM(context),
                context->config.max_key_count);
        return EINVAL;
    }

    if ((result=shmcache_check_segement(&context->memory->vm_info.segment,
                    segment, "segment")) != 0)
    {
        return result;
    }

    if ((result=shmcache_check_segement(&context->memory->vm_info.striping,
                    striping, "striping")) != 0)
    {
        return result;
    }
    return 0;
}

int shmcache_init(struct shmcache_context *context,
		struct shmcache_config *config, const bool create_segment,
        const bool check_segment)
{
	int result;
    int ht_capacity;
    int bytes;
    bool ht_segemnt_exists;
    int64_t ht_segment_size;
    struct shm_value_size_info segment;
    struct shm_value_size_info striping;
    int64_t ht_offsets[OFFSETS_COUNT];

    memset(context, 0, sizeof(*context));
    context->config = *config;
    context->pid = getpid();
    context->lock_fd = -1;
    context->create_segment = create_segment;

    ht_segment_size = shmcache_get_ht_segment_size(context,
            &segment, &striping, &ht_capacity, ht_offsets);

    ht_segemnt_exists = shm_exists(context->config.type,
            context->config.filename, SHM_HASH_TABLE_PROJ_ID);
    if ((result=shmopt_init_segment(context, &context->segments.hashtable,
                    SHM_HASH_TABLE_PROJ_ID, ht_segment_size)) != 0)
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
    shm_list_set(context, context->segments.hashtable.base,
            &context->memory->hashtable.head);
    shmcache_set_obj_allocators(context, ht_offsets);
    if (ht_segemnt_exists && check_segment &&
            (result=shmcache_check(context, &segment, &striping)) != 0)
    {
        return result;
    }

    if (create_segment) {
        if (context->memory->status == SHMCACHE_STATUS_INIT) {
            result = shmcache_do_lock_init(context, ht_capacity, &segment,
                    &striping, ht_offsets);
            if (!(result == 0 || result == -EEXIST)) {
                return result;
            }
        }
        result = shmopt_open_value_segments(context);
        if (result == 0 && context->memory->vm_info.segment.count.current <
                context->memory->vm_info.segment.count.max)
        {
            shm_lock(context);
            while (context->segments.hashtable.size +
                context->memory->vm_info.segment.size *
                context->memory->vm_info.segment.count.current < config->min_memory)
            {
                if ((result=shmopt_create_value_segment(context)) != 0) {
                    break;
                }
                if (context->memory->vm_info.segment.count.current >=
                        context->memory->vm_info.segment.count.max)
                {
                    break;
                }
            }
            shm_unlock(context);
        }
    }

    if (context->config.lock_policy.trylock_interval_us > 0) {
        context->detect_deadlock_clocks = 1000 * context->config.
            lock_policy.detect_deadlock_interval_ms / context->config.
            lock_policy.trylock_interval_us;
    }
    logDebug("file: "__FILE__", line: %d, "
            "doing count: %d, done count: %d, "
            "total entry count: %d", __LINE__,
            shm_object_pool_get_count(&context->value_allocator.doing),
            shm_object_pool_get_count(&context->value_allocator.done),
            MAX_KEYS_IN_SHM(context));

#if 0
    print_value_allocator(context, &context->value_allocator.doing);
    print_value_allocator(context, &context->value_allocator.done);

    {
        struct shm_hash_entry *current;
        struct shmcache_key_info key;
        struct shmcache_value_info value;
        int64_t bytes;
        int ii;

        logInfo("list count: %d", shm_list_count(context));
        ii = 0;
        bytes = 0;
        SHM_LIST_FOR_EACH(context, current, list) {

            key.data = current->key;
            key.length = current->key_len;

            if (shm_ht_get(context, &key, &value) != 0) {
                logError("#%d. shm_ht_get key: %.*s fail, offset: %"PRId64
                        ", value striping: %d, value offset: %"PRId64", size: %d",
                        ii, key.length, key.data,
                        (char *)current - context->segments.hashtable.base,
                        current->value.index.striping, current->value.offset,
                        current->value.length);
                break;
            } else {
                bytes += value.length;
            }
            ii++;

            /*
            logInfo("segment: %d, striping: %d, offset: %"PRId64
                    ", size: %d, length: %d",
                    current->value.index.segment,
                    current->value.index.striping, 
                    current->value.offset, 
                    current->value.size, 
                    current->value.length);
            break;
            */
        }
        logInfo("hash table used bytes: %"PRId64, bytes);
    }

    {
        unsigned int index;
        int k;
        int64_t entry_offset;
        struct shm_hash_entry *entry;

        k = 0;
        for (index=0; index<context->memory->hashtable.capacity; index++) {
            entry_offset = context->memory->hashtable.buckets[index];
            while (entry_offset > 0) {
                logInfo("%d. %"PRId64, k++, entry_offset);
                entry = HT_ENTRY_PTR(context, entry_offset);
                entry_offset = entry->ht_next;
            }
        }
    }

#endif

    return result;
}

static int64_t shmcache_parse_bytes(IniContext *iniContext,
        const char *config_filename, const char *name, int *result)
{
    char *value;
    int64_t size;

    value = iniGetStrValue(NULL, name, iniContext);
    if (value == NULL) {
        logError("file: "__FILE__", line: %d, "
                "config file: %s, item \"%s\" is not exists",
                __LINE__, config_filename, name);
        *result = ENOENT;
        return -1;
    }
    if ((*result=parse_bytes(value, 1, &size)) != 0) {
        return -1;
    }
    if (size <= 0) {
        logError("file: "__FILE__", line: %d, "
                "config file: %s, item \"%s\": %"PRId64" is invalid",
                __LINE__, config_filename, name, size);
        *result = EINVAL;
        return -1;
    }
    return size;
}

static int64_t shmcache_parse_bytes_with_default(IniContext *iniContext,
        const char *config_filename, const char *name,
        const int64_t def_value, int *result)
{
    char *value;
    int64_t size;

    value = iniGetStrValue(NULL, name, iniContext);
    if (value == NULL) {
        *result = 0;
        return def_value;
    }
    if ((*result=parse_bytes(value, 1, &size)) != 0) {
        return def_value;
    }
    return size;
}

int shmcache_load_config(struct shmcache_config *config,
		const char *config_filename)
{
    int result;
    IniContext iniContext;
    char *type;
    char *filename;
    char *hash_function;

    if ((result=iniLoadFromFile(config_filename, &iniContext)) != 0) {
        return result;
    }

    do {
        type = iniGetStrValue(NULL, "type", &iniContext);
        if (type == NULL || strcasecmp(type, "shm") == 0) {
            config->type = SHMCACHE_TYPE_SHM;
        } else {
            config->type = SHMCACHE_TYPE_MMAP;
        }

        filename = iniGetStrValue(NULL, "filename", &iniContext);
        if (filename == NULL || *filename == '\0') {
            logError("file: "__FILE__", line: %d, "
                    "config file: %s, item \"filename\" is not exists",
                    __LINE__, config_filename);
            result = ENOENT;
            break;
        }
        snprintf(config->filename, sizeof(config->filename),
                "%s", filename);

        config->max_memory = shmcache_parse_bytes(&iniContext,
                config_filename, "max_memory", &result);
        if (result != 0) {
            break;
        }

        config->min_memory = shmcache_parse_bytes_with_default(&iniContext,
                config_filename, "min_memory", 0, &result);

        config->segment_size = shmcache_parse_bytes(&iniContext,
                config_filename, "segment_size", &result);
        if (result != 0) {
            break;
        }
        if (config->max_memory / config->segment_size > 255) {
            int64_t segment_size;
            segment_size = config->max_memory / 255;
            logWarning("file: "__FILE__", line: %d, "
                    "config file: %s, segment_size: %"PRId64
                    " is too small, set to %"PRId64,
                    __LINE__, config_filename,
                    config->segment_size, segment_size);
            config->segment_size = segment_size;
        }

        config->max_key_count = iniGetIntValue(NULL, "max_key_count",
                &iniContext, 0);
        if (config->max_key_count <= 0) {
            logError("file: "__FILE__", line: %d, "
                    "config file: %s, item \"max_key_count\" "
                    "is not exists or invalid",
                    __LINE__, config_filename);
            result = EINVAL;
            break;
        }

        config->max_value_size = shmcache_parse_bytes(&iniContext,
                config_filename, "max_value_size", &result);
        if (result != 0) {
            break;
        }

        hash_function = iniGetStrValue(NULL, "hash_function", &iniContext);
        if (hash_function == NULL || *hash_function  == '\0') {
            config->hash_func = simple_hash;
        } else {
            void *handle;
            handle = dlopen(NULL, RTLD_LAZY);
            if (handle == NULL) {
                logError("file: "__FILE__", line: %d, "
                        "call dlopen fail, error: %s",
                        __LINE__, dlerror());
                result = EBUSY;
                break;
            }
            config->hash_func = (HashFunc)dlsym(handle, hash_function);
            if (config->hash_func == NULL) {
                logError("file: "__FILE__", line: %d, "
                        "call dlsym %s fail, error: %s",
                        __LINE__, hash_function, dlerror());
                result = ENOENT;
                break;
            }
            dlclose(handle);
        }

        config->va_policy.avg_key_ttl = iniGetIntValue(NULL,
                "value_policy.avg_key_ttl", &iniContext, 0);

        config->va_policy.discard_memory_size = shmcache_parse_bytes(
                &iniContext, config_filename,
                "value_policy.discard_memory_size", &result);
        if (result != 0) {
            break;
        }

        config->va_policy.max_fail_times = iniGetIntValue(NULL,
                "value_policy.max_fail_times", &iniContext, 5);
        config->lock_policy.trylock_interval_us = iniGetIntValue(NULL,
                "lock_policy.trylock_interval_us", &iniContext, 200);
        if (config->lock_policy.trylock_interval_us <= 0) {
            logError("file: "__FILE__", line: %d, "
                    "config file: %s, item "
                    "\"lock_policy.trylock_interval_us\" "
                    "is not exists or invalid",
                    __LINE__, config_filename);
            result = EINVAL;
            break;
        }

        config->va_policy.sleep_us_when_recycle_valid_entries = iniGetIntValue(NULL,
                "value_policy.sleep_us_when_recycle_valid_entries", &iniContext, 0);

        config->lock_policy.detect_deadlock_interval_ms = iniGetIntValue(
                NULL, "lock_policy.detect_deadlock_interval_ms",
                &iniContext, 1000);
        if (config->lock_policy.detect_deadlock_interval_ms <= 0) {
            logError("file: "__FILE__", line: %d, "
                    "config file: %s, item "
                    "\"lock_policy.detect_deadlock_interval_ms\" "
                    "is not exists or invalid",
                    __LINE__, config_filename);
            result = EINVAL;
            break;
        }
        config->recycle_key_once = iniGetIntValue(NULL,
                "recycle_key_once", &iniContext, 0);
        if (config->recycle_key_once <= 0) {
            config->recycle_key_once = -1;
        }
        load_log_level(&iniContext);
    } while (0);

    iniFreeContext(&iniContext);
    return result;
}

int shmcache_init_from_file_ex(struct shmcache_context *context,
		const char *config_filename, const bool create_segment,
        const bool check_segment)
{
    int result;
    struct shmcache_config config;

    if ((result=shmcache_load_config(&config, config_filename)) != 0) {
        return result;
    }

    if ((result=shmcache_init(context, &config,
                    create_segment, check_segment)) == EINVAL)
    {
        logError("file: "__FILE__", line: %d, "
                "maybe memory related config parameters changed, "
                "clear all share memory command: "
                "shmcache_remove_all %s", __LINE__, config_filename);
    }
    return result;
}

void shmcache_destroy(struct shmcache_context *context)
{
    int index;

    for (index=0; index < context->segments.values.count; index++) {
        if (context->segments.values.items[index].base != NULL) {
            shm_munmap(context->config.type,
                    context->segments.values.items[index].base,
                    context->segments.values.items[index].size);
            context->segments.values.items[index].base = NULL;
        }
    }

    if (context->segments.hashtable.base != NULL) {
        shm_munmap(context->config.type,
                context->segments.hashtable.base,
                context->segments.hashtable.size);
        context->segments.hashtable.base = NULL;
    }
}

int shmcache_set_ex(struct shmcache_context *context,
        const struct shmcache_key_info *key,
        const struct shmcache_value_info *value)
{
    int result;

    if ((result=shm_lock(context)) != 0) {
        return result;
    }
    context->memory->stats.hashtable.set.total++;
    result = shm_ht_set(context, key, value);
    if (result == 0) {
        context->memory->stats.hashtable.set.success++;
    }
    shm_unlock(context);
    return result;
}

int shmcache_set(struct shmcache_context *context,
        const struct shmcache_key_info *key,
        const char *data, const int data_len, const int ttl)
{
    struct shmcache_value_info value;
    value.options = SHMCACHE_SERIALIZER_STRING;
    value.data = (char *)data;
    value.length = data_len;
    value.expires = HT_CALC_EXPIRES(get_current_time(), ttl);
    return shmcache_set_ex(context, key, &value);
}

int shmcache_get(struct shmcache_context *context,
        const struct shmcache_key_info *key,
        struct shmcache_value_info *value)
{
    int result;

    __sync_add_and_fetch(&context->memory->stats.hashtable.get.total, 1);
    result = shm_ht_get(context, key, value);
    if (result == 0) {
        __sync_add_and_fetch(&context->memory->stats.hashtable.get.success, 1);
    }
    return result;
}

int shmcache_delete(struct shmcache_context *context,
        const struct shmcache_key_info *key)
{
    int result;
    if ((result=shm_lock(context)) != 0) {
        return result;
    }
    context->memory->stats.hashtable.del.total++;
    result = shm_ht_delete(context, key);
    if (result == 0) {
        context->memory->stats.hashtable.del.success++;
    }
    shm_unlock(context);
    return result;
}

int shmcache_set_ttl(struct shmcache_context *context,
        const struct shmcache_key_info *key, const int ttl)
{
    int result;
    if (ttl < 0) {
        return EINVAL;
    }
    if ((result=shm_lock(context)) != 0) {
        return result;
    }
    result = shm_ht_set_expires(context, key,
            HT_CALC_EXPIRES(get_current_time(), ttl));
    shm_unlock(context);
    return result;
}

int shmcache_set_expires(struct shmcache_context *context,
        const struct shmcache_key_info *key, const int expires)
{
    int result;
    if (expires > 0 && expires < get_current_time()) {
        return EINVAL;
    }
    if ((result=shm_lock(context)) != 0) {
        return result;
    }
    result = shm_ht_set_expires(context, key, expires);
    shm_unlock(context);
    return result;
}

int shmcache_incr(struct shmcache_context *context,
        const struct shmcache_key_info *key,
        const int64_t increment,
        const int ttl, int64_t *new_value)
{
    int result;
    struct shmcache_value_info value;
    char *endptr;
    char buff[24];

    if ((result=shm_lock(context)) != 0) {
        return result;
    }

    do {
        result = shm_ht_get(context, key, &value);
        if (result == 0) {
            if (value.length >= sizeof(buff)) {
                logError("file: "__FILE__", line: %d, "
                        "key: %.*s, value length: %d exceeds %d",
                        __LINE__, key->length, key->data,
                        value.length, (int)sizeof(buff));
                result = EINVAL;
                break;
            }
            memcpy(buff, value.data, value.length);
            buff[value.length] = '\0';
            endptr = NULL;
            *new_value = strtoll(buff, &endptr, 10);
            if (endptr != NULL && *endptr != '\0') {
                logError("file: "__FILE__", line: %d, "
                        "key: %.*s, value length: %d, "
                        "value: %s is not a valid integer",
                        __LINE__, key->length, key->data,
                        value.length, buff);
                result = EINVAL;
                break;
            }
            *new_value += increment;
        } else {
            *new_value = increment;
        }

        value.options = SHMCACHE_SERIALIZER_INTEGER;
        value.data = buff;
        value.length = sprintf(value.data, "%"PRId64, *new_value);
        value.expires = HT_CALC_EXPIRES(get_current_time(), ttl);
        result = shm_ht_set(context, key, &value);
    } while (0);

    context->memory->stats.hashtable.incr.total++;
    if (result == 0) {
        context->memory->stats.hashtable.incr.success++;
    }
    shm_unlock(context);
    return result;
}

int shmcache_remove_all(struct shmcache_context *context)
{
    int result;

    if ((result=shm_lock_file(context)) != 0) {
        return result;
    }

    result = shmopt_remove_all(context);
    shm_unlock_file(context);
    return result;
}

void shmcache_stats_ex(struct shmcache_context *context, struct shmcache_stats *stats,
        const bool calc_hit_ratio)
{
    int64_t total_delta;
    int64_t success_delta;

    stats->shm = context->memory->stats;
    stats->memory.max = context->segments.hashtable.size +
        context->memory->vm_info.segment.size *
        context->memory->vm_info.segment.count.max;
    stats->memory.used = context->memory->usage.used.common +
        context->memory->usage.used.entry;
    stats->memory.usage = context->memory->usage;
    stats->hashtable.count = context->memory->hashtable.count;
    stats->hashtable.segment_size = context->segments.hashtable.size;
    stats->max_key_count = MAX_KEYS_IN_SHM(context);

    if (calc_hit_ratio) {
        if (!g_schedule_flag) {
            g_current_time = time(NULL);
        }
        stats->hit.seconds = g_current_time - context->memory->stats.last.calc_time;
        total_delta = context->memory->stats.hashtable.get.total
            - context->memory->stats.last.get.total;
        if (total_delta > 0) {
            success_delta = context->memory->stats.hashtable.get.success
                - context->memory->stats.last.get.success;
            stats->hit.ratio = (double)success_delta / (double)total_delta;
            if (stats->hit.seconds > 0) {
                context->memory->stats.last.calc_time = g_current_time;
                context->memory->stats.last.get.total =
                    context->memory->stats.hashtable.get.total;
                context->memory->stats.last.get.success =
                    context->memory->stats.hashtable.get.success;
                stats->hit.get_qps = (double)total_delta / (double)stats->hit.seconds;
            } else {
                stats->hit.get_qps = total_delta;
            }
        } else {
            stats->hit.ratio = -1.00;
            stats->hit.get_qps = 0.00;
        }
    } else {
        stats->hit.seconds = 0;
        stats->hit.ratio = -1.00;
        stats->hit.get_qps = 0.00;
    }
}

void shmcache_clear_stats(struct shmcache_context *context)
{
    shm_lock(context);

    memset(&context->memory->stats, 0, sizeof(context->memory->stats));
    context->memory->stats.init_time =
        context->memory->stats.last.calc_time = get_current_time();

    shm_unlock(context);
}

const char *shmcache_get_serializer_label(const int serializer)
{
    switch (serializer) {
        case SHMCACHE_SERIALIZER_STRING:
            return "string";
        case SHMCACHE_SERIALIZER_INTEGER:
            return "integer";
        case SHMCACHE_SERIALIZER_NONE:
            return "none";
        case SHMCACHE_SERIALIZER_MSGPACK:
            return "msgpack";
        case SHMCACHE_SERIALIZER_IGBINARY:
            return "igbinary";
        case SHMCACHE_SERIALIZER_PHP:
            return "php";
        default:
            return "unkown";
    }
}

int shmcache_clear(struct shmcache_context *context)
{
    int result;

    if ((result=shm_lock(context)) != 0) {
        return result;
    }

    if (shm_ht_clear(context) > 0) {
        if (context->config.va_policy.sleep_us_when_recycle_valid_entries > 0) {
            usleep(context->config.va_policy.sleep_us_when_recycle_valid_entries);
        }
    }
    shm_unlock(context);
    return 0;
}
