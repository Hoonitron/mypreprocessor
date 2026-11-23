#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "buffer.h"
#include "file_inclusion.h"

static int is_regular_file(const char *path)
{
    struct stat st;

    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

char *read_whole_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    Buffer b;
    char chunk[4096];
    size_t n;

    if (fp == NULL)
        return NULL;
    buf_init(&b);
    while ((n = fread(chunk, 1, sizeof chunk, fp)) > 0)
        buf_putn(&b, chunk, n);
    fclose(fp);
    return b.data;
}

char *include_target(const char *rest)
{
    char closing;
    const char *end;

    if (*rest == '<')
        closing = '>';
    else if (*rest == '"')
        closing = '"';
    else
        return NULL;

    end = strchr(rest + 1, closing);
    if (end == NULL || end == rest + 1)
        return NULL;
    return xstrndup(rest + 1, (size_t)(end - rest - 1));
}

char *find_include(const char *name, const char *current_file)
{
    const char *slash = strrchr(current_file, '/');
    char *candidate;
    size_t dir_len;

    if (name[0] == '/')
        return is_regular_file(name) ? xstrdup(name) : NULL;

    if (slash != NULL) {
        dir_len = (size_t)(slash - current_file) + 1;
        candidate = xmalloc(dir_len + strlen(name) + 1);
        memcpy(candidate, current_file, dir_len);
        strcpy(candidate + dir_len, name);
        if (is_regular_file(candidate))
            return candidate;
        free(candidate);
    }

    if (is_regular_file(name))
        return xstrdup(name);
    return NULL;
}
