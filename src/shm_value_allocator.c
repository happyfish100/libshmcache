//shm_value_allocator.c

#include <errno.h>
#include "sched_thread.h"
#include "shared_func.h"
#include "shm_object_pool.h"
#include "shm_striping_allocator.h"
#include "shm_list.h"
#include "shmopt.h"
#include "shm_hashtable.h"
#include "shm_value_allocator.h"

static struct shm_hash_entry *shm_value_striping_alloc(
        struct shmcache_context *context,
        struct shm_striping_allocator *allocator, const int size)
{
    int64_t offset;
    char *base;
    struct shm_hash_entry *entry;
    offset = shm_striping_allocator_alloc(allocator, size);
    if (offset < 0) {
        return NULL;
    }

    base = shmopt_get_value_segment(context, allocator->index.segment);
    if (base == NULL) {
        return NULL;
    }

    entry = (struct shm_hash_entry *)(base + offset);
    entry->memory.offset = offset;
    entry->memory.index = allocator->index;
    entry->memory.size = size;
    return entry;
}

static struct shm_hash_entry *shm_value_allocator_do_alloc(struct shmcache_context *context,
        const int size)
{
    int64_t allocator_offset;
    int64_t removed_offset;
    struct shm_striping_allocator *allocator;
    struct shm_hash_entry *entry;

    allocator_offset = shm_object_pool_first(&context->value_allocator.doing);
    while (allocator_offset > 0) {
        allocator = (struct shm_striping_allocator *)(context->segments.
                hashtable.base + allocator_offset);
        if ((entry=shm_value_striping_alloc(context, allocator, size)) != NULL) {
            context->memory->usage.used.entry += size;
            return entry;
        }

        if ((shm_striping_allocator_free_size(allocator) <= context->config.
                va_policy.discard_memory_size) || (allocator->fail_times >
                context->config.va_policy.max_fail_times))
        {
            removed_offset = shm_object_pool_remove(&context->value_allocator.doing);
            if (removed_offset == allocator_offset) {
                allocator->in_which_pool = SHMCACHE_STRIPING_ALLOCATOR_POOL_DONE;
                shm_object_pool_push(&context->value_allocator.done, allocator_offset);
            } else {
                logCrit("file: "__FILE__", line: %d, "
                        "shm_object_pool_remove fail, "
                        "offset: %"PRId64" != expect: %"PRId64, __LINE__,
                        removed_offset, allocator_offset);
            }
        }

        allocator_offset = shm_object_pool_next(&context->value_allocator.doing);
    }

    return NULL;
}


static int shm_value_allocator_do_recycle(struct shmcache_context *context,
        struct shm_striping_allocator *allocator)
{
    int64_t allocator_offset;
    if (allocator->in_which_pool == SHMCACHE_STRIPING_ALLOCATOR_POOL_DONE) {
        allocator_offset = (char *)allocator - context->segments.hashtable.base;
        if (shm_object_pool_remove_by(&context->value_allocator.done,
                    allocator_offset) >= 0) {
            allocator->in_which_pool = SHMCACHE_STRIPING_ALLOCATOR_POOL_DOING;
            shm_object_pool_push(&context->value_allocator.doing, allocator_offset);
        } else {
            logCrit("file: "__FILE__", line: %d, "
                    "shm_object_pool_remove_by fail, "
                    "index: %d, offset: %"PRId64, __LINE__,
                    allocator->index.striping, allocator_offset);
            return EFAULT;
        }
    }
    return 0;
}

int shm_value_allocator_recycle(struct shmcache_context *context,
        struct shm_recycle_stats *recycle_stats, const int recycle_keys_once)
{
    int64_t entry_offset;
    int64_t start_time;
    struct shm_hash_entry *entry;
    struct shmcache_key_info key;
    int result;
    int index;
    int clear_count;
    int valid_count;
    bool valid;
    bool recycled;

