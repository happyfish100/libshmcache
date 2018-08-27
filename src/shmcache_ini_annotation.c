//shmcache_ini_annotation.c

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fastcommon/logger.h"
#include "fastcommon/shared_func.h"
#include "fastcommon/json_parser.h"
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

    value.options = SHMCACHE_SERIALIZER_STRING;
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
    } else if ((value.options & SHMCACHE_SERIALIZER_STRING) == 0) {
        logError("file: "__FILE__", line: %d, "
                "unsupport serilizer %s (%d)", __LINE__,
                shmcache_get_serializer_label(value.options), value.options);
        value.data = "";
        value.length = 0;
    }

    count = 0;
    if ((value.options & SHMCACHE_SERIALIZER_LIST) == SHMCACHE_SERIALIZER_LIST) {
        string_t input;
        json_array_t array;
        char error_info[256];

        input.str = value.data;
        input.len = value.length;
        if (decode_json_array(&input, &array, error_info,
                    sizeof(error_info)) == 0) {
            string_t *el;
            string_t *end;

            end = array.elements + array.count;
            if (array.count > max_values) {
                end = array.elements + max_values;
            }

            for (el=array.elements; el<end; el++) {
                output = fc_strdup(el->str, el->len);
                if (output == NULL) {
                    return count;
                }
                pOutValue[count++] = output;
            }

            free_json_array(&array);
        } else {
            logError("file: "__FILE__", line: %d, "
                    "decode json array fail, %s",
                    __LINE__, error_info);
            pOutValue[count++] = strdup("");
        }
    } else {
        output = fc_strdup(value.data, value.length);
        if (output == NULL) {
            return count;
        }
        pOutValue[count++] = output;
    }

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
