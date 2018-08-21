#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fastcommon/logger.h"
#include "fastcommon/shared_func.h"
#include "shmcache/shmcache.h"

static void usage(const char *prog)
{
    fprintf(stderr, "shmcache get key value.\n"
         "Usage: %s [config_filename] <key>\n", prog);
}

int main(int argc, char *argv[])
{
	int result;
    int index;
    char *config_filename;
    struct shmcache_context context;
    struct shmcache_key_info key;
    struct shmcache_value_info value;

    if (argc >= 2 && (strcmp(argv[1], "-h") == 0 ||
                strcmp(argv[1], "help") == 0 ||
                strcmp(argv[1], "--help") == 0))
    {
        usage(argv[0]);
        return 0;
    }
    if (argc < 2) {
        usage(argv[0]);
        return EINVAL;
    }

    config_filename = "/etc/libshmcache.conf";
    if (isFile(argv[1])) {
        if (argc < 3) {
            usage(argv[0]);
            return EINVAL;
        }
        config_filename = argv[1];
        index = 2;
    } else {
        index = 1;
    }

	log_init();
    if ((result=shmcache_init_from_file_ex(&context,
                    config_filename, false, true)) != 0)
    {
        return result;
    }

    key.data = argv[index++];
    key.length = strlen(key.data);
    result = shmcache_get(&context, &key, &value);
    if (result == 0) {
        printf("value options: %d (%s serializer), value length: %d",
                value.options, shmcache_get_serializer_label(value.options),
                value.length);
        if ((value.options & SHMCACHE_SERIALIZER_STRING) != 0) {
            printf(", value:\n%.*s", value.length, value.data);
        }
        printf("\n");
    } else {
        fprintf(stderr, "get key: %s fail, errno: %d\n",  key.data, result);
    }

	return result;
}
