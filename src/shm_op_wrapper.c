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

#define SHM_GET_MMAP_FILENAME(true_filename, filename, proj_id) \
    snprintf(true_filename, sizeof(true_filename), "%s.%d", \
            filename, proj_id - 1)

static void *shm_do_mmap(const char *filename, int proj_id,
        const int64_t size, const bool create_segment, int *err_no)
{
    char true_filename[MAX_PATH_SIZE];
    void *addr;
    int fd;
    mode_t old_mast;
    bool need_truncate;

    SHM_GET_MMAP_FILENAME(true_filename, filename, proj_id);
    fd = open(true_filename, O_RDWR);
    if (fd >= 0) {
        struct stat st;
        if (fstat(fd, &st) != 0) {
            *err_no = errno != 0 ? errno : EPERM;
            logError("file: "__FILE__", line: %d, "
                    "stat file: %s fail, "
                    "errno: %d, error info: %s", __LINE__,
                    true_filename, *err_no, strerror(*err_no));
            return NULL;
        }
        if (st.st_size > size) {
            logWarning("file: "__FILE__", line: %d, "
                    "file %s size: %"PRId64" > expect size: %"PRId64,
                    __LINE__, true_filename, st.st_size, size);
            need_truncate = false;
        } else if (st.st_size < size) {
            logWarning("file: "__FILE__", line: %d, "
                    "file %s size: %"PRId64" < expect size: %"PRId64
                    ", auto extend file size", __LINE__,
                    true_filename, st.st_size, size);
            need_truncate = true;
        } else {
            need_truncate = false;
        }
    } else {
        if (!(create_segment && errno == ENOENT)) {
            *err_no = errno != 0 ? errno : EPERM;
            logError("file: "__FILE__", line: %d, "
                    "open file: %s fail, "
                    "errno: %d, error info: %s", __LINE__,
                    true_filename, *err_no, strerror(*err_no));
            return NULL;
        }

        old_mast = umask(0);
        fd = open(true_filename, O_RDWR | O_CREAT, 0666);
        umask(old_mast);

        if (fd < 0) {
            *err_no = errno != 0 ? errno : EPERM;
            logError("file: "__FILE__", line: %d, "
                    "open file: %s fail, "
                    "errno: %d, error info: %s", __LINE__,
                    true_filename, *err_no, strerror(*err_no));
            return NULL;
        }
        need_truncate = true;
    }
    if (need_truncate) {
        if (ftruncate(fd, size) != 0) {
            *err_no = errno != 0 ? errno : EPERM;
            close(fd);
            logError("file: "__FILE__", line: %d, "
                    "truncate file: %s to size %"PRId64" fail, "
                    "errno: %d, error info: %s", __LINE__,
                    true_filename, size, *err_no, strerror(*err_no));
            return NULL;
        }
    }

    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == NULL) {
        *err_no = errno != 0 ? errno : EPERM;
        logError("file: "__FILE__", line: %d, "
                "mmap file: %s with size: %"PRId64" fail, "
                "errno: %d, error info: %s", __LINE__,
                true_filename, size, *err_no, strerror(*err_no));
    }
    close(fd);
    *err_no = 0;
    return addr;
}

void *shm_do_shmmap(const key_t key, const int64_t size,
        const bool create_segment, int *err_no)
{
    int shmid;
    void *addr;

    if (create_segment) {
        shmid = shmget(key, size, IPC_CREAT | 0666);
    } else {
        shmid = shmget(key, 0, 0666);
    }
    if (shmid < 0) {
        *err_no = errno != 0 ? errno : EPERM;
        logError("file: "__FILE__", line: %d, "
                "shmget with key %08x fail, "
                "errno: %d, error info: %s", __LINE__,
                key, *err_no, strerror(*err_no));
        return NULL;
    }

    addr = shmat(shmid, NULL, 0);
    if (addr == NULL || addr == (void *)-1) {
        *err_no = errno != 0 ? errno : EPERM;
        logError("file: "__FILE__", line: %d, "
                "shmat with key %08x fail, "
                "errno: %d, error info: %s", __LINE__,
                key, *err_no, strerror(*err_no));
        return NULL;
    }
    *err_no = 0;
    return addr;
}

