//shm_op_wrapper.c

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "logger.h"
#include "shared_func.h"
#include "shm_op_wrapper.h"

void *shm_mmap(const int type, const char *filename, int proj_id,
        const int64_t size, key_t *key)
{
    void *addr;
    int result;

    if (access(filename, F_OK) != 0) {
        if (errno != ENOENT) {
            logError("file: "__FILE__", line: %d, "
                    "access filename: %s fail, "
                    "errno: %d, error info: %s", __LINE__,
                    filename, errno, strerror(errno));
            return NULL;
        }

        result = writeToFile(filename, "OK", 2);
        if (result != 0) {
            return NULL;
        }
    }

    *key = ftok(filename, proj_id);
    if (*key == -1) {
        logError("file: "__FILE__", line: %d, "
                "call ftok fail, filename: %d, proj_id: %d, "
                "errno: %d, error info: %s", __LINE__,
                filename, proj_id, errno, strerror(errno));
        return NULL;
    }

    if (type == SHMCACHE_TYPE_MMAP) {
        int fd;
        fd = open(filename, O_RDWR | O_CREAT);  //TODO: change filename & truncate file
        addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == NULL) {
        }
    } else {
    }
	return NULL;
}

int shm_munmap(const int type, void *addr, const int64_t size)
{
    if (type == SHMCACHE_TYPE_MMAP) {
    } else {
    }
    return 0;
}

