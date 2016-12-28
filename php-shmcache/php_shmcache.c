#include "php7_ext_wrapper.h"
#include "ext/standard/info.h"
#include <zend_extensions.h>
#include <zend_exceptions.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "common_define.h"
#include "logger.h"
#include "shared_func.h"
#include "shmcache_types.h"
#include "shmcache.h"
#include "serializer/shmcache_serializer.h"
#include "php_shmcache.h"

#define MAJOR_VERSION  1
#define MINOR_VERSION  0
#define PATCH_VERSION  0

typedef struct
{
#if PHP_MAJOR_VERSION < 7
	zend_object zo;
#endif
	struct shmcache_context context;
    int serializer;
#if PHP_MAJOR_VERSION >= 7
	zend_object zo;
#endif
} php_shmcache_t;

#if PHP_MAJOR_VERSION < 7
#define shmcache_get_object(obj) zend_object_store_get_object(obj)
#else
#define shmcache_get_object(obj) (void *)((char *)(Z_OBJ_P(obj)) - XtOffsetOf(php_shmcache_t, zo))
#endif

static int le_shmcache;

static zend_class_entry *shmcache_ce = NULL;
static zend_class_entry *shmcache_exception_ce = NULL;

#if PHP_MAJOR_VERSION >= 7
static zend_object_handlers shmcache_object_handlers;
#endif

#if HAVE_SPL
static zend_class_entry *spl_ce_RuntimeException = NULL;
#endif

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 3)
const zend_fcall_info empty_fcall_info = { 0, NULL, NULL, NULL, NULL, 0, NULL, NULL, 0 };
#undef ZEND_BEGIN_ARG_INFO_EX
#define ZEND_BEGIN_ARG_INFO_EX(name, pass_rest_by_reference, return_reference, required_num_args) \
    static zend_arg_info name[] = {                                                               \
        { NULL, 0, NULL, 0, 0, 0, pass_rest_by_reference, return_reference, required_num_args },
#endif

// Every user visible function must have an entry in shmcache_functions[].
	zend_function_entry shmcache_functions[] = {
		ZEND_FE(shmcache_version, NULL)
		{NULL, NULL, NULL}  /* Must be the last line */
	};

zend_module_entry shmcache_module_entry = {
	STANDARD_MODULE_HEADER,
	"shmcache",
	shmcache_functions,
	PHP_MINIT(shmcache),
	PHP_MSHUTDOWN(shmcache),
	NULL,//PHP_RINIT(shmcache),
	NULL,//PHP_RSHUTDOWN(shmcache),
	PHP_MINFO(shmcache),
	"1.00",
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SHMCACHE
	ZEND_GET_MODULE(shmcache)
#endif

static void php_shmcache_destroy(php_shmcache_t *i_obj)
{
    shmcache_destroy(&i_obj->context);
	zend_object_std_dtor(&i_obj->zo TSRMLS_CC);
	efree(i_obj);
}

ZEND_RSRC_DTOR_FUNC(php_shmcache_dtor)
{
#if PHP_MAJOR_VERSION < 7
	if (rsrc->ptr != NULL)
	{
		php_shmcache_t *i_obj = (php_shmcache_t *)rsrc->ptr;
		php_shmcache_destroy(i_obj TSRMLS_CC);
		rsrc->ptr = NULL;
	}
#else
	if (res->ptr != NULL)
	{
		php_shmcache_t *i_obj = (php_shmcache_t *)res->ptr;
		php_shmcache_destroy(i_obj TSRMLS_CC);
		res->ptr = NULL;
	}
#endif

}

#if PHP_MAJOR_VERSION < 7
zend_object_value php_shmcache_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	php_shmcache_t *i_obj;

	i_obj = (php_shmcache_t *)ecalloc(1, sizeof(php_shmcache_t));

	zend_object_std_init(&i_obj->zo, ce TSRMLS_CC);
	retval.handle = zend_objects_store_put(i_obj,
		(zend_objects_store_dtor_t)zend_objects_destroy_object,
        NULL, NULL TSRMLS_CC);
	retval.handlers = zend_get_std_object_handlers();

	return retval;
}

#else

zend_object* php_shmcache_new(zend_class_entry *ce)
{
	php_shmcache_t *i_obj;

	i_obj = (php_shmcache_t *)ecalloc(1, sizeof(php_shmcache_t) +
            zend_object_properties_size(ce));

	zend_object_std_init(&i_obj->zo, ce TSRMLS_CC);
    object_properties_init(&i_obj->zo, ce);
    i_obj->zo.handlers = &shmcache_object_handlers;
	return &i_obj->zo;
}

#endif

/* ShmCache::__construct(string config_filename[, long serializer =
 * ShmCache::SERIALIZER_IGBINARY])
   Creates a ShmCache object */
static PHP_METHOD(ShmCache, __construct)
{
	char *config_filename;
    zend_size_t filename_len;
    long serializer;
	zval *object;
	php_shmcache_t *i_obj;
    char error_info[128];

    object = getThis();
    serializer = SHMCACHE_SERIALIZER_IGBINARY;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l",
			&config_filename, &filename_len, &serializer) == FAILURE)
	{
		logError("file: "__FILE__", line: %d, "
			"zend_parse_parameters fail!", __LINE__);

        zend_throw_exception(shmcache_exception_ce,
                "zend_parse_parameters fail", 0 TSRMLS_CC);
		return;
	}

    if (!shmcache_serializer_enabled(serializer)) {
        sprintf(error_info, "php extension: %s not enabled",
            shmcache_get_serializer_label(serializer));
		logError("file: "__FILE__", line: %d, %s",
                __LINE__, error_info);
        zend_throw_exception(shmcache_exception_ce, error_info,
                 0 TSRMLS_CC);
		return;
    }

	i_obj = (php_shmcache_t *) shmcache_get_object(object);
    if (shmcache_init_from_file(&i_obj->context, config_filename) != 0) {
        sprintf(error_info, "shmcache_init_from_file: %s fail",
                config_filename);
        zend_throw_exception(shmcache_exception_ce, error_info,
                 0 TSRMLS_CC);
		return;
    }
    i_obj->serializer = serializer;
}

