#ifndef SHMCACHE_SERIALIZER_H
#define SHMCACHE_SERIALIZER_H

#include "php7_ext_wrapper.h"

#if PHP_MAJOR_VERSION < 7
#include "ext/standard/php_smart_str.h"
#else
#include "Zend/zend_smart_str.h"
#endif

#include "shmcache_types.h"

struct shmcache_serialize_output {
    smart_str *buf;
    struct shmcache_value_info *value;
    char tmp[32];   //temp buffer for number convert
};

typedef void (*php_msgpack_serialize_func)(smart_str *buf, zval *val);
typedef void (*php_msgpack_unserialize_func)(zval *return_value, char *str, size_t str_len);

/** Serialize zval.
 *  * Return buffer is allocated by this function with emalloc.
 *  @param[out] ret Return buffer
 *  @param[out] ret_len Size of return buffer
 *  @param[in] z Variable to be serialized
 *  @return 0 on success, 1 elsewhere.
*/
typedef int (*igbinary_serialize_func)(uint8_t **ret, size_t *ret_len, zval *z TSRMLS_DC);

/** Unserialize to zval.
 *  @param[in] buf Buffer with serialized data.
 *  @param[in] buf_len Buffer length.
 *  @param[out] z Unserialized zval
 *  @return 0 on success, 1 elsewhere.
 */
#if (PHP_MAJOR_VERSION < 7) 
typedef int (*igbinary_unserialize_func)(const uint8_t *buf, size_t buf_len, zval **z TSRMLS_DC);
#else
typedef int (*igbinary_unserialize_func)(const uint8_t *buf, size_t buf_len, zval *z TSRMLS_DC);
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern php_msgpack_serialize_func msgpack_serialize_func;
extern php_msgpack_unserialize_func msgpack_unserialize_func;
extern igbinary_serialize_func igbinary_pack_func;
extern igbinary_unserialize_func igbinary_unpack_func;
extern int shmcache_serializers;

static inline bool shmcache_serializer_enabled(const int serializer)
{
    if (serializer == SHMCACHE_SERIALIZER_STRING) {
        return true;
    }
    return (shmcache_serializers & serializer) != 0;
}

int shmcache_load_functions();

int shmcache_serialize(zval *pzval, struct shmcache_serialize_output *output);

void shmcache_free_serialize_output(struct shmcache_serialize_output *output);

int shmcache_unserialize(struct shmcache_value_info *value, zval *rv);

#ifdef __cplusplus
}
#endif

#endif
