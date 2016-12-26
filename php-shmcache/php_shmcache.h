#ifndef SHMCACHE_H
#define SHMCACHE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PHP_WIN32
#define PHP_SHMCACHE_API __declspec(dllexport)
#else
#define PHP_SHMCACHE_API
#endif

PHP_MINIT_FUNCTION(shmcache);
PHP_RINIT_FUNCTION(shmcache);
PHP_MSHUTDOWN_FUNCTION(shmcache);
PHP_RSHUTDOWN_FUNCTION(shmcache);
PHP_MINFO_FUNCTION(shmcache);

ZEND_FUNCTION(shmcache_version);

PHP_SHMCACHE_API zend_class_entry *php_shmcache_get_ce(void);
PHP_SHMCACHE_API zend_class_entry *php_shmcache_get_exception(void);
PHP_SHMCACHE_API zend_class_entry *php_shmcache_get_exception_base(int root TSRMLS_DC);

#ifdef __cplusplus
}
#endif

#endif	/* SHMCACHE_H */