static int shm_get_key(const char *filename, const int proj_id,
        key_t *key)
{
    int result;
    if (access(filename, F_OK) != 0) {
        result = errno != 0 ? errno : ENOENT;
        if (result != ENOENT) {
            logError("file: "__FILE__", line: %d, "
                    "access filename: %s fail, "
                    "errno: %d, error info: %s", __LINE__,
                    filename, result, strerror(result));
            return result;
        }

        result = writeToFile(filename, "FOR LOCK", 8);
        if (result != 0) {
            return result;
        }
        if (chmod(filename, 0666) != 0) {
            result = errno != 0 ? errno : EFAULT;
            logError("file: "__FILE__", line: %d, "
                    "chmod filename: %s fail, "
                    "errno: %d, error info: %s", __LINE__,
                    filename, result, strerror(result));
            return result;
        }
    }

    *key = fc_ftok(filename, proj_id);
    if (*key == -1) {
        result = errno != 0 ? errno : EFAULT;
        logError("file: "__FILE__", line: %d, "
                "call fc_ftok fail, filename: %s, proj_id: %d, "
                "errno: %d, error info: %s", __LINE__,
                filename, proj_id, result, strerror(result));
        return result;
    }
    return 0;
}

void *shm_mmap(const int type, const char *filename,
        const int proj_id, const int64_t size, key_t *key,
        const bool create_segment, int *err_no)
{
    if ((*err_no=shm_get_key(filename, proj_id, key)) != 0) {
        return NULL;
    }
    if (type == SHMCACHE_TYPE_MMAP) {
        return shm_do_mmap(filename, proj_id, size, create_segment, err_no);
    } else {
        return shm_do_shmmap(*key, size, create_segment, err_no);
    }
}

bool shm_exists(const int type, const char *filename, const int proj_id)
{
    key_t key;
    char true_filename[MAX_PATH_SIZE];

    if (shm_get_key(filename, proj_id, &key) != 0) {
        return false;
    }
    if (type == SHMCACHE_TYPE_MMAP) {
        SHM_GET_MMAP_FILENAME(true_filename, filename, proj_id);
        return access(true_filename, F_OK) == 0;
    } else {
        return shmget(key, 0, 0666) >= 0;
    }
}

int shm_munmap(const int type, void *addr, const int64_t size)
{
    int result;
    if (type == SHMCACHE_TYPE_MMAP) {
        if (munmap(addr, size) == 0) {
            result = 0;
        } else {
            result = errno != 0 ? errno : EACCES;
            logError("file: "__FILE__", line: %d, "
                    "munmap addr: %p, size: %"PRId64" fail, "
                    "errno: %d, error info: %s", __LINE__,
                    addr, size, errno, strerror(errno));
        }
    } else {
        if (shmdt(addr) == 0) {
            result = 0;
        } else {
            result = errno != 0 ? errno : EACCES;
            logError("file: "__FILE__", line: %d, "
                    "munmap addr: %p, size: %"PRId64" fail, "
                    "errno: %d, error info: %s", __LINE__,
                    addr, size, errno, strerror(errno));
        }
    }
    return result;
}

int shm_remove(const int type, const char *filename,
        const int proj_id, const int64_t size, const key_t key)
{
    int result;
    char true_filename[MAX_PATH_SIZE];

    if (type == SHMCACHE_TYPE_MMAP) {
        snprintf(true_filename, sizeof(true_filename), "%s.%d",
                filename, proj_id - 1);
        if (unlink(true_filename) != 0) {
            result = errno != 0 ? errno : EPERM;
            logError("file: "__FILE__", line: %d, "
                    "unlink file: %s fail, "
                    "errno: %d, error info: %s", __LINE__,
                    true_filename, errno, strerror(errno));
            return result;
        }
    } else {
        int shmid;

        shmid = shmget(key, 0, 0666);
        if (shmid < 0) {
            result = errno != 0 ? errno : EACCES;
            logError("file: "__FILE__", line: %d, "
                    "shmget with key %08x fail, "
                    "errno: %d, error info: %s", __LINE__,
                    key, errno, strerror(errno));
            return result;
        }

         if (shmctl(shmid, IPC_RMID, NULL) != 0) {
            result = errno != 0 ? errno : EACCES;
            logError("file: "__FILE__", line: %d, "
                    "remove shm with key %08x fail, "
                    "errno: %d, error info: %s", __LINE__,
                    key, errno, strerror(errno));
            return result;
         }
    }
    return 0;
}

