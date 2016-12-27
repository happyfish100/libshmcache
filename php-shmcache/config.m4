dnl config.m4 for extension shmcache

PHP_ARG_WITH(shmcache, wapper for libshmcache
[  --with-shmcache             Include shmcache wapper for libshmcache])

if test "$PHP_SHMCACHE" != "no"; then
  PHP_SUBST(SHMCACHE_SHARED_LIBADD)

  if test -z "$ROOT"; then
	ROOT=/usr
  fi

  PHP_ADD_INCLUDE($ROOT/include/fastcommon)
  PHP_ADD_INCLUDE($ROOT/include/shmcache)

  PHP_ADD_LIBRARY_WITH_PATH(fastcommon, $ROOT/lib, SHMCACHE_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(shmcache, $ROOT/lib, SHMCACHE_SHARED_LIBADD)

  PHP_NEW_EXTENSION(shmcache, serializer/igbinary.c serializer/msgpack.c serializer/php.c serializer/shmcache_serializer.c php_shmcache.c, $ext_shared)

  CFLAGS="$CFLAGS -Wall"
fi
