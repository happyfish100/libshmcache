#include "fastcommon/logger.h"
#include "shmcache/shmcache.h"
#include "org_csource_shmcache_ShmCache.h"

static void throws_exception(JNIEnv *env, const char *message)
{
    jclass exClass;
    exClass = (*env)->FindClass(env, "java/lang/Exception");
    if (exClass == NULL) {
        return;
    }
    (*env)->ThrowNew(env, exClass, message);
}

jlong JNICALL Java_org_csource_shmcache_ShmCache_doInit
  (JNIEnv *env, jobject obj, jstring config_filename)
{

    struct shmcache_context *context;
    jboolean *isCopy = NULL;
    const char *szFilename;
    int result;

    log_init();
    context = (struct shmcache_context *)malloc(sizeof(struct shmcache_context));
    if (context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "malloc %d bytes fail", __LINE__,
                (int)sizeof(struct shmcache_context));
        throws_exception(env, "out of memory");
        return 0;
    }

    szFilename = (*env)->GetStringUTFChars(env, config_filename, isCopy);
    result = shmcache_init_from_file(context, szFilename);

    (*env)->ReleaseStringUTFChars(env, config_filename, szFilename);
    return result == 0 ? (long)context : 0;
}

void JNICALL Java_org_csource_shmcache_ShmCache_doDestroy
  (JNIEnv *env, jobject obj, jlong handler)
{
    struct shmcache_context *context;

    context = (struct shmcache_context *)handler;
    if (context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "empty handler", __LINE__);
        throws_exception(env, "empty handler");
        return;
    }

    shmcache_destroy(context);
    free(context);
}

void JNICALL Java_org_csource_shmcache_ShmCache_doSetVO
  (JNIEnv *env, jobject obj, jlong handler, jstring key, jobject vo)
{
    struct shmcache_context *context;
    struct shmcache_key_info shmcache_key;
    struct shmcache_value_info shmcache_value;
    jboolean *isCopy = NULL;
    jclass value_class;
    jmethodID value_mid;
    jmethodID options_mid;
    jmethodID expires_mid;
    jbyteArray value;
    int result;

    context = (struct shmcache_context *)handler;
    if (context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "empty handler", __LINE__);
        throws_exception(env, "empty handler");
        return;
    }

    shmcache_key.length = (*env)->GetStringUTFLength(env, key);
    shmcache_key.data = (char *)((*env)->GetStringUTFChars(env, key, isCopy));

    value_class = (*env)->GetObjectClass(env, vo);
    value_mid = (*env)->GetMethodID(env, value_class, "getValue", "()[B");
    options_mid = (*env)->GetMethodID(env, value_class, "getOptions", "()I");
    expires_mid = (*env)->GetMethodID(env, value_class, "getExpires", "()J");

    value = (*env)->CallObjectMethod(env, vo, value_mid);
    shmcache_value.options = (*env)->CallIntMethod(env, vo, options_mid);
    shmcache_value.expires = (*env)->CallLongMethod(env, vo, expires_mid) / 1000;

    shmcache_value.data = (char *)((*env)->GetByteArrayElements(env, value, isCopy));
    shmcache_value.length = (*env)->GetArrayLength(env, value);

    result = shmcache_set_ex(context, &shmcache_key, &shmcache_value);

    (*env)->ReleaseStringUTFChars(env, key, shmcache_key.data);
    (*env)->ReleaseByteArrayElements(env, value, (jbyte *)shmcache_value.data, 0);
    if (result != 0) {
        throws_exception(env, strerror(result));
    }
}

void JNICALL Java_org_csource_shmcache_ShmCache_doSet
  (JNIEnv *env, jobject obj, jlong handler, jstring key,
   jbyteArray value, jint ttl)
{
    struct shmcache_context *context;
    struct shmcache_key_info shmcache_key;
    jboolean *isCopy = NULL;
    jbyte *szValue;
    jsize value_len;
    int result;

    context = (struct shmcache_context *)handler;
    if (context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "empty handler", __LINE__);
        throws_exception(env, "empty handler");
        return;
    }

    shmcache_key.length = (*env)->GetStringUTFLength(env, key);
    shmcache_key.data = (char *)((*env)->GetStringUTFChars(env, key, isCopy));

    szValue = (*env)->GetByteArrayElements(env, value, isCopy);
    value_len =  (*env)->GetArrayLength(env, value);

    result = shmcache_set(context, &shmcache_key,
            (const char *)szValue, value_len, ttl);

    (*env)->ReleaseStringUTFChars(env, key, shmcache_key.data);
    (*env)->ReleaseByteArrayElements(env, value, szValue, 0);
    if (result != 0) {
        throws_exception(env, strerror(result));
    }
}

