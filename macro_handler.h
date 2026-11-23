#ifndef MACRO_HANDLER_H
#define MACRO_HANDLER_H

#include "buffer.h"

int macro_define(const char *definition);
void macro_undef_at(const char *name);
int macro_is_defined_at(const char *name);
void macro_expand_line(const char *line, Buffer *out);
void macro_free_all(void);

#endif