static PHP_METHOD(ShmCache, __destruct)
{
	zval *object;
	php_shmcache_t *i_obj;

    object = getThis();
	i_obj = (php_shmcache_t *) shmcache_get_object(object);
	php_shmcache_destroy(i_obj);
}

/* boolean ShmCache::set(string key, mixed value, long ttl)
 * return true for success, false for fail
 */
static PHP_METHOD(ShmCache, set)
{
	zval *object;
	php_shmcache_t *i_obj;
    struct shmcache_key_info key;
    struct shmcache_value_info value;
    struct shmcache_serialize_output output;
    smart_str buf = {0};
    char *key_str;
    zend_size_t key_len;
    zval *val;
    long ttl;
    int result;

    object = getThis();
	i_obj = (php_shmcache_t *) shmcache_get_object(object);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "szl",
			&key_str, &key_len, &val, &ttl) == FAILURE)
	{
		logError("file: "__FILE__", line: %d, "
			"zend_parse_parameters fail!", __LINE__);
		RETURN_FALSE;
	}

    key.data = key_str;
    key.length = key_len;

    value.options = i_obj->serializer;
    output.buf = &buf;
    output.value = &value;
    if (shmcache_serialize(val, &output) != 0) {
		RETURN_FALSE;
    }

    result = shmcache_set(&i_obj->context, &key, &value, ttl);
    shmcache_free_serialize_output(&output);
    if (result != 0) {
		RETURN_FALSE;
    }

    RETURN_TRUE;
}

/* mixed ShmCache::get(string key)
 * return mixed for success, false for fail
 */
