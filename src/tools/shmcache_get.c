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

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <key>\n", argv[0]);
        return EINVAL;
    }

	log_init();
	g_log_context.log_level = LOG_DEBUG;

    if ((result=shmcache_init_from_file(&context,
                    "../../conf/libshmcache.conf")) != 0)
    {
        return result;
    }

    key.data = argv[1];
    key.length = strlen(key.data);
    result = shmcache_get(&context, &key, &value);
    if (result == 0) {
        printf("value length: %d, value:\n%.*s\n", value.length, value.length, value.data);
    } else {
        fprintf(stderr, "get fail, errno: %d\n", result);
    }

	return result;
}

