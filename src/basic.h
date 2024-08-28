#ifndef BASIC_H
#define BASIC_H

#include <stdbool.h>
#include <stddef.h>

// String View
typedef struct {
    const char *data;
    size_t size;
} SV;

#define SVFmt "%.*s"
#define SVArg(s) (int)(s).size, (s).data
#define SVStatic(s) ((SV){s, sizeof(s) - 1})

bool sv_eq(SV a, SV b);
bool sv_suffix(SV a, SV b);

// Writer
typedef struct Writer Writer;

struct Writer {
    void (*fmt)(Writer *writer, const char *fmt, ...);
};

// Defer
#define return_defer(value)                                                                        \
    do {                                                                                           \
        result = (value);                                                                          \
        goto defer;                                                                                \
    } while (0)

// File IO
char *read_file(const char *path, size_t *size);

// Arithmetic
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#endif // BASIC_H
