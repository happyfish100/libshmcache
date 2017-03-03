#include "shmcache_serializer.h"
#include "ext/standard/php_var.h" /* for serialize */
#include "logger.h"

#if PHP_MAJOR_VERSION < 7
#define GET_ZVAL_PTR(v) &v
#else
#define GET_ZVAL_PTR(v) v
#endif

int shmcache_php_pack(zval *pzval, smart_str *buf)
{
	php_serialize_data_t var_hash;

	PHP_VAR_SERIALIZE_INIT(var_hash);
	php_var_serialize(buf, GET_ZVAL_PTR(pzval), &var_hash TSRMLS_CC);
	PHP_VAR_SERIALIZE_DESTROY(var_hash);
    return 0;
}

int shmcache_php_unpack(char *content, size_t len, zval *rv)
{
	const unsigned char *p;
	php_unserialize_data_t var_hash;

	p = (const unsigned char*)content;
	ZVAL_FALSE(rv);
	PHP_VAR_UNSERIALIZE_INIT(var_hash);
	if (!php_var_unserialize(GET_ZVAL_PTR(rv), &p, p + len,  &var_hash TSRMLS_CC)) {
		zval_ptr_dtor(GET_ZVAL_PTR(rv));
		PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
		logError("file: "__FILE__", line: %d, "
				"unpack error at offset %ld of %ld bytes",
				__LINE__, (long)((char*)p - content), len);
		return EINVAL;
	}
	PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
	return 0;
}
