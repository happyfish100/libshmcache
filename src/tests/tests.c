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

static int test_dump(struct shmcache_context *context, const int dump_count)
{
    int result;
    int i;
    struct shmcache_hentry_array array;
    struct shmcache_hash_entry *entry;
    struct shmcache_hash_entry *end;

    if ((result=shm_ht_to_array(context, &array)) != 0) {
        return result;
    }

    printf("entry count: %d\n", array.count);
    end = array.entries + array.count;
    i = 0;
    for (entry=array.entries; entry<end; entry++) {
        printf("%d. key: %.*s, value(%d): %.*s\n",
                ++i, entry->key.length, entry->key.data,
                entry->value.length, entry->value.length,
                entry->value.data);
        if (dump_count > 0 && i == dump_count) {
            break;
        }
    }

    shm_ht_free_array(&array);
    return 0;
}

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
    int ttl = 60;
    int i, k;
    int i1, i2;
	
	log_init();
	g_log_context.log_level = LOG_INFO;
   
    printf("sizeof(struct shm_hash_entry): %d\n",
            (int)sizeof(struct shm_hash_entry));

    if ((result=shmcache_init_from_file(&context,
                    "../../conf/libshmcache.conf")) != 0)
    {
        return result;
    }

    if (argc > 1 &&  strcmp(argv[1], "dump") == 0) {
        int dump_count = 0;
        if (argc > 2) {
            dump_count = atoi(argv[2]);
        }
        return test_dump(&context, dump_count);
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

