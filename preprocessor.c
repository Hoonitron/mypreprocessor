#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffer.h"
#include "comment_removal.h"
#include "file_inclusion.h"
#include "macro_handler.h"
#include "preprocessor.h"

#define MAX_DEPTH 32
#define MAX_COND 64

typedef struct {
    int active;
    int taken;
    int passthrough;
} Cond;

typedef struct {
    Cond items[MAX_COND];
    int top;
} CondStack;

static int process_file(const char *path, FILE *out, int depth);

static void emit_line(FILE *out, const char *line)
{
    fputs(line, out);
    fputc('\n', out);
}

static const char *match_directive(const char *line, const char *word)
{
    size_t n = strlen(word);

    while (*line == ' ' || *line == '\t')
        line++;
    if (*line != '#')
        return NULL;
    line++;
    while (*line == ' ' || *line == '\t')
        line++;
    if (strncmp(line, word, n) != 0)
        return NULL;
    if (isalnum((unsigned char)line[n]) || line[n] == '_')
        return NULL;
    line += n;
    while (*line == ' ' || *line == '\t')
        line++;
    return line;
}

static int is_directive_line(const char *line)
{
    while (*line == ' ' || *line == '\t')
        line++;
    return *line == '#';
}

static int stack_active(const CondStack *s)
{
    return s->top == 0 ? 1 : s->items[s->top - 1].active;
}

static int stack_push(CondStack *s, int cond, int passthrough)
{
    int parent = stack_active(s);
    Cond *c;

    if (s->top >= MAX_COND)
        return -1;
    c = &s->items[s->top++];
    c->passthrough = passthrough;
    c->taken = cond;
    c->active = passthrough ? parent : (parent && cond);
    return 0;
}

static int handle_conditional(const char *line, CondStack *stack, FILE *out, const char *path)
{
    const char *rest;
    int negate = 0;
    Cond *top = stack->top > 0 ? &stack->items[stack->top - 1] : NULL;

    rest = match_directive(line, "ifdef");
    if (rest == NULL) {
        rest = match_directive(line, "ifndef");
        negate = 1;
    }
    if (rest != NULL) {
        int cond = macro_is_defined_at(rest);

        if (negate)
            cond = !cond;
        if (stack_push(stack, cond, 0) < 0) {
            fprintf(stderr, "mypreprocessor: %s: conditionals nested too deeply\n", path);
            return -1;
        }
        return 1;
    }

    if (match_directive(line, "if") != NULL) {
        if (stack_push(stack, 1, 1) < 0) {
            fprintf(stderr, "mypreprocessor: %s: conditionals nested too deeply\n", path);
            return -1;
        }
        if (stack_active(stack))
            emit_line(out, line);
        return 1;
    }

    if (match_directive(line, "elif") != NULL) {
        if (top == NULL) {
            fprintf(stderr, "mypreprocessor: %s: #elif without #if\n", path);
        } else if (top->passthrough) {
            if (top->active)
                emit_line(out, line);
        } else {
            fprintf(stderr, "mypreprocessor: %s: #elif is not supported after #ifdef/#ifndef\n", path);
            top->active = 0;
        }
        return 1;
    }

    if (match_directive(line, "else") != NULL) {
        if (top == NULL) {
            fprintf(stderr, "mypreprocessor: %s: #else without #if\n", path);
        } else if (top->passthrough) {
            if (top->active)
                emit_line(out, line);
        } else {
            int parent = stack->top > 1 ? stack->items[stack->top - 2].active : 1;

            top->active = parent && !top->taken;
            top->taken = 1;
        }
        return 1;
    }

    if (match_directive(line, "endif") != NULL) {
        if (top == NULL) {
            fprintf(stderr, "mypreprocessor: %s: #endif without #if\n", path);
            return 1;
        }
        if (top->passthrough && top->active)
            emit_line(out, line);
        stack->top--;
        return 1;
    }

    return 0;
}

static int do_include(const char *rest, const char *line, const char *path, FILE *out, int depth)
{
    char *name = include_target(rest);
    char *found;
    int status;

    if (name == NULL) {
        fprintf(stderr, "mypreprocessor: %s: malformed #include\n", path);
        emit_line(out, line);
        return 0;
    }

    found = find_include(name, path);
    if (found == NULL) {
        fprintf(stderr, "mypreprocessor: warning: %s not found, directive left as is\n", name);
        emit_line(out, line);
        free(name);
        return 0;
    }

    status = process_file(found, out, depth + 1);
    free(found);
    free(name);
    return status;
}

static int process_line(const char *line, const char *path, FILE *out, int depth, CondStack *stack)
{
    const char *rest;
    int handled = handle_conditional(line, stack, out, path);

    if (handled < 0)
        return -1;
    if (handled > 0)
        return 0;
    if (!stack_active(stack))
        return 0;

    rest = match_directive(line, "include");
    if (rest != NULL)
        return do_include(rest, line, path, out, depth);

    rest = match_directive(line, "define");
    if (rest != NULL) {
        if (macro_define(rest) < 0)
            fprintf(stderr, "mypreprocessor: %s: invalid #define ignored\n", path);
        return 0;
    }

    rest = match_directive(line, "undef");
    if (rest != NULL) {
        macro_undef_at(rest);
        return 0;
    }

    if (is_directive_line(line)) {
        emit_line(out, line);
        return 0;
    }

    {
        Buffer expanded;

        buf_init(&expanded);
        macro_expand_line(line, &expanded);
        emit_line(out, expanded.data);
        buf_free(&expanded);
    }
    return 0;
}

static const char *read_logical_line(const char *p, Buffer *line)
{
    int first = 1;
    int directive = 0;

    for (;;) {
        const char *end = strchr(p, '\n');
        size_t n = end != NULL ? (size_t)(end - p) : strlen(p);

        if (n > 0 && p[n - 1] == '\r')
            n--;

        if (first) {
            const char *q = p;

            while (*q == ' ' || *q == '\t')
                q++;
            directive = *q == '#';
            first = 0;
        }

        if (directive && n > 0 && p[n - 1] == '\\') {
            buf_putn(line, p, n - 1);
            if (end == NULL)
                return p + strlen(p);
            p = end + 1;
            if (*p == '\0')
                return p;
            continue;
        }

        buf_putn(line, p, n);
        return end != NULL ? end + 1 : p + strlen(p);
    }
}

static int process_file(const char *path, FILE *out, int depth)
{
    char *raw;
    char *clean;
    const char *p;
    CondStack stack;
    int status = 0;

    if (depth > MAX_DEPTH) {
        fprintf(stderr, "mypreprocessor: includes nested too deeply at %s\n", path);
        return -1;
    }

    raw = read_whole_file(path);
    if (raw == NULL) {
        fprintf(stderr, "mypreprocessor: cannot open %s\n", path);
        return -1;
    }

    clean = remove_comments(raw);
    free(raw);

    stack.top = 0;
    p = clean;
    while (*p != '\0') {
        Buffer line;

        buf_init(&line);
        p = read_logical_line(p, &line);
        if (process_line(line.data, path, out, depth, &stack) < 0)
            status = -1;
        buf_free(&line);
    }

    if (stack.top != 0)
        fprintf(stderr, "mypreprocessor: warning: unterminated conditional in %s\n", path);

    free(clean);
    return status;
}

int preprocess_file(const char *path, FILE *out)
{
    return process_file(path, out, 0);
}
