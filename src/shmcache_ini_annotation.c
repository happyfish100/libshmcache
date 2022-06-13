//shmcache_ini_annotation.c

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fastcommon/logger.h"
#include "fastcommon/shared_func.h"
#include "fastcommon/json_parser.h"
#include "shmcache.h"
#include "shmcache_ini_annotation.h"

typedef struct {
    struct shmcache_context shm_ctx;
    fc_json_context_t json_ctx;
} ShmcacheAnnotationArgs;

static int config_get(IniContext *context,
        struct ini_annotation_entry *annotation,
        const IniItem *item, char **pOutValue, int max_values)
{
    ShmcacheAnnotationArgs *args;
    int count;
    int result;
    struct shmcache_key_info key;
    struct shmcache_value_info value;
    char *output;

    args = (ShmcacheAnnotationArgs *)annotation->arg;
    if (item->value[0] != '\0') {
        key.data = (char *)item->value;
    } else {
        key.data = (char *)item->name;
    }

    value.options = SHMCACHE_SERIALIZER_STRING;
    key.length = strlen(key.data);
    if ((result= shmcache_get(&args->shm_ctx, &key, &value)) != 0) {
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
                shmcache_get_serializer_label(value.options),
                value.options);
        value.data = "";
        value.length = 0;
    }

    count = 0;
    if ((value.options & SHMCACHE_SERIALIZER_LIST) ==
            SHMCACHE_SERIALIZER_LIST)
    {
        string_t input;
        const fc_json_array_t *array;

        input.str = value.data;
        input.len = value.length;
        if ((array=fc_decode_json_array(&args->json_ctx, &input)) != NULL) {
            string_t *el;
            string_t *end;

            end = array->elements + array->count;
            if (array->count > max_values) {
                end = array->elements + max_values;
            }

            for (el=array->elements; el<end; el++) {
                output = fc_strdup1(el->str, el->len);
                if (output == NULL) {
                    return count;
                }
                pOutValue[count++] = output;
            }
        } else {
            logError("file: "__FILE__", line: %d, "
                    "decode json array fail, %s", __LINE__,
                    fc_json_parser_get_error_info(&args->json_ctx)->str);
            pOutValue[count++] = strdup("");
        }
    } else {
        output = fc_strdup1(value.data, value.length);
        if (output == NULL) {
            return count;
        }
        pOutValue[count++] = output;
    }

    return count;
}

static void annotation_destroy(struct ini_annotation_entry *annotation)
{
    ShmcacheAnnotationArgs *args;

    if (annotation->arg != NULL) {
        args = (ShmcacheAnnotationArgs *)annotation->arg;
        shmcache_destroy(&args->shm_ctx);
        fc_destroy_json_context(&args->json_ctx);
        annotation->arg = NULL;
    }
}

int CONFIG_GET_init_annotation(AnnotationEntry *annotation,
        const char *config_filename)
{
    ShmcacheAnnotationArgs *args;
    int result;

    memset(annotation, 0, sizeof(AnnotationEntry));
    annotation->func_name = "CONFIG_GET";
    annotation->func_get = config_get;
    annotation->func_free = iniAnnotationFreeValues;
    annotation->func_destroy = annotation_destroy;

    args = (ShmcacheAnnotationArgs *)malloc(
            sizeof(ShmcacheAnnotationArgs));
    if (args == NULL) {
        logError("file: "__FILE__", line: %d, "
                "malloc %d bytes fail", __LINE__,
                (int)sizeof(ShmcacheAnnotationArgs));
        return ENOMEM;
    }

    if ((result=shmcache_init_from_file_ex(&args->shm_ctx,
                    config_filename, false, true)) != 0)
    {
        free(args);
        return result;
    }

    if ((result=fc_init_json_context(&args->json_ctx)) != 0) {
        free(args);
        return result;
    }

    annotation->arg = args;
    return 0;
}
