/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//shm_op_wrapper.h

#ifndef _SHM_OP_WRAPPER_H
#define _SHM_OP_WRAPPER_H

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
mmap or shmget & shmat
parameters:
	type: mmap or shm
	filename: the filename
	proj_id: the project id to generate key
    size: the share memory size
    key: return the key
return share memory pointer, NULL for fail
*/
void *shm_mmap(const int type, const char *filename,
        const int proj_id, const int64_t size, key_t *key);

/**
munmap or shmdt
parameters:
	type: mmap or shm
    addr: the address to munmap
    size: the share memory size
return: errno, 0 for success, != 0 fail
*/
int shm_munmap(const int type, void *addr, const int64_t size);

/**
remove shm
parameters:
	type: mmap or shm
	proj_id: the project id to generate key
    size: the share memory size
    key:  the key
return: errno, 0 for success, != 0 fail
*/
int shm_remove(const int type, const char *filename,
        const int proj_id, const int64_t size, const key_t key);

#ifdef __cplusplus
}
#endif

#endif

