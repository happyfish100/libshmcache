//shm_value_allocator.c

#include <errno.h>
#include <assert.h>
#include "sched_thread.h"
#include "shm_object_pool.h"
#include "shm_striping_allocator.h"
#include "shm_list.h"
#include "shmopt.h"
#include "shm_hashtable.h"
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
    int64_t removed_offset;
    struct shm_striping_allocator *allocator;

    allocator_offset = shm_object_pool_first(&context->value_allocator.doing);
    while (allocator_offset > 0) {
        allocator = (struct shm_striping_allocator *)(context->segments.
                hashtable.base + allocator_offset);
        if (shm_value_striping_alloc(allocator, size, value) == 0) {
            context->memory->usage.used += size;
            return 0;
        }

        if ((shm_striping_allocator_free_size(allocator) <= context->config.
                va_policy.discard_memory_size) || (allocator->fail_times >
                context->config.va_policy.max_fail_times))
        {
            removed_offset = shm_object_pool_remove(&context->value_allocator.doing);
            if (removed_offset == allocator_offset) {
                allocator->in_which_pool = SHMCACHE_STRIPING_ALLOCATOR_POOL_DONE;
                assert(shm_object_pool_push(&context->value_allocator.done, allocator_offset) == 0);
            } else {
                logCrit("file: "__FILE__", line: %d, "
                        "shm_object_pool_remove fail, "
                        "offset: %"PRId64" != expect: %"PRId64, __LINE__,
                        removed_offset, allocator_offset);
            }
        }

        allocator_offset = shm_object_pool_next(&context->value_allocator.doing);
    }

    return ENOMEM;
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
            assert(shm_object_pool_push(&context->value_allocator.doing, allocator_offset) == 0);
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
        struct shm_recycle_counter *recycle_counter, const int recycle_keys_once)
{
    int64_t entry_offset;
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
    g_current_time = time(NULL);
    recycled = false;
    while ((entry_offset=shm_list_first(&context->list)) > 0) {
        entry = HT_ENTRY_PTR(context, entry_offset);
        index = entry->value.index.striping;
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
                    "clear valid entries: %d", __LINE__,
                    index, clear_count, valid_count);
            result = 0;
            break;
        }
    }

    context->memory->stats.memory.clear_ht_entry.total += clear_count;
    if (valid_count > 0) {
        context->memory->stats.memory.clear_ht_entry.valid += valid_count;
    }

    recycle_counter->total++;
    if (result == 0) {
        recycle_counter->success++;
        if (valid_count > 0) {
            recycle_counter->force++;
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
                "cleared valid entries: %d",
                __LINE__, clear_count, valid_count);
    }
    return result;
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
        result = shm_value_allocator_do_alloc(context, size, value);
    }
    if (result != 0) {
        logError("file: "__FILE__", line: %d, "
                "malloc %d bytes from shm fail", __LINE__, size);
    }
    return result;
}

int shm_value_allocator_free(struct shmcache_context *context,
        struct shm_value *value, bool *recycled)
{
    struct shm_striping_allocator *allocator;
    int64_t used;

    allocator = context->value_allocator.allocators + value->index.striping;
    used = shm_striping_allocator_free(allocator, value->size);
    context->memory->usage.used -= value->size;
    if (used <= 0) {
        if (used < 0) {
            logError("file: "__FILE__", line: %d, "
                    "striping used memory: %"PRId64" < 0, "
                    "segment: %d, striping: %d, offset: %"PRId64", size: %d",
                    __LINE__, used, value->index.segment,
                    value->index.striping, value->offset, value->size);
        }
        *recycled = true;
        shm_striping_allocator_reset(allocator);
        return shm_value_allocator_do_recycle(context, allocator);
    }

    return 0;
}

