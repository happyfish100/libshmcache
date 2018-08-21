#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "fastcommon/logger.h"
#include "fastcommon/hash.h"
#include "shmcache/shm_hashtable.h"
#include "shmcache/shmcache.h"

static char *config_filename = "/etc/libshmcache.conf";
static char *key_value_seperator = "=";
static char *row_seperator = "\n";
static int start_offset = 0;
static int row_count = 0;
static char *search_key  = NULL;
static bool show_usage = false;
static char *output_filename = NULL;

static void usage(char *program)
{
    fprintf(stderr, "Usage: %s options, the options as:\n"
            "\t -h help\n"
            "\t -c <config_filename>, default: /etc/libshmcache.conf\n"
            "\t -F <key_value_seperator>, default: =\n"
            "\t -R <row_seperator>, default: \\n\n"
            "\t -k <search_key>, SQL like style such as "
            "MYKEY%% or %%MYKEY or %%MYKEY%%\n"
            "\t -s <start> start offset, default: 0\n"
            "\t -n <row_count>, 0 means NO limit, default: 0\n"
            "\t -o <filename> output filename, default: output to sdtout\n"
            "\n"
            "\t the seperators can use escape characters, such as: \\\\ for \\, \n"
            "\t \\t for tab, \\r for carriage return, \\n for new line\n"
            "\n", program);
}

static char *unescape_string(const char *s)
{
    char *output;
    char *dest;
    const char *p;
    const char *end;
    const char *start;
    int src_len;
    int num_len;
    char buff[16];

    src_len = strlen(s);
    output = (char *)malloc(src_len + 1);
    if (output == NULL) {
        fprintf(stderr, "malloc %d bytes fail", src_len + 1);
        return NULL;
    }

    dest = output;
    end = s + src_len;
    p = s;
    while (p < end) {
        if (*p == '\\' && p + 1 < end) {
            switch (*(p+1)) {
                case 'r':
                    *dest++ = '\r';
                    break;
                case 'n':
                    *dest++ = '\n';
                    break;
                case 't':
                    *dest++ = '\t';
                    break;
                case 'f':
                    *dest++ = '\f';
                    break;
                case '\\':
                    *dest++ = '\\';
                    break;
                default:
                    if (*(p+1) >= '0' && *(p+1) <= '9') {
                        start = p + 1;
                        p += 2;
                        while ((p < end) && (*p >= '0' && *p <= '9')) {
                            p++;
                            if (p - start == 3) {
                                break;
                            }
                        }
                        num_len = p - start;
                        memcpy(buff, start, num_len);
                        *(buff + num_len) = '\0';
                        *dest++ = atoi(buff);
                    } else {
                        *dest++ = *p++;
                        *dest++ = *p++;
                    }
                    continue;
            }
            p += 2;
        } else {
            *dest++ = *p++;
        }
    }

    *dest = '\0';
    return output;
}

static void parse_args(int argc, char **argv)
{
    int ch;

    while ((ch = getopt(argc, argv, "hc:F:R:k:s:n:o:")) != -1) {
        switch (ch) {
            case 'c':
                config_filename = optarg;
                break;
            case 'F':
                key_value_seperator = unescape_string(optarg);
                break;
            case 'R':
                row_seperator = unescape_string(optarg);
                break;
            case 'k':
                search_key = optarg;
                break;
            case 's':
                start_offset = atoi(optarg);
                break;
            case 'n':
                row_count = atoi(optarg);
                break;
            case 'o':
                output_filename = optarg;
                break;
            case 'h':
            default:
                show_usage = true;
                usage(argv[0]);
                return;
        }
    }

    if (optind != argc) {
        show_usage = true;
        usage(argv[0]);
    }
}

static struct shmcache_match_key_info *parse_search_key(
        struct shmcache_match_key_info *key_info)
{
    if (search_key == NULL || *search_key == '\0') {
        return NULL;
    }

    key_info->op_type = 0;
    key_info->key = search_key;
    key_info->length = strlen(search_key);
    if (*search_key == '%') {
        key_info->op_type |= SHMCACHE_MATCH_KEY_OP_RIGHT;
        key_info->key++;
        key_info->length--;
        if (key_info->length == 0) {
            return NULL;
        }
    }

    if (key_info->key[key_info->length - 1] == '%') {
        key_info->op_type |= SHMCACHE_MATCH_KEY_OP_LEFT;
        key_info->length--;
        if (key_info->length == 0) {
            return NULL;
        }
    }

    return key_info;
}

static int do_dump(struct shmcache_context *context, FILE *fp)
{
    int result;
    struct shmcache_hentry_array array;
    struct shmcache_hash_entry *entry;
    struct shmcache_hash_entry *end;
    struct shmcache_match_key_info key_info;
    struct shmcache_match_key_info *pkey;
    
    pkey = parse_search_key(&key_info);
    if ((result=shm_ht_to_array_ex(context, &array, pkey,
                    start_offset, row_count)) != 0)
    {
        return result;
    }

    end = array.entries + array.count;
    for (entry=array.entries; entry<end; entry++) {
        if ((entry->value.options & SHMCACHE_SERIALIZER_STRING) != 0) {
            fprintf(fp, "%.*s%s%.*s%s",
                    entry->key.length, entry->key.data,
                    key_value_seperator,
                    entry->value.length, entry->value.data,
                    row_seperator);
        } else {
            fprintf(fp, "%.*s%s<%s serializer, data length: %d>%s",
                    entry->key.length, entry->key.data,
                    key_value_seperator,
                    shmcache_get_serializer_label(entry->value.options),
                    entry->value.length, row_seperator);
        }
    }

    shm_ht_free_array(&array);
    return 0;
}

int main(int argc, char **argv)
{
	int result;
    struct shmcache_context context;
    FILE *fp;

    parse_args(argc, argv);
    if (show_usage) {
        return 0;
    }

	log_init();
    if (output_filename != NULL) {
        fp = fopen(output_filename, "wb");
        if (fp == NULL) {
            result = errno != 0 ? errno : EPERM;
            fprintf(stderr, "open file %s to write fail, "
                    "error info: %s\n", output_filename, strerror(result));
            return result;
        }
    } else {
        fp = stdout;
    }

    if ((result=shmcache_init_from_file_ex(&context,
                    config_filename, false, true)) != 0)
    {
        return result;
    }

    result = do_dump(&context, fp);
    if (fp != stdout) {
        fclose(fp);
    }
    shmcache_destroy(&context);
    return result;
}
