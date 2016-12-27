#include "php.h"
#include "zend_smart_str.h" /* for smart_str */
#include "shmcache_serializer.h"

extern void php_msgpack_serialize(smart_str *buf, zval *val);
extern void php_msgpack_unserialize(zval *return_value, char *str, size_t str_len);

int shmcache_msgpack_pack(zval *pzval, smart_str *buf)
{
	php_msgpack_serialize(buf, pzval);
	return 0;
}

int shmcache_msgpack_unpack(char *content, size_t len, zval *rv)
{
	ZVAL_NULL(rv);
	php_msgpack_unserialize(rv, content, len);
	return 0;
}
