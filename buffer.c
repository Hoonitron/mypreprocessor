#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffer.h"

void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);

    if (p == NULL) {
        fputs("mypreprocessor: out of memory\n", stderr);
        exit(1);
    }
    return p;
}

void *xrealloc(void *p, size_t n)
{
    void *q = realloc(p, n ? n : 1);

    if (q == NULL) {
        fputs("mypreprocessor: out of memory\n", stderr);
        exit(1);
    }
    return q;
}

char *xstrndup(const char *s, size_t n)
{
    char *r = xmalloc(n + 1);

    memcpy(r, s, n);
    r[n] = '\0';
    return r;
}

char *xstrdup(const char *s)
{
    return xstrndup(s, strlen(s));
}

void buf_init(Buffer *b)
{
    b->cap = 64;
    b->len = 0;
    b->data = xmalloc(b->cap);
    b->data[0] = '\0';
}

static void reserve(Buffer *b, size_t extra)
{
    if (b->len + extra + 1 <= b->cap)
        return;
    while (b->len + extra + 1 > b->cap)
        b->cap *= 2;
    b->data = xrealloc(b->data, b->cap);
}

void buf_putc(Buffer *b, char c)
{
    reserve(b, 1);
    b->data[b->len++] = c;
    b->data[b->len] = '\0';
}

void buf_putn(Buffer *b, const char *s, size_t n)
{
    reserve(b, n);
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

void buf_puts(Buffer *b, const char *s)
{
    buf_putn(b, s, strlen(s));
}

void buf_free(Buffer *b)
{
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}
