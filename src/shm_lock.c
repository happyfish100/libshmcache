//shm_lock.c

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "logger.h"
#include "shared_func.h"
#include "sched_thread.h"
#include "shm_hashtable.h"
#include "shm_lock.h"

int shm_lock_init(struct shmcache_context *context)
{
	pthread_mutexattr_t mat;
	int result;

	if ((result=pthread_mutexattr_init(&mat)) != 0) {
		logError("file: "__FILE__", line: %d, "
			"call pthread_mutexattr_init fail, "
			"errno: %d, error info: %s",
			__LINE__, result, strerror(result));
		return result;
	}
	if ((result=pthread_mutexattr_setpshared(&mat,
			PTHREAD_PROCESS_SHARED)) != 0)
	{
		logError("file: "__FILE__", line: %d, "
			"call pthread_mutexattr_setpshared fail, "
			"errno: %d, error info: %s",
			__LINE__, result, strerror(result));
		return result;
	}
	if ((result=pthread_mutexattr_settype(&mat,
			PTHREAD_MUTEX_NORMAL)) != 0)
	{
		logError("file: "__FILE__", line: %d, "
			"call pthread_mutexattr_settype fail, "
			"errno: %d, error info: %s",
			__LINE__, result, strerror(result));
		return result;
	}
	if ((result=pthread_mutex_init(&context->memory->lock.mutex,
                    &mat)) != 0)
    {
		logError("file: "__FILE__", line: %d, "
			"call pthread_mutex_init fail, "
			"errno: %d, error info: %s",
			__LINE__, result, strerror(result));
		return result;
	}
	pthread_mutexattr_destroy(&mat);

    context->memory->lock.pid = 0;
	return 0;
}

int shm_lock_file(struct shmcache_context *context)
{
    int result;
    mode_t old_mast;

    if (context->lock_fd > 0) {
        close(context->lock_fd);
    }

    old_mast = umask(0);
    context->lock_fd = open(context->config.filename, O_WRONLY | O_CREAT, 0666);
    umask(old_mast);
    if (context->lock_fd < 0) {
        result = errno != 0 ? errno : EPERM;
        logError("file: "__FILE__", line: %d, "
                "open filename: %s fail, "
                "errno: %d, error info: %s", __LINE__,
                context->config.filename, result, strerror(result));
        return result;
    }

    if ((result=file_write_lock(context->lock_fd)) != 0) {
        close(context->lock_fd);
        context->lock_fd = -1;
        logError("file: "__FILE__", line: %d, "
                "lock filename: %s fail, "
                "errno: %d, error info: %s", __LINE__,
                context->config.filename, result, strerror(result));
        return result;
    }

    return result;
}

void shm_unlock_file(struct shmcache_context *context)
{
    close(context->lock_fd);
    context->lock_fd = -1;
}

static int shm_detect_deadlock(struct shmcache_context *context,
        const pid_t last_pid)
{
    int result;
    if ((result=shm_lock_file(context)) != 0) {
        return result;
    }

    if (last_pid == context->memory->lock.pid) {
        context->memory->lock.pid = 0;
        if (shm_ht_clear(context) > 0) {
            if (context->config.va_policy.sleep_us_when_recycle_valid_entries > 0) {
                usleep(context->config.va_policy.sleep_us_when_recycle_valid_entries);
            }
        }
        if ((result=pthread_mutex_unlock(&context->memory->lock.mutex)) != 0) {
            logError("file: "__FILE__", line: %d, "
                    "call pthread_mutex_unlock fail, "
                    "errno: %d, error info: %s",
                    __LINE__, result, strerror(result));
        } else {
            __sync_add_and_fetch(&context->memory->stats.
                            lock.unlock_deadlock, 1);
            context->memory->stats.lock.
                last_unlock_deadlock_time = get_current_time();
            logInfo("file: "__FILE__", line: %d, "
                    "my pid: %d, unlock deadlock by process: %d",
                    __LINE__, context->pid, last_pid);
        }
    }

    shm_unlock_file(context);
    return 0;
}

int shm_lock(struct shmcache_context *context)
{
    int result;
    pid_t pid;
    int clocks;

    __sync_add_and_fetch(&context->memory->stats.lock.total, 1);
    clocks = 0;
    while ((result=pthread_mutex_trylock(&context->memory->lock.mutex)) == EBUSY) {
        __sync_add_and_fetch(&context->memory->stats.lock.retry, 1);
        usleep(context->config.lock_policy.trylock_interval_us);
        ++clocks;
        if (clocks > context->detect_deadlock_clocks &&
                (pid=context->memory->lock.pid) > 0)
        {
            clocks =  0;
            if (kill(pid, 0) != 0) {
                if (errno == ESRCH || errno == ENOENT) {
                    __sync_add_and_fetch(&context->memory->stats.
                            lock.detect_deadlock, 1);
                    context->memory->stats.lock.
                        last_detect_deadlock_time = get_current_time();
                    shm_detect_deadlock(context, pid);
                }
            }
        }
    }
    if (result == 0) {
        context->memory->lock.pid = context->pid;
    } else {
        logError("file: "__FILE__", line: %d, "
                "call pthread_mutex_trylock fail, "
                "errno: %d, error info: %s",
                __LINE__, result, strerror(result));
    }
    return result;
}

int shm_unlock(struct shmcache_context *context)
{
    int result;

    context->memory->lock.pid = 0;
    if ((result=pthread_mutex_unlock(&context->memory->lock.mutex)) != 0) {
        logError("file: "__FILE__", line: %d, "
                "call pthread_mutex_unlock fail, "
                "errno: %d, error info: %s",
                __LINE__, result, strerror(result));
    }
    return result;
}

