//shm_hashtable.c

#include <errno.h>
#include <pthread.h>
#include "shm_hashtable.h"
#include "logger.h"
#include "shared_func.h"

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

#define HT_ENTRY_PTR(context, entry_offset) ((struct shm_hash_entry *) \
    (context->segments.hashtable.base + entry_offset))

#define HT_KEY_EQUALS(hentry, pkey) (hentry->key_len == pkey->length && \
        memcmp(hentry->key, pkey->data, pkey->length) == 0)

#define HT_VALUE_EQUALS(hvalue, hv_len, pvalue) (hv_len == pvalue->length \
        && memcmp(hvalue, pvalue->data, pvalue->length) == 0)

int shm_ht_set(struct shmcache_context *context,
        const struct shmcache_buffer *key,
        const struct shmcache_buffer *value, const int ttl)
{
    unsigned int index;
    int64_t entry_offset;
    struct shm_hash_entry *entry;
    struct shm_hash_entry *previous;

    previous = NULL;
    index = HT_GET_BUCKET_INDEX(context, key);
    entry_offset = context->memory->hashtable.buckets[index];
    while (entry_offset > 0) {
        entry = HT_ENTRY_PTR(context, entry_offset);

        if (HT_KEY_EQUALS(entry, key)) {

//            if (HT_VALUE_EQUALS()) {
//            }
//hentry->value.length
        }

        entry_offset = entry->ht_next;
        previous = entry;
    }

    return 0;
}

int shm_ht_get(struct shmcache_context *context,
        const struct shmcache_buffer *key,
        struct shmcache_buffer *value)
{
    return 0;
}

int shm_ht_delete(struct shmcache_context *context,
        const struct shmcache_buffer *key)
{
    return 0;
}
