//shmopt.c

#include <errno.h>
#include <pthread.h>
#include "shmopt.h"
#include "logger.h"

int shmopt_init(struct shmcache_segment_context *context,
		struct shmcache_config *config)
{
	return 0;
}

void shmopt_destroy(struct shmcache_segment_context *context)
{
}

void *shmopt_get_value_segment(struct shmcache_segment_context *context,
        const int index)
{
    return NULL;
}

void shmopt_reinit(struct shmcache_segment_context *context)
{
}

