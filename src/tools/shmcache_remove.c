#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logger.h"
#include "shmcache.h"
#include "shmopt.h"

int main(int argc, char *argv[])
{
	int result;
    char confirm;
    struct shmcache_context context;

    if (!(argc >= 2 && strcmp(argv[1], "--force") == 0)) {
        printf("clear all share memory? <y|n>:");
        if (!(scanf("%c", &confirm) == 1 && confirm == 'y')) {
            printf("share memory NOT removed.\n");
            return ECANCELED;
        }
    }
	
	log_init();
	g_log_context.log_level = LOG_DEBUG;

    if ((result=shmcache_init_from_file(&context,
                    "../../conf/libshmcache.conf")) != 0)
    {
        return result;
    }

    if ((result=shmcache_remove_all(&context)) == 0) {
        printf("ALL share memory segments are removed!\n");
    }
    return  result;
}