static PHP_METHOD(ShmCache, get)
{
	zval *object;
	php_shmcache_t *i_obj;
    struct shmcache_key_info key;
    struct shmcache_value_info value;
    char *key_str;
    zend_size_t key_len;

    object = getThis();
	i_obj = (php_shmcache_t *) shmcache_get_object(object);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
			&key_str, &key_len) == FAILURE)
	{
		logError("file: "__FILE__", line: %d, "
			"zend_parse_parameters fail!", __LINE__);
		RETURN_FALSE;
	}

    key.data = key_str;
    key.length = key_len;
    if (shmcache_get(&i_obj->context, &key, &value) != 0) {
		RETURN_FALSE;
    }

    if (!shmcache_serializer_enabled(value.options)) {
        char error_info[128];
        sprintf(error_info, "php extension: %s not enabled",
            shmcache_get_serializer_label(value.options));
		logError("file: "__FILE__", line: %d, %s",
                __LINE__, error_info);
        zend_throw_exception(shmcache_exception_ce, error_info,
                 0 TSRMLS_CC);
		RETURN_FALSE;
    }

    if (shmcache_unserialize(&value, return_value) != 0) {
		RETURN_FALSE;
    }
    if (return_value == NULL) {
		RETURN_FALSE;
    }
}

/* boolean ShmCache::delete(string key)
 * return true for success, false for fail
 */
static PHP_METHOD(ShmCache, delete)
{
	zval *object;
	php_shmcache_t *i_obj;
    struct shmcache_key_info key;
    char *key_str;
    zend_size_t key_len;

    object = getThis();
	i_obj = (php_shmcache_t *) shmcache_get_object(object);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
			&key_str, &key_len) == FAILURE)
	{
		logError("file: "__FILE__", line: %d, "
			"zend_parse_parameters fail!", __LINE__);
		RETURN_FALSE;
	}

    key.data = key_str;
    key.length = key_len;
    if (shmcache_delete(&i_obj->context, &key) != 0) {
		RETURN_FALSE;
    }

    RETURN_TRUE;
}

/* array ShmCache::stats()
 * return stats array
 */
