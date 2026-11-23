#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffer.h"
#include "macro_handler.h"
#include "preprocessor.h"

static char *make_output_name(const char *input)
{
    const char *slash = strrchr(input, '/');
    const char *base = slash != NULL ? slash + 1 : input;
    const char *dot = strrchr(base, '.');
    size_t stem = (dot != NULL && dot != base) ? (size_t)(dot - input) : strlen(input);
    char *name = xmalloc(stem + 3);

    memcpy(name, input, stem);
    strcpy(name + stem, ".i");
    return name;
}

int main(int argc, char *argv[])
{
    char *output_name;
    FILE *out;
    int status;

    if (argc != 2) {
        fprintf(stderr, "usage: %s inputfile.c\n", argv[0]);
        return 1;
    }

    output_name = make_output_name(argv[1]);
    if (strcmp(output_name, argv[1]) == 0) {
        fprintf(stderr, "mypreprocessor: output file would overwrite the input file\n");
        free(output_name);
        return 1;
    }

    out = fopen(output_name, "w");
    if (out == NULL) {
        perror(output_name);
        free(output_name);
        return 1;
    }

    status = preprocess_file(argv[1], out);
    if (fclose(out) != 0)
        status = -1;
    if (status != 0)
        remove(output_name);

    macro_free_all();
    free(output_name);
    return status == 0 ? 0 : 1;
}
