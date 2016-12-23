#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logger.h"
#include "shmcache.h"
#include "shmopt.h"

static void usage(const char *prog)
{
    fprintf(stderr, "Remove all shmcache including share memories. \n"
            "Usage: %s [config_filename] [-y or --yes]\n", prog);

}

#define IS_YES(str) (strcmp(str, "--yes") == 0 || strcmp(str, "-y") == 0)

int main(int argc, char *argv[])
{
	int result;
    bool ask;
    char confirm;
    char *config_filename;
    struct shmcache_context context;

    if (argc >= 2 && (strcmp(argv[1], "-h") == 0 ||
                strcmp(argv[1], "help") == 0 ||
                strcmp(argv[1], "--help") == 0))
    {
        usage(argv[0]);
        return 0;
    }
    if (argc > 3) {
        usage(argv[0]);
        return EINVAL;
    }

    ask = true;
    config_filename = "/etc/libshmcache.conf";
    if (argc == 2) {
        if (IS_YES(argv[1])) {
            ask = false;
        } else {
            config_filename = argv[1];
        }
    } else if (argc == 3) {
        if (IS_YES(argv[1])) {
            ask = false;
            config_filename = argv[2];
        } else if (IS_YES(argv[2])) {
            config_filename = argv[1];
            ask = false;
        } else {
            usage(argv[0]);
            return EINVAL;
        }
    }

    if (ask) {
        printf("Clear all share memory with config "
                "file: %s? <y|n>: ", config_filename);
        if (!(scanf("%c", &confirm) == 1 && confirm == 'y')) {
            printf("share memory NOT removed.\n");
            return ECANCELED;
        }
    }
	
	log_init();
    if ((result=shmcache_init_from_file_ex(&context,
                    config_filename, false, false)) != 0)
    {
        return result;
    }

    if ((result=shmcache_remove_all(&context)) == 0) {
        printf("ALL share memory segments are removed!\n");
    }
    return  result;
}
