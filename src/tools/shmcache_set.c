#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "logger.h"
#include "shmcache.h"

int main(int argc, char *argv[])
{
	int result;
    struct shmcache_context context;
    struct shmcache_buffer key;
    struct shmcache_buffer value;
    int ttl;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <key> <value> <ttl>\n", argv[0]);
        return EINVAL;
    }

	log_init();
	g_log_context.log_level = LOG_DEBUG;

    if ((result=shmcache_init_from_file(&context,
                    "../conf/libshmcache.conf")) != 0)
    {
        return result;
    }

    key.data = argv[1];
    key.length = strlen(key.data);
    value.data = argv[2];
    value.length = strlen(value.data);
    ttl = atoi(argv[3]);
    result = shmcache_set(&context, &key, &value, ttl);
    if (result != 0) {
        fprintf(stderr, "set fail, errno: %d\n", result);
    }

	return result;
}

