#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "logger.h"
#include "shared_func.h"
#include "shmcache.h"

static void usage(const char *prog)
{
    fprintf(stderr, "shmcache set key value.\n"
         "Usage: %s [config_filename] <key> <value> <ttl>\n", prog);
}

int main(int argc, char *argv[])
{
	int result;
    int index;
    char *config_filename;
    struct shmcache_context context;
    struct shmcache_key_info key;
    char *value;
    int value_len;
    int ttl;

    if (argc >= 2 && (strcmp(argv[1], "-h") == 0 ||
                strcmp(argv[1], "help") == 0 ||
                strcmp(argv[1], "--help") == 0))
    {
        usage(argv[0]);
        return 0;
    }
    if (argc < 4) {
        usage(argv[0]);
        return EINVAL;
    }

    config_filename = "/etc/libshmcache.conf";
    if (isFile(argv[1])) {
        if (argc < 5) {
            usage(argv[0]);
            return EINVAL;
        }
        config_filename = argv[1];
        index = 2;
    } else {
        index = 1;
    }

	log_init();
    if ((result=shmcache_init_from_file(&context, config_filename)) != 0) {
        return result;
    }

    key.data = argv[index++];
    key.length = strlen(key.data);
    value = argv[index++];
    value_len = strlen(value);
    ttl = atoi(argv[index++]);
    result = shmcache_set(&context, &key, value, value_len, ttl);
    if (result == 0) {
        printf("set key: %s success.\n", key.data);
    } else {
        fprintf(stderr, "set key: %s fail, errno: %d\n", key.data, result);
    }

	return result;
}
