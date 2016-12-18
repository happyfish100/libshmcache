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
#include "shmcache.h"

int main(int argc, char *argv[])
{
	int result;
    struct shmcache_config config;
    struct shmcache_context context;
	
	log_init();
	g_log_context.log_level = LOG_DEBUG;
   
    printf("sizeof(struct shm_hash_entry): %d\n",
            (int)sizeof(struct shm_hash_entry));

    memset(&config, 0, sizeof(config));
    if ((result=shmcache_init(&context, &config)) != 0) {
        return result;
    }

	return 0;
}

