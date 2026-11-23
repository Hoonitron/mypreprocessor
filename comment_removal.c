#include "buffer.h"
#include "comment_removal.h"

static void put_newline(Buffer *b)
{
    while (b->len > 0 && (b->data[b->len - 1] == ' ' || b->data[b->len - 1] == '\t'))
        b->len--;
    b->data[b->len] = '\0';
    buf_putc(b, '\n');
}

static size_t copy_literal(const char *s, Buffer *out)
{
    char quote = s[0];
    size_t i = 1;

    buf_putc(out, quote);
    while (s[i] != '\0' && s[i] != '\n') {
        buf_putc(out, s[i]);
        if (s[i] == '\\' && s[i + 1] != '\0' && s[i + 1] != '\n') {
            i++;
            buf_putc(out, s[i]);
        } else if (s[i] == quote) {
            return i + 1;
        }
        i++;
    }
    return i;
}

char *remove_comments(const char *src)
{
    Buffer out;
    size_t i = 0;

    buf_init(&out);
    while (src[i] != '\0') {
        char c = src[i];

        if (c == '"' || c == '\'') {
            i += copy_literal(src + i, &out);
        } else if (c == '/' && src[i + 1] == '/') {
            i += 2;
            while (src[i] != '\0' && src[i] != '\n')
                i++;
        } else if (c == '/' && src[i + 1] == '*') {
            i += 2;
            while (src[i] != '\0' && !(src[i] == '*' && src[i + 1] == '/')) {
                if (src[i] == '\n')
                    put_newline(&out);
                i++;
            }
            if (src[i] != '\0')
                i += 2;
            buf_putc(&out, ' ');
        } else if (c == '\n') {
            put_newline(&out);
            i++;
        } else {
            buf_putc(&out, c);
            i++;
        }
    }
    return out.data;
}
