//shmcache.c

#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include "shmcache.h"
#include "logger.h"
#include "shared_func.h"

#define SHMCACE_MEM_ALIGN(x, align)  (((x) + (align - 1)) & (~(align - 1)))

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

    page_size = getpagesize();
    if (segment->size < 1 * 1024 * 1024) {
         striping->size = segment->size; 
    } else {
         striping->size = SHMCACE_MEM_ALIGN(1 * 1024 * 1024, page_size);
         if (striping->size < max_value_size * 2) {
             striping->size = SHMCACE_MEM_ALIGN(max_value_size * 2, page_size);
             if (striping->size > segment->size) {
                 striping->size = segment->size;
             }
         }
    }
    striping->count.max = segment->count.max * (segment->size / striping->size);
    striping->count.current = 0;
}

static int64_t shmcache_get_ht_segment_size(struct shmcache_context *context)
{
    struct shm_value_size_info segment;
    struct shm_value_size_info striping;

    get_value_segment_count_size(&context->config, &segment);
    get_value_striping_count_size(context->config.max_value_size,
            &segment, &striping);

    //sizeof(struct shm_memory_info)
    return 0;
}

int shmcache_init(struct shmcache_context *context,
		struct shmcache_config *config)
{
    memset(context, 0, sizeof(*context));
    context->config = *config;
    
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
