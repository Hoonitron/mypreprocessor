#ifndef FILE_INCLUSION_H
#define FILE_INCLUSION_H
int exists(char *path);
char *readfile(char *path);
char *getname(char *rest);
char *findfile(char *name,char *current);
#endif
