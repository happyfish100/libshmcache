#ifndef SHMCACHE_SERIALIZER_H
#define SHMCACHE_SERIALIZER_H

#include "php7_ext_wrapper.h"

#if PHP_MAJOR_VERSION < 7
#include "ext/standard/php_smart_str.h"
#else
#include "Zend/zend_smart_str.h"
#endif

#include "shmcache_types.h"

#define SHMCACHE_SERIALIZER_NONE      0
#define SHMCACHE_SERIALIZER_IGBINARY  1
#define SHMCACHE_SERIALIZER_MSGPACK   2
#define SHMCACHE_SERIALIZER_PHP       4

struct shmcache_serialize_output {
    smart_str *buf;
    struct shmcache_value_info *value;
};

#ifdef __cplusplus
extern "C" {
#endif

int shmcache_serialize(zval *pzval, struct shmcache_serialize_output *output);

void shmcache_free_serialize_output(struct shmcache_serialize_output *output);

int shmcache_unserialize(struct shmcache_value_info *value, zval *rv);

#ifdef __cplusplus
}
#endif

#endif
