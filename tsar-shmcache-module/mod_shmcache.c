
/*
 * (C) 2010-2011 Alibaba Group Holding Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "tsar.h"
#include "logger.h"
#include "shmcache.h"

#define FILED_INDEX_GET_QPS            0
#define FILED_INDEX_HIT_RATIO          1
#define FILED_INDEX_SET_QPS            2
#define FILED_INDEX_INCR_QPS           3
#define FILED_INDEX_DEL_QPS            4
#define FILED_INDEX_RETRY_LOCKS        5
#define FILED_INDEX_RECYCLE_BY_KEY     6
#define FILED_INDEX_BY_KEY_FORCE       7
#define FILED_INDEX_RECYCLE_BY_VAL     8
#define FILED_INDEX_BY_VAL_FORCE       9
#define FILED_INDEX_CLEAR_HT_COUNT     10
#define FILED_INDEX_CLEAR_HT_VALID     11
#define FILED_INDEX_SET_TOTAL_COUNT    12
#define FILED_INDEX_SET_SUCCESS_COUNT  13
#define FILED_INDEX_INCR_TOTAL_COUNT   14
#define FILED_INDEX_INCR_SUCCESS_COUNT 15
#define FILED_INDEX_GET_TOTAL_COUNT    16
#define FILED_INDEX_GET_SUCCESS_COUNT  17
#define FILED_INDEX_DEL_TOTAL_COUNT    18
#define FILED_INDEX_DEL_SUCCESS_COUNT  19
#define FILEDS_TOTAL_COUNT             20

#define FILED_SUB_START_INDEX    FILED_INDEX_RETRY_LOCKS
#define FILED_SUB_END_INDEX      FILED_INDEX_CLEAR_HT_VALID

#define FILED_KEEP_START_INDEX   FILED_INDEX_SET_TOTAL_COUNT
#define FILED_KEEP_END_INDEX     FILED_INDEX_DEL_SUCCESS_COUNT

static const char *shmcache_usage = "    --shmcache          shmcache information";

/* Structure for tsar */
static struct mod_info shmcache_info[] = {
    {"GetQPS" , SUMMARY_BIT,    MERGE_NULL,  STATS_NULL},
    {"HitRat",  SUMMARY_BIT,    MERGE_NULL,  STATS_NULL},
    {"SetQPS",  DETAIL_BIT,    MERGE_NULL,  STATS_NULL},
    {"IncQPS",  DETAIL_BIT,    MERGE_NULL,  STATS_NULL},
    {"DelQPS",  DETAIL_BIT,    MERGE_NULL,  STATS_NULL},
    {"Relock",  DETAIL_BIT,    MERGE_NULL,  STATS_NULL},
    {"CycKey",  DETAIL_BIT,    MERGE_NULL,  STATS_NULL},
    {"CKForce", DETAIL_BIT,    MERGE_NULL,  STATS_NULL},
    {"CycVal",  DETAIL_BIT,    MERGE_NULL,  STATS_NULL},
    {"CVForce", DETAIL_BIT,    MERGE_NULL,  STATS_NULL},
    {"CLREle",  DETAIL_BIT,    MERGE_NULL,  STATS_NULL},
    {"CLRVld",  DETAIL_BIT,    MERGE_NULL,  STATS_NULL},
    {"set.total_count",    HIDE_BIT,       MERGE_NULL,  STATS_NULL},
    {"set.success_count",  HIDE_BIT,       MERGE_NULL,  STATS_NULL},
    {"incr.total_count",   HIDE_BIT,       MERGE_NULL,  STATS_NULL},
    {"incr.success_count", HIDE_BIT,     MERGE_NULL,  STATS_NULL},
    {"get.total_count",    HIDE_BIT,     MERGE_NULL,  STATS_NULL},
    {"get.success_count",  HIDE_BIT,     MERGE_NULL,  STATS_NULL},
    {"del.total_count",    HIDE_BIT,     MERGE_NULL,  STATS_NULL},
    {"del.success_count",  HIDE_BIT,     MERGE_NULL,  STATS_NULL}
};

