#include <stdio.h>

#include "basic.h"

char *read_file(const char *path, size_t *size) {
    char *result = NULL;

    FILE *f = fopen(path, "r");
    if (!f) {
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) == -1) {
        return_defer(NULL);
    }

    const long offset = ftell(f);
    if (offset == -1) {
        return_defer(NULL);
    }

    if (fseek(f, 0, SEEK_SET) == -1) {
        return_defer(NULL);
    }

    result = malloc(offset + 1);
    if (!result) {
        return_defer(NULL);
    }

    fread(result, offset, 1, f);
    if (ferror(f)) {
        free(result);
        return_defer(NULL);
    }
    result[offset] = '\0';

    *size = offset;

defer:
    fclose(f);
    return result;
}
