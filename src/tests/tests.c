#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "logger.h"
#include "hash.h"
#include "shm_hashtable.h"
#include "shmcache.h"

int main(int argc, char *argv[])
{
#define MAX_VALUE_SIZE  (4 * 1024)

	int result;
    struct shmcache_context context;
    struct shmcache_key_info key;
    struct shmcache_value_info value;
    char szKey[SHMCACHE_MAX_KEY_SIZE];
    char szValue[MAX_VALUE_SIZE];
    int ttl = 600;
    int i;
	
	log_init();
	g_log_context.log_level = LOG_INFO;
   
    printf("sizeof(struct shm_hash_entry): %d\n",
            (int)sizeof(struct shm_hash_entry));

    if ((result=shmcache_init_from_file(&context,
                    "../../conf/libshmcache.conf")) != 0)
    {
        return result;
    }

    srand(time(NULL));
    memset(szValue, 'A', sizeof(szValue));
    key.data = szKey;
    value.data = szValue;
    value.options = 0;
    for (i=0; i<10000; i++) {
        key.length = sprintf(key.data, "key_%04d", i + 1);
        value.length = (MAX_VALUE_SIZE * (int64_t)rand()) / (int64_t)RAND_MAX;
        result = shmcache_set(&context, &key, &value, ttl);
        if (result != 0) {
            printf("%d. set fail, errno: %d\n", i + 1, result);
            break;
        }
    }
    printf("hash table count: %d\n", shm_ht_count(&context));

	return 0;
}