static void read_shmcache_stats(struct module *mod, const char *parameter)
{
    /* parameter actually equals to mod->parameter */
    struct shmcache_context context;
    struct shmcache_stats stats;
    const char *config_filename = "/etc/libshmcache.conf";
    char buf[512];

    log_init();
    if (shmcache_init_from_file_ex(&context, config_filename, false, true) == 0) {
        shmcache_stats_ex(&context, &stats, false);
        shmcache_destroy(&context);
    } else {
        memset(&stats, 0, sizeof(stats));
    }

    snprintf(buf, sizeof(buf),
            "0,0,0,0,0,"
            "%"PRId64","
            "%"PRId64","
            "%"PRId64","
            "%"PRId64","
            "%"PRId64","
            "%"PRId64","
            "%"PRId64","
            "%"PRId64","
            "%"PRId64","
            "%"PRId64","
            "%"PRId64","
            "%"PRId64","
            "%"PRId64","
            "%"PRId64","
            "%"PRId64,
            stats.shm.lock.retry,
            stats.shm.memory.recycle.key.total,
            stats.shm.memory.recycle.key.force,
            stats.shm.memory.recycle.value_striping.total,
            stats.shm.memory.recycle.value_striping.force,
            stats.shm.memory.clear_ht_entry.total,
            stats.shm.memory.clear_ht_entry.valid,
            stats.shm.hashtable.set.total,
            stats.shm.hashtable.set.success,
            stats.shm.hashtable.incr.total,
            stats.shm.hashtable.incr.success,
            stats.shm.hashtable.get.total,
            stats.shm.hashtable.get.success,
            stats.shm.hashtable.del.total,
            stats.shm.hashtable.del.success);

    log_destroy();

   /* send data to tsar you can get it by pre_array&cur_array at set_shmcache_record */
    set_mod_record(mod, buf);
    return;
}

#define CALC_INCR_FIELD_VAL(i, v) \
   do { \
       v = cur_array[i] - pre_array[i]; \
       if (v < 0 || v > 1000000000L) {  \
           v = 0;    \
       } \
   } while (0)

#define SET_QPS_FIELD_VAL(current, total) \
    do { \
        st_array[current] = cur_array[total] - pre_array[total]; \
        if (st_array[current] < 0.00 || st_array[current] / inter > 100000000.00) { \
            st_array[current] = 0.00; \
        } else if (st_array[current] > 0.01) {  \
            st_array[current] /= inter;  \
        } else {  \
            st_array[current] = 0.00; \
        } \
    } while (0)

static void set_shmcache_record(struct module *mod, double st_array[],
    U_64 pre_array[], U_64 cur_array[], int inter)
{
    int i;
    int64_t sub;
    int64_t total_sub; 
    int64_t success_sub; 

    SET_QPS_FIELD_VAL(FILED_INDEX_GET_QPS, FILED_INDEX_GET_TOTAL_COUNT);
    SET_QPS_FIELD_VAL(FILED_INDEX_SET_QPS, FILED_INDEX_SET_TOTAL_COUNT);
    SET_QPS_FIELD_VAL(FILED_INDEX_INCR_QPS, FILED_INDEX_INCR_TOTAL_COUNT);
    SET_QPS_FIELD_VAL(FILED_INDEX_DEL_QPS, FILED_INDEX_DEL_TOTAL_COUNT);

    CALC_INCR_FIELD_VAL(FILED_INDEX_GET_TOTAL_COUNT, total_sub);
    CALC_INCR_FIELD_VAL(FILED_INDEX_GET_SUCCESS_COUNT, success_sub);
    if (total_sub > 0) {
        st_array[FILED_INDEX_HIT_RATIO] = 100.00 * (double)success_sub / (double)total_sub; 
    } else {
        st_array[FILED_INDEX_HIT_RATIO] = 0.00;
    }

    for (i = FILED_SUB_START_INDEX; i <= FILED_SUB_END_INDEX; i++) {
        CALC_INCR_FIELD_VAL(i, sub);
        st_array[i] = sub;
    }

    for (i = FILED_KEEP_START_INDEX; i <= FILED_KEEP_END_INDEX; i++) {
        st_array[i] = cur_array[i];
    }
}

/* register mod to tsar */
void
mod_register(struct module *mod)
{
    register_mod_fields(mod, "--shmcache", shmcache_usage, shmcache_info,
            FILEDS_TOTAL_COUNT, read_shmcache_stats, set_shmcache_record);
}
