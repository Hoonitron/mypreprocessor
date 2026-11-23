#ifndef FILE_INCLUSION_H
#define FILE_INCLUSION_H

char *read_whole_file(const char *path);
char *include_target(const char *rest);
char *find_include(const char *name, const char *current_file);

#endif
