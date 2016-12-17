/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//shm_list.h

#ifndef _SHM_LIST_H
#define _SHM_LIST_H

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
init list to empty
parameters:
	list: the list
return error no, 0 for success, != 0 fail
*/
static inline void shm_list_init(struct shmcache_list *list)
{
}

/**
add an element to list tail
parameters:
	list: the list
    obj_offset: the object offset
return none
*/
static inline void shm_list_add_tail(struct shmcache_list *list, int64_t obj_offset)
{
}

/**
remove an element from list
parameters:
	list: the list
    obj_offset: the object offset
return none
*/
static inline void shm_list_remove(struct shmcache_list *list, int64_t obj_offset)
{
}

#ifdef __cplusplus
}
#endif

#endif

