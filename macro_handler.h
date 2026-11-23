#ifndef MACRO_HANDLER_H
#define MACRO_HANDLER_H
#include"buffer.h"
int isidstart(char c);
int isidchar(char c);
int adddefine(char *def);
void undefmacro(char *name);
int ismacro(char *name);
void expandline(char *line,BUF *out);
void freemacros();
#endif
