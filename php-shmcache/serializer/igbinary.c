#include "shmcache_serializer.h"

int shmcache_igbinary_pack(zval *pzval, char **buf, int *len)
{
    int ret;
    size_t out_len;

    if ((ret=igbinary_pack_func((uint8_t **)buf, &out_len, pzval TSRMLS_CC)) == 0) {
        *len = out_len;
    } else {
        *len = 0;
        *buf = NULL;
    }
	return ret;
}

int shmcache_igbinary_unpack(char *content, size_t len, zval *rv)
{
#if (PHP_MAJOR_VERSION < 7) 
    INIT_PZVAL(rv);
    return igbinary_unpack_func((const uint8_t *)content, (size_t)len, &rv TSRMLS_CC);
#else
    return igbinary_unpack_func((const uint8_t *)content, (size_t)len, rv TSRMLS_CC);
#endif
}
