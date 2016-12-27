#include "shmcache_types.h"
#include "logger.h"
#include "shmcache_serializer.h"

extern int shmcache_php_pack(zval *pzval, smart_str *buf);
extern int shmcache_php_unpack(char *content, size_t len, zval *rv);

extern int shmcache_msgpack_pack(zval *pzval, smart_str *buf);
extern int shmcache_msgpack_unpack(char *content, size_t len, zval *rv);

extern int shmcache_igbinary_pack(zval *pzval, char **buf, int *len);
extern int shmcache_igbinary_unpack(char *content, size_t len, zval *rv);

int shmcache_serialize(zval *pzval,
        struct shmcache_serialize_output *output)
{
    int result;
    int serializer;

    serializer = output->value->options;
    switch (serializer) {
        case SHMCACHE_SERIALIZER_NONE:
            if (Z_TYPE_P(pzval) == IS_STRING) {
                output->value->data = Z_STRVAL_P(pzval);
                output->value->length = Z_STRLEN_P(pzval);
                return 0;
            } else {
                output->value->data = NULL;
                output->value->length = 0;
                logError("file: "__FILE__", line: %d, "
                        "invalid type: %d, must be string",
                        __LINE__, Z_TYPE_P(pzval));
                return EINVAL;
            }
        case SHMCACHE_SERIALIZER_IGBINARY:
            return shmcache_igbinary_pack(pzval, &output->value->data,
                    &output->value->length);
        case SHMCACHE_SERIALIZER_MSGPACK:
        case SHMCACHE_SERIALIZER_PHP:
            if (serializer == SHMCACHE_SERIALIZER_MSGPACK) {
                result = shmcache_msgpack_pack(pzval, output->buf);
            } else {
                result = shmcache_php_pack(pzval, output->buf);
            }

            if (result == 0) {
#if PHP_MAJOR_VERSION < 7
    output->value->data = output->buf->c;
    output->value->length = output->buf->len;
#else
    output->value->data = output->buf->s->val;
    output->value->length = output->buf->s->len;
#endif
        } else {
            output->value->data = NULL;
            output->value->length = 0;
        }
            return result;
        default:
            output->value->data = NULL;
            output->value->length = 0;
            logError("file: "__FILE__", line: %d, "
                    "invalid serializer: %d",
                    __LINE__, serializer);
            return EINVAL;
    }
}

void shmcache_free_serialize_output(struct shmcache_serialize_output *output)
{
    switch (output->value->options) {
        case SHMCACHE_SERIALIZER_IGBINARY:
            efree(output->value->data);
            break;
        case SHMCACHE_SERIALIZER_MSGPACK:
        case SHMCACHE_SERIALIZER_PHP:
            smart_str_free(output->buf);
            break;
        default:
            break;
    }
}

int shmcache_unserialize(struct shmcache_value_info *value, zval *rv)
{
    switch (value->options) {
        case SHMCACHE_SERIALIZER_NONE:
            ZEND_ZVAL_STRINGL(rv, value->data, value->length, 1);
            return 0;
        case SHMCACHE_SERIALIZER_IGBINARY:
            return shmcache_igbinary_unpack(value->data, value->length, rv);
        case SHMCACHE_SERIALIZER_MSGPACK:
            return shmcache_msgpack_unpack(value->data, value->length, rv);
        case SHMCACHE_SERIALIZER_PHP:
            return shmcache_php_unpack(value->data, value->length, rv);
        default:
            logError("file: "__FILE__", line: %d, "
                    "invalid serializer: %d",
                    __LINE__, value->options);
            return EINVAL;
    }
}
