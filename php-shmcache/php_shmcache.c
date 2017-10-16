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

typedef struct
{
#if PHP_MAJOR_VERSION < 7
	zend_object zo;
#endif
	struct shmcache_context *context;
    int serializer;
#if PHP_MAJOR_VERSION >= 7
	zend_object zo;
#endif
} php_shmcache_t;

struct shmcache_context_by_file {
    char *config_filename;
    struct shmcache_context *context;
};
static struct shmcache_context_array {
    struct shmcache_context_by_file *contexts;
    int count;
    int alloc;
} context_array = {NULL, 0, 0};

static struct shmcache_context *shmcache_get_context(const char *filename,
        char *error_info, const int error_size);

#if PHP_MAJOR_VERSION < 7
#define shmcache_get_object(obj) zend_object_store_get_object(obj TSRMLS_CC)
#else
#define shmcache_get_object(obj) (void *)((char *)(Z_OBJ_P(obj)) - XtOffsetOf(php_shmcache_t, zo))
#endif

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
	"1.0.5",
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SHMCACHE
	ZEND_GET_MODULE(shmcache)
#endif

#if PHP_MAJOR_VERSION < 7
static void php_shmcache_destroy(void *object TSRMLS_DC)
{
    php_shmcache_t *i_obj = (php_shmcache_t *)object;
	zend_object_std_dtor(&i_obj->zo TSRMLS_CC);
	efree(i_obj);
}
#else
static void php_shmcache_destroy(zend_object *object)
{
    php_shmcache_t *i_obj = (php_shmcache_t *)((char*)(object) -
            XtOffsetOf(php_shmcache_t, zo));
	zend_object_std_dtor(&i_obj->zo TSRMLS_CC);
}
#endif

#if PHP_MAJOR_VERSION < 7
zend_object_value php_shmcache_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	php_shmcache_t *i_obj;

	i_obj = (php_shmcache_t *)ecalloc(1, sizeof(php_shmcache_t));

	zend_object_std_init(&i_obj->zo, ce TSRMLS_CC);
	retval.handle = zend_objects_store_put(i_obj,
		(zend_objects_store_dtor_t)zend_objects_destroy_object,
        php_shmcache_destroy, NULL TSRMLS_CC);
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
    i_obj->context = shmcache_get_context(config_filename,
            error_info, sizeof(error_info));
    if (i_obj->context == NULL) {
        zend_throw_exception(shmcache_exception_ce, error_info,
                 0 TSRMLS_CC);
		return;
    }
    i_obj->serializer = serializer;
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

    if (ZEND_IS_BOOL(val) && !ZEND_IS_TRUE(val)) {
		logError("file: "__FILE__", line: %d, "
			"can't set value to false!", __LINE__);
        zend_throw_exception(shmcache_exception_ce,
                "can't set value to false", 0 TSRMLS_CC);
		RETURN_FALSE;
    }

    key.data = key_str;
    key.length = key_len;

    value.options = i_obj->serializer;
    value.expires = HT_CALC_EXPIRES(time(NULL), ttl);
    output.buf = &buf;
    output.value = &value;
    if (shmcache_serialize(val, &output) != 0) {
		RETURN_FALSE;
    }

    result = shmcache_set_ex(i_obj->context, &key, &value);
    shmcache_free_serialize_output(&output);
    if (result != 0) {
		RETURN_FALSE;
    }

    RETURN_TRUE;
}

/* boolean ShmCache::setExpires(string key, long expires)
 * return true for success, false for fail
 */
static PHP_METHOD(ShmCache, setExpires)
{
	zval *object;
	php_shmcache_t *i_obj;
    struct shmcache_key_info key;
    char *key_str;
    zend_size_t key_len;
    long expires;
    int result;

    object = getThis();
	i_obj = (php_shmcache_t *) shmcache_get_object(object);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
			&key_str, &key_len, &expires) == FAILURE)
	{
		logError("file: "__FILE__", line: %d, "
			"zend_parse_parameters fail!", __LINE__);
		RETURN_FALSE;
	}

    key.data = key_str;
    key.length = key_len;
    result = shmcache_set_expires(i_obj->context, &key, expires);
    if (result == 0) {
        RETURN_TRUE;
    } else {
        if (result == EINVAL) {
            logError("file: "__FILE__", line: %d, "
                    "expires: %ld is invalid!", __LINE__, expires);
            zend_throw_exception(shmcache_exception_ce,
                    "invalid expires parameter", 0 TSRMLS_CC);
        }
		RETURN_FALSE;
    }
}

