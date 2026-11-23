#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "macro_handler.h"

#define MAX_HIDE 64

typedef struct {
    char *name;
    char **params;
    int nparams;
    int is_func;
    char *body;
} Macro;

static Macro *table = NULL;
static size_t table_len = 0;
static size_t table_cap = 0;

static int ident_start(char c)
{
    return isalpha((unsigned char)c) || c == '_';
}

static int ident_char(char c)
{
    return isalnum((unsigned char)c) || c == '_';
}

static size_t ident_length(const char *s)
{
    size_t n = 0;

    if (!ident_start(s[0]))
        return 0;
    while (ident_char(s[n]))
        n++;
    return n;
}

static size_t number_length(const char *s)
{
    size_t n = 0;

    while (ident_char(s[n]) || s[n] == '.')
        n++;
    return n;
}

static size_t literal_length(const char *s)
{
    char quote = s[0];
    size_t i = 1;

    while (s[i] != '\0') {
        if (s[i] == '\\' && s[i + 1] != '\0')
            i += 2;
        else if (s[i] == quote)
            return i + 1;
        else
            i++;
    }
    return i;
}

static const char *skip_blanks(const char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    return s;
}

static char *trimmed_copy(const char *s, size_t n)
{
    while (n > 0 && isspace((unsigned char)*s)) {
        s++;
        n--;
    }
    while (n > 0 && isspace((unsigned char)s[n - 1]))
        n--;
    return xstrndup(s, n);
}

static void free_macro(Macro *m)
{
    int i;

    for (i = 0; i < m->nparams; i++)
        free(m->params[i]);
    free(m->params);
    free(m->name);
    free(m->body);
}

static long find_index(const char *name, size_t len)
{
    size_t i;

    for (i = 0; i < table_len; i++) {
        if (strlen(table[i].name) == len && strncmp(table[i].name, name, len) == 0)
            return (long)i;
    }
    return -1;
}

static Macro *find_macro(const char *name, size_t len)
{
    long i = find_index(name, len);

    return i < 0 ? NULL : &table[i];
}

static void remove_index(size_t i)
{
    free_macro(&table[i]);
    memmove(&table[i], &table[i + 1], (table_len - i - 1) * sizeof(Macro));
    table_len--;
}

static int param_index(const Macro *m, const char *name, size_t len)
{
    int i;

    for (i = 0; i < m->nparams; i++) {
        if (strlen(m->params[i]) == len && strncmp(m->params[i], name, len) == 0)
            return i;
    }
    return -1;
}

static int parse_params(const char **cursor, Macro *m)
{
    const char *p = skip_blanks(*cursor + 1);

    if (*p == ')') {
        *cursor = p + 1;
        return 0;
    }
    for (;;) {
        size_t n;

        p = skip_blanks(p);
        n = ident_length(p);
        if (n == 0)
            return -1;
        m->params = xrealloc(m->params, sizeof(char *) * (size_t)(m->nparams + 1));
        m->params[m->nparams++] = xstrndup(p, n);
        p = skip_blanks(p + n);
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p == ')') {
            *cursor = p + 1;
            return 0;
        }
        return -1;
    }
}

int macro_define(const char *definition)
{
    Macro m;
    size_t n = ident_length(definition);
    const char *p;
    long old;

    if (n == 0)
        return -1;

    m.name = xstrndup(definition, n);
    m.params = NULL;
    m.nparams = 0;
    m.is_func = 0;
    m.body = NULL;

    p = definition + n;
    if (*p == '(') {
        m.is_func = 1;
        if (parse_params(&p, &m) < 0) {
            free_macro(&m);
            return -1;
        }
    }
    m.body = trimmed_copy(p, strlen(p));

    old = find_index(m.name, n);
    if (old >= 0)
        remove_index((size_t)old);

    if (table_len == table_cap) {
        table_cap = table_cap ? table_cap * 2 : 16;
        table = xrealloc(table, table_cap * sizeof(Macro));
    }
    table[table_len++] = m;
    return 0;
}

void macro_undef_at(const char *name)
{
    size_t n = ident_length(name);
    long i = n > 0 ? find_index(name, n) : -1;

    if (i >= 0)
        remove_index((size_t)i);
}

int macro_is_defined_at(const char *name)
{
    size_t n = ident_length(name);

    return n > 0 && find_index(name, n) >= 0;
}

void macro_free_all(void)
{
    size_t i;

    for (i = 0; i < table_len; i++)
        free_macro(&table[i]);
    free(table);
    table = NULL;
    table_len = 0;
    table_cap = 0;
}

static void free_args(char **args, int n)
{
    int i;

    for (i = 0; i < n; i++)
        free(args[i]);
    free(args);
}

