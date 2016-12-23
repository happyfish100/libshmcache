#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "logger.h"
#include "shared_func.h"
#include "shmcache.h"

static void usage(const char *prog)
{
    fprintf(stderr, "show shmcache stats.\n"
         "Usage: %s [config_filename]\n", prog);
}

int main(int argc, char *argv[])
{
	int result;
    char *config_filename;
    struct shmcache_context context;
    struct shmcache_stats stats;

    if (argc >= 2 && (strcmp(argv[1], "-h") == 0 ||
                strcmp(argv[1], "help") == 0 ||
                strcmp(argv[1], "--help") == 0))
    {
        usage(argv[0]);
        return 0;
    }

    config_filename = "/etc/libshmcache.conf";
    if (isFile(argv[1])) {
       config_filename = argv[1];
    }

	log_init();
    if ((result=shmcache_init_from_file_ex(&context,
                    config_filename, false)) != 0)
    {
        return result;
    }

    shmcache_stats(&context, &stats);


    printf("\nhash table stats:\n");
    printf("key_count: %d\n"
            "segment_size: %.03f MB\n\n"
            "set_total_count: %"PRId64"\n"
            "set_success_count: %"PRId64"\n"
            "get_total_count: %"PRId64"\n"
            "get_success_count: %"PRId64"\n"
            "del_total_count: %"PRId64"\n"
            "del_success_count: %"PRId64"\n\n",
            stats.hashtable.count,
            (double)stats.hashtable.segment_size / (1024 * 1024),
            stats.shm.hashtable.set.total,
            stats.shm.hashtable.set.success,
            stats.shm.hashtable.get.total,
            stats.shm.hashtable.get.success,
            stats.shm.hashtable.del.total,
            stats.shm.hashtable.del.success);

    printf("\nmemory stats:\n");
    printf("total: %.03f MB\n"
            "alloced: %.03f MB\n"
            "used: %.03f MB\n"
            "free: %.03f MB\n\n",
            (double)stats.max_memory / (1024 * 1024),
            (double)stats.shm.memory.alloced / (1024 * 1024),
            (double)stats.shm.memory.used / (1024 * 1024),
            (double)(stats.max_memory - stats.shm.memory.used) /
            (1024 * 1024));

    printf("\nmemory recycle stats:\n");
    printf("clear_ht_entry.total_count: %"PRId64"\n"
            "clear_ht_entry.valid_count: %"PRId64"\n\n"
            "recycle.key.total_count: %"PRId64"\n"
            "recycle.key.success_count: %"PRId64"\n"
            "recycle.key.force_count: %"PRId64"\n\n"
            "recycle.value_striping.total_count: %"PRId64"\n"
            "recycle.value_striping.success_count: %"PRId64"\n"
            "recycle.value_striping.force_count: %"PRId64"\n\n",
            stats.shm.memory.clear_ht_entry.total,
            stats.shm.memory.clear_ht_entry.valid,
            stats.shm.memory.recycle.key.total,
            stats.shm.memory.recycle.key.success,
            stats.shm.memory.recycle.key.force,
            stats.shm.memory.recycle.value_striping.total,
            stats.shm.memory.recycle.value_striping.success,
            stats.shm.memory.recycle.value_striping.force);

    printf("\nlock stats:\n");
    printf("total_count: %"PRId64"\n"
            "retry_count: %"PRId64"\n"
            "detect_deadlock: %"PRId64"\n"
            "unlock_deadlock: %"PRId64"\n\n",
            stats.shm.lock.total,
            stats.shm.lock.retry,
            stats.shm.lock.detect_deadlock,
            stats.shm.lock.unlock_deadlock);

	return 0;
}
