#include "shmcache_types.h"
#include "logger.h"
#include "shmcache_serializer.h"

php_msgpack_serialize_func msgpack_serialize_func = NULL;
php_msgpack_unserialize_func msgpack_unserialize_func = NULL;
igbinary_serialize_func igbinary_pack_func = NULL;
igbinary_unserialize_func igbinary_unpack_func = NULL;

int shmcache_serializers = SHMCACHE_SERIALIZER_STRING |
        SHMCACHE_SERIALIZER_INTEGER |
        SHMCACHE_SERIALIZER_NONE |
        SHMCACHE_SERIALIZER_PHP;

extern int shmcache_php_pack(zval *pzval, smart_str *buf);
extern int shmcache_php_unpack(char *content, size_t len, zval *rv);

extern int shmcache_msgpack_pack(zval *pzval, smart_str *buf);
extern int shmcache_msgpack_unpack(char *content, size_t len, zval *rv);

extern int shmcache_igbinary_pack(zval *pzval, char **buf, int *len);
extern int shmcache_igbinary_unpack(char *content, size_t len, zval *rv);

int shmcache_load_functions()
{
    void *handle;

    handle = dlopen(NULL, RTLD_LAZY);
    if (handle == NULL) {
        logError("file: "__FILE__", line: %d, "
                "call dlopen fail, error: %s",
                __LINE__, dlerror());
        return errno != 0 ? errno : EFAULT;
    }
    msgpack_serialize_func = (php_msgpack_serialize_func)dlsym(handle,
            "php_msgpack_serialize");
    msgpack_unserialize_func = (php_msgpack_unserialize_func)dlsym(handle,
            "php_msgpack_unserialize");
    igbinary_pack_func = (igbinary_serialize_func)dlsym(handle,
            "igbinary_serialize");
    igbinary_unpack_func = (igbinary_unserialize_func)dlsym(handle,
            "igbinary_unserialize");
    if (msgpack_serialize_func != NULL && msgpack_unserialize_func != NULL) {
        shmcache_serializers |= SHMCACHE_SERIALIZER_MSGPACK;
    }
    if (igbinary_pack_func != NULL && igbinary_unpack_func != NULL) {
        shmcache_serializers |= SHMCACHE_SERIALIZER_IGBINARY;
    }

    dlclose(handle);
    return 0;
}

int shmcache_serialize(zval *pzval,
        struct shmcache_serialize_output *output)
{
    int result;
    int serializer;

    switch (ZEND_TYPE_OF(pzval)) {
        case IS_STRING:
            output->value->options = SHMCACHE_SERIALIZER_STRING;
            output->value->data = Z_STRVAL_P(pzval);
            output->value->length = Z_STRLEN_P(pzval);
            return 0;
        case IS_LONG:
            output->value->options = SHMCACHE_SERIALIZER_INTEGER;
            output->value->data = output->tmp;
            output->value->length = sprintf(output->tmp, "%"PRId64,
                    (int64_t)Z_LVAL_P(pzval));
            return 0;
        default:
            break;
    }

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
    char buff[20];
    int len;

    switch (value->options) {
        case SHMCACHE_SERIALIZER_NONE:
        case SHMCACHE_SERIALIZER_STRING:
            ZEND_ZVAL_STRINGL(rv, value->data, value->length, 1);
            return 0;
        case SHMCACHE_SERIALIZER_INTEGER:
            if (value->length < (int)sizeof(buff)) {
                len = value->length;
            } else {
                len = sizeof(buff) - 1;
            }
            memcpy(buff, value->data, len);
            buff[len] = '\0';
            ZVAL_LONG(rv, strtoll(buff, NULL, 10));
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
