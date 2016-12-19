/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

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
	lock: the lock context
return errno, 0 for success, != 0 fail
*/
int shm_lock_init(struct shmcache_context *context);

/**
lock
parameters:
	lock: the lock context
return errno, 0 for success, != 0 fail
*/
int shm_lock(struct shmcache_context *context);

/**
unlock
parameters:
	lock: the lock context
return errno, 0 for success, != 0 fail
*/
int shm_unlock(struct shmcache_context *context);

#ifdef __cplusplus
}
#endif

#endif

