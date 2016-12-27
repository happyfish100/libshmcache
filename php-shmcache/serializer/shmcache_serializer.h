#ifndef SHMCACHE_SERIALIZER_H
#define SHMCACHE_SERIALIZER_H

#include "zend_smart_str.h"
#include "logger.h"

#define SHMCACHE_SERIALIZER_NONE      0
#define SHMCACHE_SERIALIZER_IGBINARY  1
#define SHMCACHE_SERIALIZER_MSGPACK   2
#define SHMCACHE_SERIALIZER_PHP       3

#ifdef __cplusplus
extern "C" {
#endif

extern int shmcache_php_pack(zval *pzval, smart_str *buf);
extern int shmcache_php_unpack(char *content, size_t len, zval *rv);

extern int shmcache_msgpack_pack(zval *pzval, smart_str *buf);
extern int shmcache_msgpack_unpack(char *content, size_t len, zval *rv);

extern int shmcache_igbinary_pack(zval *pzval, smart_str *buf);
extern int shmcache_igbinary_unpack(char *content, size_t len, zval *rv);

static inline int shmcache_serialize(const int serializer, zval *pzval,
        smart_str *buf)
{
    switch (serializer) {
        case SHMCACHE_SERIALIZER_NONE:
            if (Z_TYPE_P(pzval) == IS_STRING) {
                smart_string_setl(buf, Z_STRVAL_P(pzval), Z_STRLEN_P(pzval));
                return 0;
            } else {
                logError("file: "__FILE__", line: %d, "
                        "invalid type: %d, must be string",
                        __LINE__, Z_TYPE_P(pzval));
                return EINVAL;
            }
        case SHMCACHE_SERIALIZER_IGBINARY:
            return shmcache_igbinary_pack(pzval, buf);
        case SHMCACHE_SERIALIZER_MSGPACK:
            return shmcache_msgpack_pack(pzval, buf);
        case SHMCACHE_SERIALIZER_PHP:
            return shmcache_php_pack(pzval, buf);
        default:
            logError("file: "__FILE__", line: %d, "
                    "invalid serializer: %d",
                    __LINE__, serializer);
            return EINVAL;
    }
}

static inline void shmcache_free_smart_str(const int serializer, smart_str *buf)
{
    switch (serializer) {
        case SHMCACHE_SERIALIZER_IGBINARY:
            efree(buf->s);
            break;
        case SHMCACHE_SERIALIZER_MSGPACK:
        case SHMCACHE_SERIALIZER_PHP:
            smart_str_free(buf);
            break;
}

static inline int shmcache_unserialize(const int serializer,
        char *content, size_t len, zval *rv)
{
    switch (serializer) {
        case SHMCACHE_SERIALIZER_NONE:
#if PHP_MAJOR_VERSION < 7
            INIT_ZVAL_P(rv);
            ZVAL_STRINGL(rv, content, len, 1);
#else
            {
                zend_string *sz_data;
                bool use_heap_data;
                ZSTR_ALLOCA_INIT(sz_data, content, len, use_heap_data);
                ZVAL_NEW_STR(rv, sz_data);
            }
#endif
            return 0;
        case SHMCACHE_SERIALIZER_IGBINARY:
            return shmcache_igbinary_unpack(content, len, rv);
        case SHMCACHE_SERIALIZER_MSGPACK:
            return shmcache_msgpack_unpack(content, len, rv);
        case SHMCACHE_SERIALIZER_PHP:
            return shmcache_php_unpack(content, len, rv);
        default:
            logError("file: "__FILE__", line: %d, "
                    "invalid serializer: %d",
                    __LINE__, serializer);
            return EINVAL;
    }
}

#ifdef __cplusplus
}
#endif

#endif