/* boolean ShmCache::setTTL(string key, long ttl)
 * return true for success, false for fail
 */
static PHP_METHOD(ShmCache, setTTL)
{
	zval *object;
	php_shmcache_t *i_obj;
    struct shmcache_key_info key;
    char *key_str;
    zend_size_t key_len;
    long ttl;
    int result;

    object = getThis();
	i_obj = (php_shmcache_t *) shmcache_get_object(object);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
			&key_str, &key_len, &ttl) == FAILURE)
	{
		logError("file: "__FILE__", line: %d, "
			"zend_parse_parameters fail!", __LINE__);
		RETURN_FALSE;
	}

    key.data = key_str;
    key.length = key_len;
    result = shmcache_set_ttl(i_obj->context, &key, ttl);
    if (result == 0) {
        RETURN_TRUE;
    } else {
        if (result == EINVAL) {
            logError("file: "__FILE__", line: %d, "
                    "ttl: %ld is invalid!", __LINE__, ttl);
            zend_throw_exception(shmcache_exception_ce,
                    "invalid ttl parameter", 0 TSRMLS_CC);
        }
		RETURN_FALSE;
    }
}

/* long ShmCache::incr(string key, long increment, long ttl)
 * return the value after increase, false for fail
 */
static PHP_METHOD(ShmCache, incr)
{
	zval *object;
	php_shmcache_t *i_obj;
    struct shmcache_key_info key;
    char *key_str;
    zend_size_t key_len;
    long increment;
    long ttl;
    int64_t new_value;
    int result;

    object = getThis();
	i_obj = (php_shmcache_t *) shmcache_get_object(object);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sll",
			&key_str, &key_len, &increment, &ttl) == FAILURE)
	{
		logError("file: "__FILE__", line: %d, "
			"zend_parse_parameters fail!", __LINE__);
		RETURN_FALSE;
	}

    key.data = key_str;
    key.length = key_len;
    result = shmcache_incr(i_obj->context, &key, increment, ttl, &new_value);
    if (result != 0) {
		RETURN_FALSE;
    }

    RETURN_LONG(new_value);
}

/* mixed ShmCache::get(string key[, boolean returnExpired = false])
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
    zend_bool returnExpired;
    int result;

    object = getThis();
	i_obj = (php_shmcache_t *) shmcache_get_object(object);

    returnExpired = false;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b",
			&key_str, &key_len, &returnExpired) == FAILURE)
	{
		logError("file: "__FILE__", line: %d, "
			"zend_parse_parameters fail!", __LINE__);
		RETURN_FALSE;
	}

    key.data = key_str;
    key.length = key_len;
    if ((result=shmcache_get(i_obj->context, &key, &value)) != 0) {
        if (!(returnExpired && result == ETIMEDOUT)) {
            RETURN_FALSE;
        }
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

/* long ShmCache::getExpires(string key[, boolean returnExpired = false])
 * return expires timestamp, 0 for never expire, false for not exist
 */
static PHP_METHOD(ShmCache, getExpires)
{
	zval *object;
	php_shmcache_t *i_obj;
    struct shmcache_key_info key;
    struct shmcache_value_info value;
    char *key_str;
    zend_size_t key_len;
    zend_bool returnExpired;
    int result;

    object = getThis();
	i_obj = (php_shmcache_t *) shmcache_get_object(object);

    returnExpired = false;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b",
			&key_str, &key_len, &returnExpired) == FAILURE)
	{
		logError("file: "__FILE__", line: %d, "
			"zend_parse_parameters fail!", __LINE__);
		RETURN_FALSE;
	}

    key.data = key_str;
    key.length = key_len;
    if ((result=shmcache_get(i_obj->context, &key, &value)) != 0) {
        if (!(returnExpired && result == ETIMEDOUT)) {
            RETURN_FALSE;
        }
    }
    RETURN_LONG(value.expires);
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
    if (shmcache_delete(i_obj->context, &key) != 0) {
		RETURN_FALSE;
    }

    RETURN_TRUE;
}

/* array ShmCache::stats([bool calc_hit_ratio = true])
 * return stats array
 */
