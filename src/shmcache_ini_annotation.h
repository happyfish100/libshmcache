//shmcache_ini_annotation.h

#ifndef _SHMCACHE_INI_ANNOTATION_H
#define _SHMCACHE_INI_ANNOTATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fastcommon/common_define.h"
#include "shmcache_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
parameters:
	annotation: the annotation entry
    config_filename: the libshmcache config filename
return errno, 0 for success, != 0 fail
*/
int CONFIG_GET_init_annotation(AnnotationEntry *annotation,
        const char *config_filename);

#ifdef __cplusplus
}
#endif

#endif

