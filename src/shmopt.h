/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//shmopt.h

#ifndef _SHMOPT_H
#define _SHMOPT_H

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
segment context init
parameters:
	segment_context: the context pointer
	context: the context pointer
return error no, 0 for success, != 0 fail
*/
int shmopt_init(struct shmcache_context *context);

/**
segment context destroy
parameters:
	context: the context pointer
*/
void shmopt_destroy(struct shmcache_context *context);

/**
reinit
parameters:
	context: the context pointer
*/
void shmopt_reinit(struct shmcache_context *context);

/**
alloc a node from the context
parameters:
	context: the context pointer
    index: value segment index
return the value segment ptr, return NULL if fail
*/
void *shmopt_get_value_segment(struct shmcache_context *context,
        const int index);

#ifdef __cplusplus
}
#endif

#endif