static PHP_METHOD(ShmCache, stats)
{
	zval *object;
	php_shmcache_t *i_obj;
    struct shmcache_stats stats;
    zval *hashtable;
    zval *memory;
    zval *used_detail;
    zval *recycle;
    zval *lock;
    zend_bool calc_hit_ratio;

    object = getThis();
	i_obj = (php_shmcache_t *) shmcache_get_object(object);

    calc_hit_ratio = true;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b",
			&calc_hit_ratio) == FAILURE)
	{
		logError("file: "__FILE__", line: %d, "
			"zend_parse_parameters fail!", __LINE__);
		RETURN_FALSE;
	}

    shmcache_stats_ex(i_obj->context, &stats, calc_hit_ratio);
    array_init(return_value);

    {
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
    zend_add_assoc_long_ex(hashtable, "incr.total",
            sizeof("incr.total"), stats.shm.hashtable.incr.total);
    zend_add_assoc_long_ex(hashtable, "incr.success",
            sizeof("incr.success"), stats.shm.hashtable.incr.success);
    zend_add_assoc_long_ex(hashtable, "get.total",
            sizeof("get.total"), stats.shm.hashtable.get.total);
    zend_add_assoc_long_ex(hashtable, "get.success",
            sizeof("get.success"), stats.shm.hashtable.get.success);
    zend_add_assoc_long_ex(hashtable, "del.total",
            sizeof("del.total"), stats.shm.hashtable.del.total);
    zend_add_assoc_long_ex(hashtable, "del.success",
            sizeof("del.success"), stats.shm.hashtable.del.success);
    zend_add_assoc_long_ex(hashtable, "last_clear_time",
            sizeof("last_clear_time"),
            stats.shm.hashtable.last_clear_time);
    zend_add_assoc_double_ex(hashtable, "get.qps",
            sizeof("get.qps"), stats.hit.get_qps);
    if (stats.hit.ratio >= 0.00) {
        char hit_ratio[32];
        int ratio_len;
        ratio_len = sprintf(hit_ratio, "%.2f%%", stats.hit.ratio * 100.00);
        zend_add_assoc_stringl_ex(hashtable, "hit.ratio",
                sizeof("hit.ratio"), hit_ratio, ratio_len, 1);
    } else {
        zend_add_assoc_stringl_ex(hashtable, "hit.ratio",
                sizeof("hit.ratio"), "-", 1, 1);
    }
    zend_add_assoc_long_ex(hashtable, "hit.last_seconds",
            sizeof("hit.last_seconds"), stats.hit.seconds);
    }

    {
    ALLOC_INIT_ZVAL(memory);
    array_init(memory);
    zend_add_assoc_zval_ex(return_value, "memory",
            sizeof("memory"), memory);
    zend_add_assoc_long_ex(memory, "total",
            sizeof("total"), stats.memory.max);
    zend_add_assoc_long_ex(memory, "alloced",
            sizeof("alloced"), stats.memory.usage.alloced);
    zend_add_assoc_long_ex(memory, "used",
            sizeof("used"), stats.memory.used);
    zend_add_assoc_long_ex(memory, "free",
            sizeof("free"), stats.memory.max - stats.memory.used);
    }

    {
    ALLOC_INIT_ZVAL(used_detail);
    array_init(used_detail);
    zend_add_assoc_zval_ex(memory, "used_detail",
            sizeof("used_detail"), used_detail);
    zend_add_assoc_long_ex(used_detail, "common",
            sizeof("common"), stats.memory.usage.used.common);
    zend_add_assoc_long_ex(used_detail, "entry",
            sizeof("entry"), stats.memory.usage.used.entry);
    zend_add_assoc_long_ex(used_detail, "key",
            sizeof("key"), stats.memory.usage.used.key);
    zend_add_assoc_long_ex(used_detail, "value",
            sizeof("value"), stats.memory.usage.used.value);
    }

    {
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
    }

    {
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
}

/* boolean ShmCache::clear()
 * return true for success, false for fail
 */
static PHP_METHOD(ShmCache, clear)
{
	zval *object;
	php_shmcache_t *i_obj;

    object = getThis();
	i_obj = (php_shmcache_t *) shmcache_get_object(object);

    RETURN_BOOL(shmcache_clear(i_obj->context) == 0);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, 0, 1)
