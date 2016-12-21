#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logger.h"
#include "shmcache.h"
#include "shmopt.h"

int main(int argc, char *argv[])
{
	int result;
    struct shmcache_context context;
	
	log_init();
	g_log_context.log_level = LOG_DEBUG;

    if ((result=shmcache_init_from_file(&context,
                    "../conf/libshmcache.conf")) != 0)
    {
        return result;
    }

    return shmopt_remove_all(&context);
}