jlong JNICALL Java_org_csource_shmcache_ShmCache_doIncr
  (JNIEnv *env, jobject obj, jlong handler, jstring key,
   jlong increment, jint ttl)
{
    struct shmcache_context *context;
    struct shmcache_key_info shmcache_key;
    jboolean *isCopy = NULL;
    int result;
    int64_t new_value = 0;

    context = (struct shmcache_context *)handler;
    if (context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "empty handler", __LINE__);
        throws_exception(env, "empty handler");
        return new_value;
    }

    shmcache_key.length = (*env)->GetStringUTFLength(env, key);
    shmcache_key.data = (char *)((*env)->GetStringUTFChars(env, key, isCopy));

    result = shmcache_incr(context, &shmcache_key, increment, ttl, &new_value);
    (*env)->ReleaseStringUTFChars(env, key, shmcache_key.data);

    if (result != 0) {
        throws_exception(env, strerror(result));
    }
    return new_value;
}

jobject JNICALL Java_org_csource_shmcache_ShmCache_doGet
  (JNIEnv *env, jobject obj, jlong handler, jstring key)
{
#define VALUE_CLASS_NAME "org/csource/shmcache/ShmCache$Value"

    struct shmcache_context *context;
    jboolean *isCopy = NULL;
    int result;
    struct shmcache_key_info shmcache_key;
    struct shmcache_value_info shmcache_value;
    jbyteArray value;
    jobject vo;

    context = (struct shmcache_context *)handler;
    if (context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "empty handler", __LINE__);
        throws_exception(env, "empty handler");
        return NULL;
    }

    shmcache_key.length = (*env)->GetStringUTFLength(env, key);
    shmcache_key.data = (char *)((*env)->GetStringUTFChars(env, key, isCopy));
    result = shmcache_get(context, &shmcache_key, &shmcache_value);
    (*env)->ReleaseStringUTFChars(env, key, shmcache_key.data);

    if (result == 0) {
        char error_info[256];
        jclass value_class;
        jmethodID constructor_mid;

        value_class = (*env)->FindClass(env, VALUE_CLASS_NAME);
        if (value_class == NULL) {
            sprintf(error_info, "can find class: %s", VALUE_CLASS_NAME);
            logError("file: "__FILE__", line: %d, %s",
                    __LINE__, error_info);
            throws_exception(env, error_info);
            return 0;
        }

        constructor_mid = (*env)->GetMethodID(env, value_class,
                "<init>", "([BIJ)V");
        if (constructor_mid == NULL) {
            sprintf(error_info, "can find constructor for class %s", VALUE_CLASS_NAME);
            logError("file: "__FILE__", line: %d, %s",
                    __LINE__, error_info);
            throws_exception(env, error_info);
            return 0;
        }

        value = (*env)->NewByteArray(env, shmcache_value.length);
        (*env)->SetByteArrayRegion(env, value, 0, shmcache_value.length,
                (const jbyte *)shmcache_value.data);

        vo = (*env)->NewObject(env, value_class, constructor_mid,
                value, shmcache_value.options,
                (int64_t)shmcache_value.expires * 1000);
    } else {
        vo = NULL;
    }

    return vo;
}

jbyteArray JNICALL Java_org_csource_shmcache_ShmCache_doGetBytes
  (JNIEnv *env, jobject obj, jlong handler, jstring key)
{
    struct shmcache_context *context;
    jboolean *isCopy = NULL;
    int result;
    struct shmcache_key_info shmcache_key;
    struct shmcache_value_info shmcache_value;
    jbyteArray value;

    context = (struct shmcache_context *)handler;
    if (context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "empty handler", __LINE__);
        throws_exception(env, "empty handler");
        return NULL;
    }

    shmcache_key.length = (*env)->GetStringUTFLength(env, key);
    shmcache_key.data = (char *)((*env)->GetStringUTFChars(env, key, isCopy));
    result = shmcache_get(context, &shmcache_key, &shmcache_value);
    (*env)->ReleaseStringUTFChars(env, key, shmcache_key.data);

    if (result == 0) {
        value = (*env)->NewByteArray(env, shmcache_value.length);
        (*env)->SetByteArrayRegion(env, value, 0, shmcache_value.length,
                (const jbyte *)shmcache_value.data);
    } else {
        value = NULL;
    }
    return value;
}

