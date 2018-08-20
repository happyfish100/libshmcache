//shmcache_ini_annotation.c

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fastcommon/logger.h"
#include "fastcommon/shared_func.h"
#include "fastcommon/sched_thread.h"
#include "shmcache.h"
#include "shmcache_ini_annotation.h"

static int config_get(IniContext *context,
        struct ini_annotation_entry *annotation,
        const IniItem *item, char **pOutValue, int max_values)
{
    struct shmcache_context *shm_context;
    int count;
    int result;
    struct shmcache_key_info key;
    struct shmcache_value_info value;
    char *output;

    shm_context = (struct shmcache_context *)annotation->arg;
    if (item->value[0] != '\0') {
        key.data = (char *)item->value;
    } else {
        key.data = (char *)item->name;
    }

    key.length = strlen(key.data);
    if ((result= shmcache_get(shm_context, &key, &value)) != 0) {
        if (result == ENOENT || result == ETIMEDOUT) {
            logError("file: "__FILE__", line: %d, "
                    "shmcache_get fail, key: %s not exist",
                    __LINE__, key.data);
        } else {
            logError("file: "__FILE__", line: %d, "
                    "shmcache_get fail, key: %s, error info: %s",
                    __LINE__, key.data, strerror(result));
        }
        value.data = "";
        value.length = 0;
    }

    count = 0;
    output = (char *)malloc(value.length + 1);
    if (output == NULL) {
        logError("file: "__FILE__", line: %d, "
                "malloc %d bytes fail",
                __LINE__, value.length + 1);
        return count;
    }

    if (value.length > 0) {
        memcpy(output, value.data, value.length);
    }
    *(output + value.length) = '\0';

    pOutValue[count++] = output;
    return count;
}

static void annotation_destroy(struct ini_annotation_entry *annotation)
{
    if (annotation->arg != NULL) {
        shmcache_destroy((struct shmcache_context *)annotation->arg);
        annotation->arg = NULL;
    }
}

int CONFIG_GET_init_annotation(AnnotationEntry *annotation,
        const char *config_filename)
{
    struct shmcache_context *shm_context;
    int result;

    memset(annotation, 0, sizeof(AnnotationEntry));
    annotation->func_name = "CONFIG_GET";
    annotation->func_get = config_get;
    annotation->func_free = iniAnnotationFreeValues;
    annotation->func_destroy = annotation_destroy;

    shm_context = (struct shmcache_context *)malloc(
            sizeof(struct shmcache_context));
    if (shm_context == NULL) {
        logError("file: "__FILE__", line: %d, "
                "malloc %d bytes fail", __LINE__,
                (int)sizeof(struct shmcache_context));
        return ENOMEM;
    }

    if ((result=shmcache_init_from_file_ex(shm_context,
                    config_filename, false, true)) != 0)
    {
        free(shm_context);
        return result;
    }

    annotation->arg = shm_context;
    return 0;
}
