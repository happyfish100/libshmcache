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
#define MAX_VALUE_SIZE  (1 * 1024)

	int result;
    struct shmcache_config config;
    struct shmcache_context context;
    struct shmcache_buffer key;
    struct shmcache_buffer value;
    char szKey[SHMCACHE_MAX_KEY_SIZE];
    char szValue[MAX_VALUE_SIZE];
    int ttl = 600;
    int i;
	
	log_init();
	g_log_context.log_level = LOG_INFO;
   
    printf("sizeof(struct shm_hash_entry): %d\n",
            (int)sizeof(struct shm_hash_entry));

    memset(&config, 0, sizeof(config));

    strcpy(config.filename, "/tmp/shmcache.dat");
    config.max_memory = 4 * 1024 * 1024;
    config.segment_size = 1 * 1024 * 1024;
    config.max_key_count = 10000;
    config.max_value_size = 64 * 1024;
    config.type = SHMCACHE_TYPE_SHM;  //shm or mmap

    config.va_policy.avg_key_ttl = 600;
    config.va_policy.discard_memory_size = 128;
    config.va_policy.max_fail_times = 5;

    config.lock_policy.trylock_interval_us = 1000;
    config.lock_policy.detect_deadlock_interval_ms = 1000;
    config.hash_func = simple_hash;

    if ((result=shmcache_init(&context, &config)) != 0) {
        return result;
    }

    srand(time(NULL));
    memset(szValue, 'A', sizeof(szValue));
    key.data = szKey;
    value.data = szValue;
    for (i=0; i<102400; i++) {
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

