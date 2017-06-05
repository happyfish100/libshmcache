//shm_hashtable.c

#include <errno.h>
#include <pthread.h>
#include "logger.h"
#include "shared_func.h"
#include "sched_thread.h"
#include "shmopt.h"
#include "shm_lock.h"
#include "shm_object_pool.h"
#include "shm_striping_allocator.h"
#include "shm_hashtable.h"

int shm_ht_get_capacity(const int max_count)
{
    unsigned int *capacity;
    capacity = hash_get_prime_capacity(max_count);
    if (capacity == NULL) {
        return max_count;
    }
    return *capacity;
}

void shm_ht_init(struct shmcache_context *context, const int capacity)
{
    context->memory->hashtable.capacity = capacity;
    context->memory->hashtable.count = 0;
}

#define HT_GET_BUCKET_INDEX(context, key) \
    ((unsigned int)context->config.hash_func(key->data, key->length) % \
     context->memory->hashtable.capacity)

#define HT_KEY_EQUALS(hentry, pkey) (hentry->key_len == pkey->length && \
        memcmp(hentry->key, pkey->data, pkey->length) == 0)

#define HT_VALUE_EQUALS(hvalue, hv_len, pvalue) (hv_len == pvalue->length \
        && memcmp(hvalue, pvalue->data, pvalue->length) == 0)

int shm_ht_set(struct shmcache_context *context,
        const struct shmcache_key_info *key,
        const struct shmcache_value_info *value)
{
    int result;
    unsigned int index;
    int64_t old_offset;
    int64_t new_offset;
    struct shm_hash_entry *old_entry;
    struct shm_hash_entry *new_entry;
    struct shm_hash_entry *previous;
    char *hvalue;
    bool found;

    if (key->length > SHMCACHE_MAX_KEY_SIZE) {
		logError("file: "__FILE__", line: %d, "
                "invalid key length: %d exceeds %d", __LINE__,
                key->length, SHMCACHE_MAX_KEY_SIZE);
        return ENAMETOOLONG;
    }

    if (value->length > context->config.max_value_size) {
		logError("file: "__FILE__", line: %d, "
                "invalid value length: %d exceeds %d", __LINE__,
                value->length, context->config.max_value_size);
        return EINVAL;
    }

    if (context->memory->hashtable.count >= context->config.max_key_count) {
        if ((result=shm_value_allocator_recycle(context, &context->memory->
                        stats.memory.recycle.key, context->
                        config.recycle_key_once)) != 0)
        {
            return result;
        }
    }

    if (!g_schedule_flag) {
        g_current_time = time(NULL);
    }

    if ((new_entry=shm_value_allocator_alloc(context,
                    key->length, value->length)) == NULL)
    {
       return result;
    }

    previous = NULL;
    old_entry = NULL;
    found = false;
    index = HT_GET_BUCKET_INDEX(context, key);
    old_offset = context->memory->hashtable.buckets[index];
    while (old_offset > 0) {
        old_entry = shm_get_hentry_ptr(context, old_offset);
        if (HT_KEY_EQUALS(old_entry, key)) {
            found = true;
            break;
        }

        old_offset = old_entry->ht_next;
        previous = old_entry;
    }

    if (found) {
        bool recycled = false;
        new_entry->ht_next = old_entry->ht_next;
        shm_ht_free_entry(context, old_entry, old_offset, &recycled);
    } else {
        new_entry->ht_next = 0;
    }

    new_offset = shm_get_hentry_offset(new_entry);
    memcpy(new_entry->key, key->data, key->length);
    new_entry->key_len = key->length;

    hvalue = shm_get_value_ptr(context, new_entry);
    memcpy(hvalue, value->data, value->length);
    new_entry->value.length = value->length;
    new_entry->value.options = value->options;
    new_entry->expires = value->expires;

    if (previous != NULL) {  //add to tail
        previous->ht_next = new_offset;
    } else {
        context->memory->hashtable.buckets[index] = new_offset;
    }
    context->memory->hashtable.count++;
    context->memory->usage.used.value += value->length;
    context->memory->usage.used.key += new_entry->key_len;
    shm_list_add_tail(context, new_offset);

    return 0;
}

int shm_ht_get(struct shmcache_context *context,
        const struct shmcache_key_info *key,
        struct shmcache_value_info *value)
{
    unsigned int index;
    int64_t entry_offset;
    struct shm_hash_entry *entry;

