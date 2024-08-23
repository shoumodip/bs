#ifndef BASIC_H
#define BASIC_H

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// String View
typedef struct {
    const char *data;
    size_t size;
} SV;

#define SVFmt "%.*s"
#define SVArg(s) (int)(s).size, (s).data
#define SVStatic(s) ((SV){s, sizeof(s) - 1})

bool sv_eq(SV a, SV b);

// Defer
#define return_defer(value)                                                                        \
    do {                                                                                           \
        result = (value);                                                                          \
        goto defer;                                                                                \
    } while (0)

// Dynamic Array
#define DA_INIT_CAP 128

#define da_free(l)                                                                                 \
    do {                                                                                           \
        free((l)->data);                                                                           \
        memset((l), 0, sizeof(*(l)));                                                              \
    } while (0)

#define da_push(l, v)                                                                              \
    do {                                                                                           \
        if ((l)->count >= (l)->capacity) {                                                         \
            (l)->capacity = (l)->capacity == 0 ? DA_INIT_CAP : (l)->capacity * 2;                  \
            (l)->data = realloc((l)->data, (l)->capacity * sizeof(*(l)->data));                    \
            assert((l)->data);                                                                     \
        }                                                                                          \
                                                                                                   \
        (l)->data[(l)->count] = (v);                                                               \
        (l)->count++;                                                                              \
    } while (0)

#define da_push_many(l, v, c)                                                                      \
    do {                                                                                           \
        if ((l)->count + (c) > (l)->capacity) {                                                    \
            if ((l)->capacity == 0) {                                                              \
                (l)->capacity = DA_INIT_CAP;                                                       \
            }                                                                                      \
                                                                                                   \
            while ((l)->count + (c) > (l)->capacity) {                                             \
                (l)->capacity *= 2;                                                                \
            }                                                                                      \
                                                                                                   \
            (l)->data = realloc((l)->data, (l)->capacity * sizeof(*(l)->data));                    \
            assert((l)->data);                                                                     \
        }                                                                                          \
                                                                                                   \
        if ((v)) {                                                                                 \
            memcpy((l)->data + (l)->count, (v), (c) * sizeof(*(l)->data));                         \
            (l)->count += (c);                                                                     \
        }                                                                                          \
    } while (0)

// File IO
char *read_file(const char *path, size_t *size);

// Arithmetic
#define max(a, b) ((a) > (b) ? (a) : (b))

#endif // BASIC_H
