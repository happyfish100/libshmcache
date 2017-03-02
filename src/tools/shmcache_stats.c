#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "logger.h"
#include "shared_func.h"
#include "sched_thread.h"
#include "shmcache.h"

static void stats_output(struct shmcache_context *context);

static void usage(const char *prog)
{
    fprintf(stderr, "show shmcache stats.\n"
         "Usage: %s [config_filename] [clean|clear]\n"
         "\tclean or clear to reset the shm stats\n\n", prog);
}

int main(int argc, char *argv[])
{
	int result;
    int last_index;
    bool clear_stats;
    char *config_filename;
    struct shmcache_context context;

    if (argc >= 2 && (strcmp(argv[1], "-h") == 0 ||
                strcmp(argv[1], "help") == 0 ||
                strcmp(argv[1], "--help") == 0))
    {
        usage(argv[0]);
        return 0;
    }

    clear_stats = false;
    if (argc >= 2) {
        last_index = argc - 1;
        if (strcmp(argv[last_index], "clean") == 0 || 
                strcmp(argv[last_index], "clear") == 0)
        {
            clear_stats = true;
        }
    }

    config_filename = "/etc/libshmcache.conf";
    if (argc >= 2) {
        if (argc == 2) {
            if (!clear_stats) {
                config_filename = argv[1];
            }
        } else {
            config_filename = argv[1];
        }
    }

	log_init();
    if ((result=shmcache_init_from_file_ex(&context,
                    config_filename, false, true)) != 0)
    {
        return result;
    }

    if (clear_stats) {
        shmcache_clear_stats(&context);
        printf("shm stats cleared.\n\n");
    } else {
        stats_output(&context);
    }
	return 0;
}

static void stats_output(struct shmcache_context *context)
{
    struct shmcache_stats stats;
    int avg_key_len;
    int avg_value_len;
    char total_ratio[32];
    char ratio[32];
    char rw_ratio[32];
    char time_buff[32];

    g_current_time = time(NULL);
    shmcache_stats(context, &stats);
    if (stats.hashtable.count > 0) {
        avg_key_len = stats.memory.usage.used.key / stats.hashtable.count;
        avg_value_len = stats.memory.usage.used.value / stats.hashtable.count;
    } else {
        avg_key_len = 0;
        avg_value_len = 0;
    }
    if (stats.shm.hashtable.get.total > 0) {
        sprintf(total_ratio, "%.2f%%", 100.00 * stats.shm.hashtable.get.success
                / (double)stats.shm.hashtable.get.total);
    } else {
        total_ratio[0] = '-';
        total_ratio[1] = '\0';
    }
    if (stats.hit.ratio >= 0.00) {
        sprintf(ratio, "%.2f%%", stats.hit.ratio * 100.00);
    } else {
        ratio[0] = '-';
        ratio[1] = '\0';
    }
    if (stats.shm.hashtable.set.total > 0) {
        sprintf(rw_ratio, "%.2f / 1.00", stats.shm.hashtable.get.total
                / (double)stats.shm.hashtable.set.total);
    } else {
        rw_ratio[0] = '-';
        rw_ratio[1] = '\0';
    }

    printf("\ntimestamp info:\n");
    printf("shm init time: %s\n", formatDatetime(context->memory->init_time,
                "%Y-%m-%d %H:%M:%S", time_buff, sizeof(time_buff)));
    if (context->memory->stats.hashtable.last_clear_time > 0) {
        printf("last clear time: %s\n", formatDatetime(context->memory->stats.
                    hashtable.last_clear_time, "%Y-%m-%d %H:%M:%S",
                    time_buff, sizeof(time_buff)));
    }
    printf("stats begin time: %s\n", formatDatetime(context->memory->stats.
                init_time, "%Y-%m-%d %H:%M:%S",
                time_buff, sizeof(time_buff)));

    if (context->memory->stats.memory.recycle.key.last_recycle_time > 0) {
        printf("last recycle by key time: %s\n", formatDatetime(
                    context->memory->stats.memory.recycle.key.
                    last_recycle_time, "%Y-%m-%d %H:%M:%S",
                    time_buff, sizeof(time_buff)));
    }
    if (context->memory->stats.memory.recycle.value_striping.last_recycle_time > 0) {
        printf("last recycle by value time: %s\n", formatDatetime(
                    context->memory->stats.memory.recycle.value_striping.
                    last_recycle_time, "%Y-%m-%d %H:%M:%S",
                    time_buff, sizeof(time_buff)));
    }
    if (context->memory->stats.lock.last_detect_deadlock_time > 0) {
        printf("last detect deadlock time: %s\n", formatDatetime(
                    context->memory->stats.lock.
                    last_detect_deadlock_time, "%Y-%m-%d %H:%M:%S",
                    time_buff, sizeof(time_buff)));
    }
    if (context->memory->stats.lock.last_unlock_deadlock_time > 0) {
        printf("last unlock deadlock time: %s\n", formatDatetime(
                    context->memory->stats.lock.
                    last_unlock_deadlock_time, "%Y-%m-%d %H:%M:%S",
                    time_buff, sizeof(time_buff)));
    }
    printf("\n");

    printf("\nhash table stats:\n");
    printf("max_key_count: %d\n"
            "current_key_count: %d\n"
            "segment_size: %.03f MB\n\n"
            "set.total_count: %"PRId64"\n"
            "set.success_count: %"PRId64"\n"
            "incr.total_count: %"PRId64"\n"
            "incr.success_count: %"PRId64"\n"
            "get.total_count: %"PRId64"\n"
            "get.success_count: %"PRId64"\n"
            "del.total_count: %"PRId64"\n"
            "del.success_count: %"PRId64"\n"
            "get.qps: %.2f\n"
            "hit ratio (last %d seconds): %s\n"
            "total hit ratio (last %d seconds): %s\n"
            "total RW ratio: %s\n\n",
            stats.max_key_count,
            stats.hashtable.count,
            (double)stats.hashtable.segment_size / (1024 * 1024),
            stats.shm.hashtable.set.total,
            stats.shm.hashtable.set.success,
            stats.shm.hashtable.incr.total,
            stats.shm.hashtable.incr.success,
            stats.shm.hashtable.get.total,
            stats.shm.hashtable.get.success,
            stats.shm.hashtable.del.total,
            stats.shm.hashtable.del.success,
            stats.hit.get_qps, stats.hit.seconds, ratio,
            (int)(g_current_time - context->memory->stats.init_time),
            total_ratio, rw_ratio);

    printf("\nmemory stats:\n");
    printf("total: %.03f MB\n"
            "alloced: %.03f MB\n"
            "used: %.03f MB\n"
            "free: %.03f MB\n"
            "avg_key_len: %d\n"
            "avg_value_len: %d\n\n",
            (double)stats.memory.max / (1024 * 1024),
            (double)stats.memory.usage.alloced / (1024 * 1024),
            (double)stats.memory.used / (1024 * 1024),
            (double)(stats.memory.max - stats.memory.used) /
            (1024 * 1024), avg_key_len, avg_value_len);

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
}
