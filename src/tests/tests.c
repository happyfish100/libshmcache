#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "fastcommon/logger.h"
#include "fastcommon/hash.h"
#include "shmcache/shm_hashtable.h"
#include "shmcache/shmcache.h"

int main(int argc, char *argv[])
{
#define MAX_VALUE_SIZE  (8 * 1024)
#define KEY_COUNT 13000

	int result;
    int current_time;
    struct shmcache_context context;
    char szKey[SHMCACHE_MAX_KEY_SIZE];
    char szValue[MAX_VALUE_SIZE];
    struct shmcache_hash_entry *entries;
    struct shmcache_hash_entry tmp;
    int bytes;
    int ttl = 300;
    int i, k;
    int i1, i2;
	
	log_init();
	g_log_context.log_level = LOG_INFO;
   
    printf("sizeof(struct shm_hash_entry): %d\n",
            (int)sizeof(struct shm_hash_entry));

    if ((result=shmcache_init_from_file(&context,
                    "/usr/local/etc/libshmcache.conf")) != 0)
    {
        return result;
    }

    bytes = sizeof(struct shmcache_hash_entry) * KEY_COUNT;
    entries = (struct shmcache_hash_entry *)malloc(bytes);
    if (entries == NULL) {
        logError("file: "__FILE__", line: %d, "
                "malloc %d bytes fail", __LINE__, bytes);
        return ENOMEM;
    }

    current_time = time(NULL);
    srand(current_time);
    rand();
    memset(szValue, 'A', sizeof(szValue));
    for (i=0; i<KEY_COUNT; i++) {
        entries[i].key.length = sprintf(szKey, "key_%04d", i + 1);
        entries[i].key.data = strdup(szKey);

        entries[i].value.data = szValue;
        entries[i].value.length = (MAX_VALUE_SIZE * (int64_t)rand()) / (int64_t)RAND_MAX;
        entries[i].value.options = SHMCACHE_SERIALIZER_STRING;
        entries[i].value.expires = current_time + ttl;
    }

    for (i=0; i<KEY_COUNT; i++) {
        i1 = (KEY_COUNT * (int64_t)rand()) / (int64_t)RAND_MAX;
        i2 = KEY_COUNT - 1 - i1;
        if (i1 != i2) {
            tmp = entries[i1];
            entries[i1] = entries[i2];
            entries[i2] = tmp;
        }
    }

    for (k=0; k<5; k++) {
        for (i=0; i<KEY_COUNT; i++) {
            result = shmcache_set_ex(&context,
                            &entries[i].key,
                            &entries[i].value);
            if (result != 0) {
                printf("%d. set fail, errno: %d\n", i + 1, result);
                break;
            }
        }
    }
    printf("hash table count: %d\n", shm_ht_count(&context));

	return 0;
}

