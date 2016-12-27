#include "shmcache_serializer.h"

int shmcache_msgpack_pack(zval *pzval, smart_str *buf)
{
	msgpack_serialize_func(buf, pzval);
	return 0;
}

int shmcache_msgpack_unpack(char *content, size_t len, zval *rv)
{
	ZVAL_NULL(rv);
	msgpack_unserialize_func(rv, content, len);
	return 0;
}
