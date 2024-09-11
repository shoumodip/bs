#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bs/basic.h"

Bs_Sv bs_sv_from_cstr(const char *data) {
    return (Bs_Sv){data, strlen(data)};
}

Bs_Sv bs_sv_drop(Bs_Sv *s, size_t count) {
    const Bs_Sv result = Bs_Sv(s->data, count);
    s->data += count;
    s->size -= count;
    return result;
}

bool bs_sv_eq(Bs_Sv a, Bs_Sv b) {
    return a.size == b.size && !memcmp(a.data, b.data, b.size);
}

bool bs_sv_find(Bs_Sv s, char ch, size_t *index) {
    const char *p = memchr(s.data, ch, s.size);
    if (!p) {
        return false;
    }

    *index = p - s.data;
    return true;
}

bool bs_sv_prefix(Bs_Sv a, Bs_Sv b) {
    return a.size >= b.size && !memcmp(a.data, b.data, b.size);
}

bool bs_sv_suffix(Bs_Sv a, Bs_Sv b) {
    return a.size >= b.size && !memcmp(a.data + a.size - b.size, b.data, b.size);
}

void bs_fmt(Bs_Writer *w, const char *fmt, ...) {
    static char bs_fmt_buffer[1024];

    va_list args;
    va_start(args, fmt);
    const int count = vsnprintf(bs_fmt_buffer, sizeof(bs_fmt_buffer), fmt, args);
    assert(count >= 0 && count + 1 < sizeof(bs_fmt_buffer));
    va_end(args);

    w->write(w, Bs_Sv(bs_fmt_buffer, count));
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
