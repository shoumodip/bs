#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "basic.h"

bool sv_eq(SV a, SV b) {
    return a.size == b.size && !memcmp(a.data, b.data, b.size);
}

bool sv_suffix(SV a, SV b) {
    return a.size >= b.size && !memcmp(a.data + a.size - b.size, b.data, b.size);
}

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