    index = HT_GET_BUCKET_INDEX(context, key);
    entry_offset = context->memory->hashtable.buckets[index];
    while (entry_offset > 0) {
        entry = shm_get_hentry_ptr(context, entry_offset);
        if (HT_KEY_EQUALS(entry, key)) {
            value->data = shm_get_value_ptr(context, entry);
            value->length = entry->value.length;
            value->options = entry->value.options;
            value->expires = entry->expires;
            if (HT_ENTRY_IS_VALID(entry, get_current_time())) {
                return 0;
            } else {
                return ETIMEDOUT;
            }
        }

        entry_offset = entry->ht_next;
    }

    return ENOENT;
}

void shm_ht_free_entry(struct shmcache_context *context,
        struct shm_hash_entry *entry, const int64_t entry_offset,
        bool *recycled)
{
    context->memory->hashtable.count--;
    context->memory->usage.used.value -= entry->value.length;
    context->memory->usage.used.key -= entry->key_len;
    shm_list_delete(context, entry_offset);
    shm_value_allocator_free(context, entry, recycled);
    entry->ht_next = 0;
}

int shm_ht_delete_ex(struct shmcache_context *context,
        const struct shmcache_key_info *key, bool *recycled)
{
    int result;
    unsigned int index;
    int64_t entry_offset;
    struct shm_hash_entry *entry;
    struct shm_hash_entry *previous;

    previous = NULL;
    result = ENOENT;
    index = HT_GET_BUCKET_INDEX(context, key);
    entry_offset = context->memory->hashtable.buckets[index];
    while (entry_offset > 0) {
        entry = shm_get_hentry_ptr(context, entry_offset);
        if (HT_KEY_EQUALS(entry, key)) {
            if (previous != NULL) {
                previous->ht_next = entry->ht_next;
            } else {
                context->memory->hashtable.buckets[index] = entry->ht_next;
            }

            shm_ht_free_entry(context, entry, entry_offset, recycled);
            result = 0;
            break;
        }

        entry_offset = entry->ht_next;
        previous = entry;
    }

    return result;
}

int shm_ht_set_expires(struct shmcache_context *context,
        const struct shmcache_key_info *key, const time_t expires)
{
    int result;
    unsigned int index;
    int64_t entry_offset;
    struct shm_hash_entry *entry;

    result = ENOENT;
    index = HT_GET_BUCKET_INDEX(context, key);
    entry_offset = context->memory->hashtable.buckets[index];
    while (entry_offset > 0) {
        entry = shm_get_hentry_ptr(context, entry_offset);
        if (HT_KEY_EQUALS(entry, key)) {
            if (HT_ENTRY_IS_VALID(entry, get_current_time())) {
                entry->expires = expires;
                result = 0;
            } else {
                result = ETIMEDOUT;
            }
            break;
        }

        entry_offset = entry->ht_next;
    }

    return result;
}

int shm_ht_clear(struct shmcache_context *context)
{
    struct shm_striping_allocator *allocator;
    struct shm_striping_allocator *end;
    int64_t allocator_offset;
    int ht_count;

    context->memory->stats.hashtable.last_clear_time =
        context->memory->stats.last.calc_time = get_current_time();
    ht_count = context->memory->hashtable.count;
    memset(context->memory->hashtable.buckets, 0, sizeof(int64_t) *
            context->memory->hashtable.capacity);
    context->memory->hashtable.count = 0;
    shm_list_init(context);

    shm_object_pool_init_empty(&context->value_allocator.doing);
    shm_object_pool_init_empty(&context->value_allocator.done);
    end = context->value_allocator.allocators +
        context->memory->vm_info.striping.count.current;
    for (allocator=context->value_allocator.allocators; allocator<end; allocator++) {
        shm_striping_allocator_reset(allocator);
        allocator->in_which_pool = SHMCACHE_STRIPING_ALLOCATOR_POOL_DOING;
        allocator_offset = (char *)allocator - context->segments.hashtable.base;
        shm_object_pool_push(&context->value_allocator.doing, allocator_offset);
    }
    context->memory->usage.used.key = 0;
    context->memory->usage.used.value = 0;
    context->memory->usage.used.entry = 0;

    logInfo("file: "__FILE__", line: %d, pid: %d, "
            "clear hashtable, %d entries be cleared!",
            __LINE__, context->pid, ht_count);
    return ht_count;
}
