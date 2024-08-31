#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "basic.h"

Bs_Sv bs_sv_from_cstr(const char *data) {
    return (Bs_Sv){data, strlen(data)};
}

Bs_Sv bs_sv_from_parts(const char *data, size_t size) {
    return (Bs_Sv){data, size};
}

bool bs_sv_eq(Bs_Sv a, Bs_Sv b) {
    return a.size == b.size && !memcmp(a.data, b.data, b.size);
}

bool bs_sv_suffix(Bs_Sv a, Bs_Sv b) {
    return a.size >= b.size && !memcmp(a.data + a.size - b.size, b.data, b.size);
}

void bs_write(Bs_Writer *w, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    w->fmt(w, fmt, args);
    va_end(args);
}

char *bs_read_file(const char *path, size_t *size) {
    char *result = NULL;

    FILE *f = fopen(path, "r");
    if (!f) {
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) == -1) {
        bs_return_defer(NULL);
    }

    const long offset = ftell(f);
    if (offset == -1) {
        bs_return_defer(NULL);
    }

    if (fseek(f, 0, SEEK_SET) == -1) {
        bs_return_defer(NULL);
    }

    result = malloc(offset + 1);
    if (!result) {
        bs_return_defer(NULL);
    }

    fread(result, offset, 1, f);
    if (ferror(f)) {
        free(result);
        bs_return_defer(NULL);
    }
    result[offset] = '\0';

    *size = offset;

defer:
    fclose(f);
    return result;
}
