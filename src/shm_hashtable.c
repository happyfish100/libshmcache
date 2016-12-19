//shm_hashtable.c

#include <errno.h>
#include <pthread.h>
#include "logger.h"
#include "shared_func.h"
#include "sched_thread.h"
#include "shmopt.h"
#include "shm_lock.h"
#include "shm_object_pool.h"
#include "shm_hashtable.h"

int shm_ht_get_capacity(const int max_count)
{
    unsigned int *capacity;
    //capacity = hash_get_prime_capacity(max_count);
    capacity = NULL;  //TODO: remove me!!!
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

#define HT_CALC_EXPIRES(ttl) \
    (ttl == SHMCACHE_NEVER_EXPIRED_TTL ? 0 : get_current_time() + ttl)

static inline char *shm_ht_get_value_ptr(struct shmcache_context *context,
        const struct shm_value *value)
{
    char *base;
    base = shmopt_get_value_segment(context,
            value->index.segment);
    if (base != NULL) {
        return base + value->offset;
    } else {
        return NULL;
    }
}

int shm_ht_set(struct shmcache_context *context,
        const struct shmcache_buffer *key,
        const struct shmcache_buffer *value, const int ttl)
{
    int result;
    unsigned int index;
    int64_t entry_offset;
    struct shm_hash_entry *entry;
    struct shm_hash_entry *previous;
    char *hvalue;
    struct shm_value new_value;
    bool found;

    if (key->length > SHMCACHE_MAX_KEY_SIZE) {
		logError("file: "__FILE__", line: %d, "
                "invalid key length: %d exceeds %d", __LINE__,
                key->length, SHMCACHE_MAX_KEY_SIZE);
        return ENAMETOOLONG;
    }

    previous = NULL;
    entry = NULL;
    found = false;
    index = HT_GET_BUCKET_INDEX(context, key);
    entry_offset = context->memory->hashtable.buckets[index];
    while (entry_offset > 0) {
        entry = HT_ENTRY_PTR(context, entry_offset);
        if (HT_KEY_EQUALS(entry, key)) {
            hvalue = shm_ht_get_value_ptr(context, &entry->value);
            if (hvalue != NULL) {
                if (HT_VALUE_EQUALS(hvalue, entry->value.length, value)) {
                    entry->expires = HT_CALC_EXPIRES(ttl);
                    shm_list_remove(&context->list, entry_offset);
                    shm_list_add_tail(&context->list, entry_offset);
                    return 0;
                }
            }
            found = true;
            break;
        }

        entry_offset = entry->ht_next;
        previous = entry;
    }

    if (found) {
        shm_value_allocator_free(context, &entry->value);
        shm_list_remove(&context->list, entry_offset);
    } else {
       entry_offset = shm_object_pool_alloc(&context->hentry_allocator);
       if (entry_offset < 0) {
           return ENOMEM;
       }
       entry = HT_ENTRY_PTR(context, entry_offset);
    }

    if ((result=shm_value_allocator_alloc(context, value->length,
                    &new_value)) != 0)
    {
        if (!found) {  //rollback
            shm_object_pool_free(&context->hentry_allocator, entry_offset);
        }
        return result;
    }

    hvalue = shm_ht_get_value_ptr(context, &new_value);
    memcpy(hvalue, value->data, value->length);
    new_value.length = value->length;

    entry->value = new_value;
    entry->expires = HT_CALC_EXPIRES(ttl);
    shm_list_add_tail(&context->list, entry_offset);

    if (found) {
        return 0;
    }

    memcpy(entry->key, key->data, key->length);
    entry->key_len = key->length;
    entry->ht_next = 0;
    if (previous != NULL) {  //add to tail
        previous->ht_next = entry_offset;
    } else {
        context->memory->hashtable.buckets[index] = entry_offset;
    }

    return 0;
}

int shm_ht_get(struct shmcache_context *context,
        const struct shmcache_buffer *key,
        struct shmcache_buffer *value)
{
    unsigned int index;
    int64_t entry_offset;
    struct shm_hash_entry *entry;

    index = HT_GET_BUCKET_INDEX(context, key);
    entry_offset = context->memory->hashtable.buckets[index];
    while (entry_offset > 0) {
        entry = HT_ENTRY_PTR(context, entry_offset);
        if (HT_KEY_EQUALS(entry, key)) {
            if (entry->expires >= get_current_time()) {
                value->data = shm_ht_get_value_ptr(context, &entry->value);
                value->length = entry->value.length;
                return 0;
            } else {
                return ETIMEDOUT;
            }
        }

        entry_offset = entry->ht_next;
    }

    return ENOENT;
}

int shm_ht_delete(struct shmcache_context *context,
        const struct shmcache_buffer *key)
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
        entry = HT_ENTRY_PTR(context, entry_offset);
        if (HT_KEY_EQUALS(entry, key)) {
            if (previous != NULL) {
                previous->ht_next = entry->ht_next;
            } else {
                context->memory->hashtable.buckets[index] = entry->ht_next;
            }

            shm_value_allocator_free(context, &entry->value);
            shm_list_remove(&context->list, entry_offset);
            shm_object_pool_free(&context->hentry_allocator, entry_offset);
            result = 0;
            break;
        }

        entry_offset = entry->ht_next;
        previous = entry;
    }

    return result;
}
