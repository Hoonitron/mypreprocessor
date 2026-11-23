#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Buffer;

void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t n);

void buf_init(Buffer *b);
void buf_putc(Buffer *b, char c);
void buf_putn(Buffer *b, const char *s, size_t n);
void buf_puts(Buffer *b, const char *s);
void buf_free(Buffer *b);

#endif