static PHP_METHOD(ShmCache, stats)
{
	zval *object;
	php_shmcache_t *i_obj;
    struct shmcache_stats stats;
    zval *hashtable;
    zval *memory;
    zval *recycle;
    zval *lock;

    object = getThis();
	i_obj = (php_shmcache_t *) shmcache_get_object(object);

    shmcache_stats(&i_obj->context, &stats);
    array_init(return_value);

    ALLOC_INIT_ZVAL(hashtable);
    array_init(hashtable);
    zend_add_assoc_zval_ex(return_value, "hashtable",
            sizeof("hashtable"), hashtable);
    zend_add_assoc_long_ex(hashtable, "max_keys",
            sizeof("max_keys"), stats.max_key_count);
    zend_add_assoc_long_ex(hashtable, "current_keys",
            sizeof("current_keys"), stats.hashtable.count);
    zend_add_assoc_long_ex(hashtable, "segment_size",
            sizeof("segment_size"), stats.hashtable.segment_size);
    zend_add_assoc_long_ex(hashtable, "set.total",
            sizeof("set.total"), stats.shm.hashtable.set.total);
    zend_add_assoc_long_ex(hashtable, "set.success",
            sizeof("set.success"), stats.shm.hashtable.set.success);
    zend_add_assoc_long_ex(hashtable, "get.total",
            sizeof("get.total"), stats.shm.hashtable.get.total);
    zend_add_assoc_long_ex(hashtable, "get.success",
            sizeof("get.success"), stats.shm.hashtable.get.success);
    zend_add_assoc_long_ex(hashtable, "del.total",
            sizeof("del.total"), stats.shm.hashtable.del.total);
    zend_add_assoc_long_ex(hashtable, "del.success",
            sizeof("del.success"), stats.shm.hashtable.del.success);

    ALLOC_INIT_ZVAL(memory);
    array_init(memory);
    zend_add_assoc_zval_ex(return_value, "memory",
            sizeof("memory"), memory);
    zend_add_assoc_long_ex(memory, "total",
            sizeof("total"), stats.memory.max);
    zend_add_assoc_long_ex(memory, "alloced",
            sizeof("alloced"), stats.memory.alloced);
    zend_add_assoc_long_ex(memory, "used",
            sizeof("used"), stats.memory.used);
    zend_add_assoc_long_ex(memory, "free",
            sizeof("free"), stats.memory.max - stats.memory.used);

    ALLOC_INIT_ZVAL(recycle);
    array_init(recycle);
    zend_add_assoc_zval_ex(return_value, "recycle",
            sizeof("recycle"), recycle);
    zend_add_assoc_long_ex(recycle, "ht_entry.total",
            sizeof("ht_entry.total"),
            stats.shm.memory.clear_ht_entry.total);
    zend_add_assoc_long_ex(recycle, "ht_entry.valid",
            sizeof("ht_entry.valid"),
            stats.shm.memory.clear_ht_entry.valid);

    zend_add_assoc_long_ex(recycle, "by_key.total",
            sizeof("by_key.total"),
            stats.shm.memory.recycle.key.total);
    zend_add_assoc_long_ex(recycle, "by_key.success",
            sizeof("by_key.success"),
            stats.shm.memory.recycle.key.success);
    zend_add_assoc_long_ex(recycle, "by_key.force",
            sizeof("by_key.force"),
            stats.shm.memory.recycle.key.force);

    zend_add_assoc_long_ex(recycle, "by_value_striping.total",
            sizeof("by_value_striping.total"),
            stats.shm.memory.recycle.value_striping.total);
    zend_add_assoc_long_ex(recycle, "by_value_striping.success",
            sizeof("by_value_striping.success"),
            stats.shm.memory.recycle.value_striping.success);
    zend_add_assoc_long_ex(recycle, "by_value_striping.force",
            sizeof("by_value_striping.force"),
            stats.shm.memory.recycle.value_striping.force);

    ALLOC_INIT_ZVAL(lock);
    array_init(lock);
    zend_add_assoc_zval_ex(return_value, "lock",
            sizeof("lock"), lock);
    zend_add_assoc_long_ex(lock, "total",
            sizeof("total"), stats.shm.lock.total);
    zend_add_assoc_long_ex(lock, "retry",
            sizeof("retry"), stats.shm.lock.retry);
    zend_add_assoc_long_ex(lock, "detect_deadlock",
            sizeof("detect_deadlock"),
            stats.shm.lock.detect_deadlock);
    zend_add_assoc_long_ex(lock, "unlock_deadlock",
            sizeof("unlock_deadlock"),
            stats.shm.lock.unlock_deadlock);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, 0, 1)
ZEND_ARG_INFO(0, config_filename)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo___destruct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_set, 0, 0, 3)
ZEND_ARG_INFO(0, key)
ZEND_ARG_INFO(0, value)
ZEND_ARG_INFO(0, ttl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_get, 0, 0, 1)
ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_delete, 0, 0, 1)
ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_stats, 0, 0, 0)
ZEND_END_ARG_INFO()

#define SHMC_ME(name, args) PHP_ME(ShmCache, name, args, ZEND_ACC_PUBLIC)
static zend_function_entry shmcache_class_methods[] = {
    SHMC_ME(__construct,  arginfo___construct)
    SHMC_ME(__destruct,   arginfo___destruct)
    SHMC_ME(set,          arginfo_set)
    SHMC_ME(get,          arginfo_get)
    SHMC_ME(delete,       arginfo_delete)
    SHMC_ME(stats,        arginfo_stats)
    { NULL, NULL, NULL }
};

