#include "zend_smart_str.h"
#include "logger.h"
#include "shmcache_serializer.h"

/** Serialize zval.
 *  * Return buffer is allocated by this function with emalloc.
 *  @param[out] ret Return buffer
 *  @param[out] ret_len Size of return buffer
 *  @param[in] z Variable to be serialized
 *  @return 0 on success, 1 elsewhere.
*/
extern int igbinary_serialize(uint8_t **ret, size_t *ret_len, zval *z TSRMLS_DC);

/** Unserialize to zval.
 *  @param[in] buf Buffer with serialized data.
 *  @param[in] buf_len Buffer length.
 *  @param[out] z Unserialized zval
 *  @return 0 on success, 1 elsewhere.
 */
#if (PHP_MAJOR_VERSION < 7) 
extern int igbinary_unserialize(const uint8_t *buf, size_t buf_len, zval **z TSRMLS_DC);
#else
extern int igbinary_unserialize(const uint8_t *buf, size_t buf_len, zval *z TSRMLS_DC);
#endif

int shmcache_igbinary_pack(zval *pzval, smart_str *buf)
{
    int ret;
    size_t out_len = 0;
    uint8_t *out = NULL;

    if ((ret=igbinary_serialize(&out, &out_len, pzval TSRMLS_CC)) == 0) {
        smart_string_setl(buf, out, out_len);
    }
	return ret;
}

int shmcache_igbinary_unpack(char *content, size_t len, zval *rv)
{
#if (PHP_MAJOR_VERSION < 7) 
    INIT_PZVAL(z_ret);
    return igbinary_unserialize((const uint8_t *)content, (size_t)len, &rv TSRMLS_CC);
#else
    return igbinary_unserialize((const uint8_t *)content, (size_t)len, rv TSRMLS_CC);
#endif
}
