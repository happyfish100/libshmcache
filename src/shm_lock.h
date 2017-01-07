//shm_lock.h

#ifndef _SHM_LOCK_H
#define _SHM_LOCK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common_define.h"
#include "shmcache_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
lock init
parameters:
	context: the context pointer
return errno, 0 for success, != 0 fail
*/
int shm_lock_init(struct shmcache_context *context);

/**
lock
parameters:
	context: the context pointer
return errno, 0 for success, != 0 fail
*/
int shm_lock(struct shmcache_context *context);

/**
unlock
parameters:
	context: the context pointer
return errno, 0 for success, != 0 fail
*/
int shm_unlock(struct shmcache_context *context);

/**
lock file
parameters:
	context: the context pointer
return errno, 0 for success, != 0 fail
*/
int shm_lock_file(struct shmcache_context *context);

/**
unlock file
parameters:
	context: the context pointer
return none
*/
void shm_unlock_file(struct shmcache_context *context);

#ifdef __cplusplus
}
#endif

#endif