ZEND_ARG_INFO(0, config_filename)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_set, 0, 0, 3)
ZEND_ARG_INFO(0, key)
ZEND_ARG_INFO(0, value)
ZEND_ARG_INFO(0, ttl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_setExpires, 0, 0, 2)
ZEND_ARG_INFO(0, key)
ZEND_ARG_INFO(0, expires)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_setTTL, 0, 0, 2)
ZEND_ARG_INFO(0, key)
ZEND_ARG_INFO(0, ttl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_incr, 0, 0, 3)
ZEND_ARG_INFO(0, key)
ZEND_ARG_INFO(0, increment)
ZEND_ARG_INFO(0, ttl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_get, 0, 0, 1)
ZEND_ARG_INFO(0, key)
ZEND_ARG_INFO(0, returnExpired)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_getExpires, 0, 0, 1)
ZEND_ARG_INFO(0, key)
ZEND_ARG_INFO(0, returnExpired)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_delete, 0, 0, 1)
ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_stats, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_clear, 0, 0, 0)
ZEND_END_ARG_INFO()

#define SHMC_ME(name, args) PHP_ME(ShmCache, name, args, ZEND_ACC_PUBLIC)
static zend_function_entry shmcache_class_methods[] = {
    SHMC_ME(__construct,  arginfo___construct)
    SHMC_ME(set,          arginfo_set)
    SHMC_ME(setExpires,   arginfo_setExpires)
    SHMC_ME(setTTL,       arginfo_setTTL)
    SHMC_ME(incr,         arginfo_incr)
    SHMC_ME(get,          arginfo_get)
    SHMC_ME(getExpires,   arginfo_getExpires)
    SHMC_ME(delete,       arginfo_delete)
    SHMC_ME(stats,        arginfo_stats)
    SHMC_ME(clear,        arginfo_clear)
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
	shmcache_object_handlers.free_obj = php_shmcache_destroy;
	shmcache_object_handlers.clone_obj = NULL;
#endif

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

     zend_declare_class_constant_long(shmcache_ce, ZEND_STRL("MAX_KEY_SIZE"),
             SHMCACHE_MAX_KEY_SIZE TSRMLS_CC);
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
	char info[64];
	sprintf(info, "shmcache v%d.%d.%d support",
		SHMCACHE_MAJOR_VERSION, SHMCACHE_MINOR_VERSION,
        SHMCACHE_PATCH_VERSION);

	php_info_print_table_start();
	php_info_print_table_header(2, info, "enabled");
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
		SHMCACHE_MAJOR_VERSION, SHMCACHE_MINOR_VERSION,
        SHMCACHE_PATCH_VERSION);

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

static struct shmcache_context *shmcache_get_context(const char *filename,
        char *error_info, const int error_size)
{
    int i;
    int bytes;
    char *config_filename;
    struct shmcache_context *context;

    for (i=0; i<context_array.count; i++) {
        if (strcmp(filename, context_array.contexts[i].config_filename) == 0) {
            return context_array.contexts[i].context;
        }
    }

    if (context_array.count >= context_array.alloc) {
        int alloc;
        struct shmcache_context_by_file *contexts;
        if (context_array.alloc == 0) {
            alloc = 2;
        } else {
            alloc = context_array.alloc * 2;
        }

        bytes = sizeof(struct shmcache_context_by_file) * alloc;
        contexts = (struct shmcache_context_by_file *)malloc(bytes);
        if (contexts == NULL) {
            snprintf(error_info, error_size,
                    "malloc %d bytes fail", bytes);
            return NULL;
        }
        memset(contexts, 0, bytes);
        if (context_array.count > 0) {
            memcpy(contexts, context_array.contexts, context_array.count *
                    sizeof(struct shmcache_context_by_file));
            free(context_array.contexts);
        }
        context_array.contexts = contexts;
        context_array.alloc = alloc;
    }

    config_filename = strdup(filename);
    if (config_filename == NULL) {
        snprintf(error_info, error_size,
			"strdup filename: %s fail", filename);
        return NULL;
    }
    context = (struct shmcache_context *)malloc(
            sizeof(struct shmcache_context));
    if (context == NULL) {
        snprintf(error_info, error_size, "malloc %d bytes fail",
                (int)sizeof(struct shmcache_context));
        return NULL;
    }

    if (shmcache_init_from_file(context, config_filename) != 0) {
        sprintf(error_info, "shmcache_init_from_file: %s fail",
                config_filename);
        free(config_filename);
        free(context);
        return NULL;
    }

    context_array.contexts[context_array.count].
        config_filename = config_filename;
    context_array.contexts[context_array.count].context = context;
    context_array.count++;
    return context;
}