PHP_MINIT_FUNCTION(shmcache)
{
	zend_class_entry ce;

	log_init();

#if PHP_MAJOR_VERSION >= 7
	memcpy(&shmcache_object_handlers, zend_get_std_object_handlers(),
            sizeof(zend_object_handlers));
	shmcache_object_handlers.offset = XtOffsetOf(php_shmcache_t, zo);
	shmcache_object_handlers.free_obj = NULL;
	shmcache_object_handlers.clone_obj = NULL;
#endif

	le_shmcache = zend_register_list_destructors_ex(NULL, php_shmcache_dtor,
			"ShmCache", module_number);

	INIT_CLASS_ENTRY(ce, "ShmCache", shmcache_class_methods);
	shmcache_ce = zend_register_internal_class(&ce TSRMLS_CC);
	shmcache_ce->create_object = php_shmcache_new;

	INIT_CLASS_ENTRY(ce, "ShmCacheException", NULL);
#if PHP_MAJOR_VERSION < 7
	shmcache_exception_ce = zend_register_internal_class_ex(&ce,
		php_shmcache_get_exception_base(0 TSRMLS_CC), NULL TSRMLS_CC);
#else
	shmcache_exception_ce = zend_register_internal_class_ex(&ce,
		php_shmcache_get_exception_base(0 TSRMLS_CC));
#endif

     zend_declare_class_constant_long(shmcache_ce, ZEND_STRL("NEVER_EXPIRED"),
             SHMCACHE_NEVER_EXPIRED TSRMLS_CC);

     /* serializer */
     zend_declare_class_constant_long(shmcache_ce, ZEND_STRL("SERIALIZER_NONE"),
             SHMCACHE_SERIALIZER_NONE TSRMLS_CC);
     zend_declare_class_constant_long(shmcache_ce, ZEND_STRL("SERIALIZER_IGBINARY"),
             SHMCACHE_SERIALIZER_IGBINARY TSRMLS_CC);
     zend_declare_class_constant_long(shmcache_ce, ZEND_STRL("SERIALIZER_MSGPACK"),
             SHMCACHE_SERIALIZER_MSGPACK TSRMLS_CC);
     zend_declare_class_constant_long(shmcache_ce, ZEND_STRL("SERIALIZER_PHP"),
             SHMCACHE_SERIALIZER_PHP TSRMLS_CC);

     shmcache_load_functions();

     return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(shmcache)
{
	log_destroy();
	return SUCCESS;
}

PHP_RINIT_FUNCTION(shmcache)
{
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(shmcache)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(shmcache)
{
	char mc_info[64];
	sprintf(mc_info, "shmcache v%d.%02d support", 
		MAJOR_VERSION, MINOR_VERSION);

	php_info_print_table_start();
	php_info_print_table_header(2, mc_info, "enabled");
	php_info_print_table_end();
}

/*
string shmcache_version()
return client library version
*/
ZEND_FUNCTION(shmcache_version)
{
	char szVersion[16];
	int len;

	len = sprintf(szVersion, "%d.%d.%d",
		MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);

	ZEND_RETURN_STRINGL(szVersion, len, 1);
}

PHP_SHMCACHE_API zend_class_entry *php_shmcache_get_ce(void)
{
	return shmcache_ce;
}

PHP_SHMCACHE_API zend_class_entry *php_shmcache_get_exception(void)
{
	return shmcache_exception_ce;
}

PHP_SHMCACHE_API zend_class_entry *php_shmcache_get_exception_base(int root TSRMLS_DC)
{
#if HAVE_SPL
	if (!root)
	{
		if (!spl_ce_RuntimeException)
		{
			zend_class_entry *pce;
			zval *value;

			if (zend_hash_find_wrapper(CG(class_table), "runtimeexception",
			   sizeof("RuntimeException"), &value) == SUCCESS)
			{
				pce = Z_CE_P(value);
				spl_ce_RuntimeException = pce;
				return pce;
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return spl_ce_RuntimeException;
		}
	}
#endif
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 2)
	return zend_exception_get_default();
#else
	return zend_exception_get_default(TSRMLS_C);
#endif
}