    result = ENOMEM;
    clear_count = valid_count = 0;
    start_time = get_current_time_us();
    g_current_time = start_time / 1000000;
    recycled = false;
    while ((entry_offset=shm_list_first(context)) > 0) {
        entry = shm_get_hentry_ptr(context, entry_offset);
        index = entry->memory.index.striping;
        key.data = entry->key;
        key.length = entry->key_len;
        valid = HT_ENTRY_IS_VALID(entry, g_current_time);
        if (shm_ht_delete_ex(context, &key, &recycled) != 0) {
            logError("file: "__FILE__", line: %d, "
                    "shm_ht_delete fail, index: %d, "
                    "entry offset: %"PRId64", "
                    "key: %.*s, key length: %d", __LINE__,
                    index, entry_offset, entry->key_len,
                    entry->key, entry->key_len);

            shm_ht_free_entry(context, entry, entry_offset, &recycled);
        }

        clear_count++;
        if (valid) {
            valid_count++;
        }

        if (recycle_keys_once > 0) {
            if (clear_count >= recycle_keys_once) {
                result = 0;
                break;
            }
        } else if (recycled) {
            logInfo("file: "__FILE__", line: %d, "
                    "recycle #%d striping memory, "
                    "clear total entries: %d, "
                    "clear valid entries: %d, "
                    "time used: %"PRId64" us", __LINE__,
                    index, clear_count, valid_count,
                    get_current_time_us() - start_time);
            result = 0;
            break;
        }
    }

    context->memory->stats.memory.clear_ht_entry.total += clear_count;
    if (valid_count > 0) {
        context->memory->stats.memory.clear_ht_entry.valid += valid_count;
    }

    recycle_stats->last_recycle_time = g_current_time;
    recycle_stats->total++;
    if (result == 0) {
        recycle_stats->success++;
        if (valid_count > 0) {
            recycle_stats->force++;
            if (context->config.va_policy.
                    sleep_us_when_recycle_valid_entries > 0)
            {
                usleep(context->config.va_policy.
                        sleep_us_when_recycle_valid_entries);
            }
        }
    } else {
        logError("file: "__FILE__", line: %d, "
                "unable to recycle memory, "
                "clear total entries: %d, "
                "cleared valid entries: %d, "
                "time used: %"PRId64" us", __LINE__,
                __LINE__, clear_count, valid_count,
                get_current_time_us() - start_time);
    }
    return result;
}

struct shm_hash_entry *shm_value_allocator_alloc(struct shmcache_context *context,
        const int key_len, const int value_len)
{
    int result;
    int size;
    bool recycle;
    int64_t allocator_offset;
    struct shm_striping_allocator *allocator;
    struct shm_hash_entry *entry;

    size = sizeof(struct shm_hash_entry) + MEM_ALIGN(key_len) + MEM_ALIGN(value_len);
    if ((entry=shm_value_allocator_do_alloc(context, size)) != NULL) {
        return entry;
    }

    if (context->memory->vm_info.segment.count.current >=
            context->memory->vm_info.segment.count.max)
    {
        recycle = true;
    } else {
        allocator_offset = shm_object_pool_first(&context->value_allocator.done);
        if (allocator_offset > 0) {
            allocator = (struct shm_striping_allocator *)(context->segments.
                    hashtable.base + allocator_offset);
            recycle = (context->config.va_policy.avg_key_ttl > 0 && g_current_time -
                    allocator->last_alloc_time >= context->config.va_policy.avg_key_ttl);
        } else {
            recycle = false;
        }
    }

    if (recycle ) {
        result = shm_value_allocator_recycle(context,
                &context->memory->stats.memory.recycle.value_striping, -1);
    } else {
        result = shmopt_create_value_segment(context);
    }
    if (result == 0) {
        entry  = shm_value_allocator_do_alloc(context, size);
    }
    if (entry == NULL) {
        logError("file: "__FILE__", line: %d, "
                "malloc %d bytes from shm fail", __LINE__, size);
    }
    return entry;
}

int shm_value_allocator_free(struct shmcache_context *context,
        struct shm_hash_entry *entry, bool *recycled)
{
    struct shm_striping_allocator *allocator;
    int64_t used;

    allocator = context->value_allocator.allocators + entry->memory.index.striping;
    used = shm_striping_allocator_free(allocator, entry->memory.size);
    context->memory->usage.used.entry -= entry->memory.size;
    if (used <= 0) {
        if (used < 0) {
            logError("file: "__FILE__", line: %d, "
                    "striping used memory: %"PRId64" < 0, "
                    "segment: %d, striping: %d, offset: %"PRId64", size: %d",
                    __LINE__, used, entry->memory.index.segment,
                    entry->memory.index.striping, entry->memory.offset,
                    entry->memory.size);
        }
        *recycled = true;
        shm_striping_allocator_reset(allocator);
        return shm_value_allocator_do_recycle(context, allocator);
    }

    return 0;
}
