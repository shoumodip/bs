#ifndef BS_BASIC_H
#define BS_BASIC_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

// String View
typedef struct {
    const char *data;
    size_t size;
} Bs_Sv;

#define Bs_Sv(s, c) ((Bs_Sv){s, c})
#define Bs_Sv_Fmt "%.*s"
#define Bs_Sv_Arg(s) (int)(s).size, (s).data
#define Bs_Sv_Static(s) ((Bs_Sv){s, sizeof(s) - 1})

Bs_Sv bs_sv_from_cstr(const char *data);
Bs_Sv bs_sv_drop(Bs_Sv *s, size_t count);
Bs_Sv bs_sv_split(Bs_Sv *s, char ch);

bool bs_sv_eq(Bs_Sv a, Bs_Sv b);
bool bs_sv_find(Bs_Sv s, char ch, size_t *index);
bool bs_sv_prefix(Bs_Sv a, Bs_Sv b);
bool bs_sv_suffix(Bs_Sv a, Bs_Sv b);

// Writer
typedef struct Bs_Writer Bs_Writer;

struct Bs_Writer {
    void *data;
    void (*write)(Bs_Writer *writer, Bs_Sv sv);
};

Bs_Writer bs_file_writer(FILE *file);

void bs_fmt(Bs_Writer *writer, const char *fmt, ...) __attribute__((__format__(__printf__, 2, 3)));

// Defer
#define bs_return_defer(value)                                                                     \
    do {                                                                                           \
        result = (value);                                                                          \
        goto defer;                                                                                \
    } while (0)

// File IO
char *bs_read_file(const char *path, size_t *size);

// Arithmetic
#define bs_min(a, b) ((a) < (b) ? (a) : (b))
#define bs_max(a, b) ((a) > (b) ? (a) : (b))

#define bs_c_array_size(a) (sizeof(a) / sizeof(*a))

#endif // BS_BASIC_H