static void push_arg(char ***args, int *n, const char *s, size_t len)
{
    *args = xrealloc(*args, sizeof(char *) * (size_t)(*n + 1));
    (*args)[(*n)++] = trimmed_copy(s, len);
}

static size_t collect_args(const char *s, char ***args_out, int *count_out)
{
    char **args = NULL;
    int count = 0;
    int depth = 1;
    size_t start = 1;
    size_t i = 1;

    while (s[i] != '\0') {
        char c = s[i];

        if (c == '"' || c == '\'') {
            i += literal_length(s + i);
            continue;
        }
        if (c == '(') {
            depth++;
        } else if (c == ')') {
            depth--;
            if (depth == 0) {
                push_arg(&args, &count, s + start, i - start);
                *args_out = args;
                *count_out = count;
                return i + 1;
            }
        } else if (c == ',' && depth == 1) {
            push_arg(&args, &count, s + start, i - start);
            start = i + 1;
        }
        i++;
    }
    free_args(args, count);
    return 0;
}

static int is_hidden(const char *name, const char **hide, int nhide)
{
    int i;

    for (i = 0; i < nhide; i++) {
        if (strcmp(hide[i], name) == 0)
            return 1;
    }
    return 0;
}

static void expand(const char *s, Buffer *out, const char **hide, int nhide);

static void expand_with(const char *text, const Macro *m, Buffer *out, const char **hide, int nhide)
{
    const char *next[MAX_HIDE];
    int i;

    if (nhide >= MAX_HIDE) {
        buf_puts(out, text);
        return;
    }
    for (i = 0; i < nhide; i++)
        next[i] = hide[i];
    next[nhide] = m->name;
    expand(text, out, next, nhide + 1);
}

static void substitute(const Macro *m, char **args, Buffer *out)
{
    const char *b = m->body;
    size_t i = 0;

    while (b[i] != '\0') {
        if (b[i] == '"' || b[i] == '\'') {
            size_t n = literal_length(b + i);

            buf_putn(out, b + i, n);
            i += n;
        } else if (isdigit((unsigned char)b[i])) {
            size_t n = number_length(b + i);

            buf_putn(out, b + i, n);
            i += n;
        } else if (ident_start(b[i])) {
            size_t n = ident_length(b + i);
            int k = param_index(m, b + i, n);

            if (k >= 0)
                buf_puts(out, args[k]);
            else
                buf_putn(out, b + i, n);
            i += n;
        } else {
            buf_putc(out, b[i]);
            i++;
        }
    }
}

static size_t invoke_function(const Macro *m, const char *s, Buffer *out, const char **hide, int nhide)
{
    char **args = NULL;
    int nargs = 0;
    int i;
    int valid;
    size_t used = collect_args(s, &args, &nargs);
    Buffer body;

    if (used == 0)
        return 0;

    valid = nargs == m->nparams || (m->nparams == 0 && nargs == 1 && args[0][0] == '\0');
    if (!valid) {
        fprintf(stderr, "mypreprocessor: macro %s expects %d argument(s) but got %d\n",
                m->name, m->nparams, nargs);
        free_args(args, nargs);
        return 0;
    }

    if (m->nparams > 0) {
        for (i = 0; i < nargs; i++) {
            Buffer e;

            buf_init(&e);
            expand(args[i], &e, hide, nhide);
            free(args[i]);
            args[i] = e.data;
        }
    }

    buf_init(&body);
    substitute(m, args, &body);
    expand_with(body.data, m, out, hide, nhide);
    buf_free(&body);
    free_args(args, nargs);
    return used;
}

static void expand(const char *s, Buffer *out, const char **hide, int nhide)
{
    size_t i = 0;

    while (s[i] != '\0') {
        char c = s[i];

        if (c == '"' || c == '\'') {
            size_t n = literal_length(s + i);

            buf_putn(out, s + i, n);
            i += n;
        } else if (isdigit((unsigned char)c)) {
            size_t n = number_length(s + i);

            buf_putn(out, s + i, n);
            i += n;
        } else if (ident_start(c)) {
            size_t n = ident_length(s + i);
            Macro *m = find_macro(s + i, n);

            if (m == NULL || is_hidden(m->name, hide, nhide)) {
                buf_putn(out, s + i, n);
                i += n;
            } else if (!m->is_func) {
                expand_with(m->body, m, out, hide, nhide);
                i += n;
            } else {
                size_t j = i + n;
                size_t used = 0;

                while (s[j] == ' ' || s[j] == '\t')
                    j++;
                if (s[j] == '(')
                    used = invoke_function(m, s + j, out, hide, nhide);

                if (used == 0) {
                    buf_putn(out, s + i, n);
                    i += n;
                } else {
                    i = j + used;
                }
            }
        } else {
            buf_putc(out, c);
            i++;
        }
    }
}

void macro_expand_line(const char *line, Buffer *out)
{
    expand(line, out, NULL, 0);
}