jboolean JNICALL Java_org_csource_shmcache_ShmCache_doDelete
  (JNIEnv *env, jobject obj, jlong handler, jstring key)
{
    struct shmcache_context *context;
    jboolean *isCopy = NULL;
    struct shmcache_key_info shmcache_key;
    int result;

    context = (struct shmcache_context *)handler;
    if (context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "empty handler", __LINE__);
        throws_exception(env, "empty handler");
        return false;
    }

    shmcache_key.length = (*env)->GetStringUTFLength(env, key);
    shmcache_key.data = (char *)((*env)->GetStringUTFChars(env, key, isCopy));
    result = shmcache_delete(context, &shmcache_key);
    (*env)->ReleaseStringUTFChars(env, key, shmcache_key.data);
    return (result == 0);
}

jboolean JNICALL Java_org_csource_shmcache_ShmCache_doSetTTL
  (JNIEnv *env, jobject obj, jlong handler, jstring key, jint ttl)
{
    struct shmcache_context *context;
    jboolean *isCopy = NULL;
    struct shmcache_key_info shmcache_key;
    int result;

    context = (struct shmcache_context *)handler;
    if (context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "empty handler", __LINE__);
        throws_exception(env, "empty handler");
        return false;
    }

    shmcache_key.length = (*env)->GetStringUTFLength(env, key);
    shmcache_key.data = (char *)((*env)->GetStringUTFChars(env, key, isCopy));
    result = shmcache_set_ttl(context, &shmcache_key, ttl);
    (*env)->ReleaseStringUTFChars(env, key, shmcache_key.data);
    return (result == 0);
}

jlong JNICALL Java_org_csource_shmcache_ShmCache_doGetExpires
  (JNIEnv *env, jobject obj, jlong handler, jstring key)
{
    struct shmcache_context *context;
    jboolean *isCopy = NULL;
    struct shmcache_key_info shmcache_key;
    struct shmcache_value_info shmcache_value;
    int result;

    context = (struct shmcache_context *)handler;
    if (context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "empty handler", __LINE__);
        throws_exception(env, "empty handler");
        return false;
    }

    shmcache_key.length = (*env)->GetStringUTFLength(env, key);
    shmcache_key.data = (char *)((*env)->GetStringUTFChars(env, key, isCopy));
    result = shmcache_get(context, &shmcache_key, &shmcache_value);
    (*env)->ReleaseStringUTFChars(env, key, shmcache_key.data);
    if (result == 0) {
        return shmcache_value.expires;
    } else {
        return -1;
    }
}

jboolean JNICALL Java_org_csource_shmcache_ShmCache_doClear
  (JNIEnv *env, jobject obj, jlong handler)
{
    struct shmcache_context *context;

    context = (struct shmcache_context *)handler;
    if (context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "empty handler", __LINE__);
        throws_exception(env, "empty handler");
        return false;
    }

    return shmcache_clear(context) == 0;
}

jlong JNICALL Java_org_csource_shmcache_ShmCache_doGetLastClearTime
  (JNIEnv *env, jobject obj, jlong handler)
{
    struct shmcache_context *context;

    context = (struct shmcache_context *)handler;
    if (context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "empty handler", __LINE__);
        throws_exception(env, "empty handler");
        return 0;
    }

    return shmcache_get_last_clear_time(context);
}

jlong JNICALL Java_org_csource_shmcache_ShmCache_doGetInitTime
  (JNIEnv *env, jobject obj, jlong handler)
{
    struct shmcache_context *context;

    context = (struct shmcache_context *)handler;
    if (context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "empty handler", __LINE__);
        throws_exception(env, "empty handler");
        return 0;
    }

    return shmcache_get_init_time(context);
}
