
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
#include "shared_func.h"
#include "shmcache.h"

#define PSF_LOCAL_PORTS_FILENAME "/tmp/shmcache_local_ports.txt"

#define FILED_INDEX_DEAL          3
#define FILED_INDEX_TIMEUSED      4
#define FILED_INDEX_SERVER_ERROR  7
#define FILED_INDEX_DISCONNECT    8
#define FILED_INDEX_DISCARD       9

static const char *shmcache_usage = "    --shmcache               shmcache information";

/* Structure for tsar */
static struct mod_info shmcache_info[] = {
    {"  qps ", SUMMARY_BIT,    MERGE_NULL,  STATS_NULL},
    {"  rt  ", SUMMARY_BIT,    MERGE_NULL,  STATS_NULL},
    {" total", HIDE_BIT,       MERGE_NULL,  STATS_NULL},
    {"  deal", HIDE_BIT,       MERGE_NULL,  STATS_NULL},
    {"  time", HIDE_BIT,       MERGE_NULL,  STATS_NULL},
    {"  wait", DETAIL_BIT,     MERGE_NULL,  STATS_NULL},
    {"  busy", DETAIL_BIT,     MERGE_NULL,  STATS_NULL},
    {" error", DETAIL_BIT,     MERGE_NULL,  STATS_NULL},
    {"disconnect", DETAIL_BIT, MERGE_NULL,  STATS_NULL},
    {"discard", DETAIL_BIT,    MERGE_NULL,  STATS_NULL},
    {"calloc", DETAIL_BIT,     MERGE_NULL,  STATS_NULL},
    {"current", DETAIL_BIT,    MERGE_NULL,  STATS_NULL},
    {"  cmax", DETAIL_BIT,     MERGE_NULL,  STATS_NULL}
};

static void read_shmcache_stats(struct module *mod, const char *parameter)
{
    /* parameter actually equals to mod->parameter */
    char buf[512];

    log_init();
    /*
    if (get_shmcache_stats(buf, sizeof(buf)) != 0) {
        //do nothing when error
    }
    */
    log_destroy();

   /* send data to tsar you can get it by pre_array&cur_array at set_shmcache_record */
    set_mod_record(mod, buf);
    return;
}

#define SET_INCR_FIELD_VAL(i) \
   do { \
       st_array[i] = cur_array[i] - pre_array[i]; \
       if (st_array[i] < 0 || st_array[i] > 100000000.00) {  \
           st_array[i] = 0;    \
       } \
   } while (0)

static void set_shmcache_record(struct module *mod, double st_array[],
    U_64 pre_array[], U_64 cur_array[], int inter)
{
    int i;

    st_array[0] = cur_array[FILED_INDEX_DEAL] - pre_array[FILED_INDEX_DEAL];
    if (st_array[0] < 0.00 || st_array[0] / inter > 10000000.00) {
        st_array[0] = 0.00;
        st_array[1] = 0.00;
    } else if (st_array[0] > 0.01) {
        st_array[1] = (double)(cur_array[FILED_INDEX_TIMEUSED] -
                pre_array[FILED_INDEX_TIMEUSED]) / st_array[0];
        if (st_array[1] < 0.001) {
            st_array[1] = 0.00;
        } else if (st_array[1] > 1000000.00) {
            st_array[1] = 0.00;
        }
        st_array[0] /= inter;
    } else {
        st_array[0] = 0.00;
        st_array[1] = 0.00;
    }

    /* set st record */
    for (i = FILED_INDEX_TIMEUSED+1; i < mod->n_col; i++) {
        st_array[i] = cur_array[i];
    }

    SET_INCR_FIELD_VAL(FILED_INDEX_SERVER_ERROR);
    SET_INCR_FIELD_VAL(FILED_INDEX_DISCONNECT);
    SET_INCR_FIELD_VAL(FILED_INDEX_DISCARD);
}

/* register mod to tsar */
void
mod_register(struct module *mod)
{
    register_mod_fields(mod, "--shmcache", shmcache_usage, shmcache_info,
            13, read_shmcache_stats, set_shmcache_record);
}

/*
int main()
{
    char buf[512];
    log_init();

    if (get_shmcache_stats(buf, sizeof(buf)) != 0) {
        return -1;
    }
    log_destroy();

    printf("buf: %s\n", buf);
    return 0;
}
*/
