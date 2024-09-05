#ifndef BS_H
#define BS_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "value.h"

// Interface
typedef struct Bs Bs;

Bs *bs_new(void);
void bs_free(Bs *bs);
void *bs_realloc(Bs *bs, void *ptr, size_t old_size, size_t new_size);

// Helpers
typedef struct Bs_Buffer Bs_Buffer;

void bs_file_write(Bs_Writer *w, Bs_Sv sv);
void bs_buffer_write(Bs_Writer *writer, Bs_Sv sv);
Bs_Sv bs_buffer_reset(Bs_Buffer *buffer, size_t pos);

Bs_Writer bs_file_writer(FILE *file);
Bs_Writer bs_buffer_writer(Bs_Buffer *buffer);
Bs_Buffer *bs_buffer_get(Bs *bs);

Bs_Writer *bs_stdout_get(Bs *bs);
void bs_stdout_set(Bs *bs, Bs_Writer writer);

Bs_Writer *bs_stderr_get(Bs *bs);
void bs_stderr_set(Bs *bs, Bs_Writer writer);

void *bs_userdata_get(Bs *bs);
void bs_userdata_set(Bs *bs, void *data);

Bs_Object *bs_object_new(Bs *bs, Bs_Object_Type type, size_t size);
Bs_Str *bs_str_const(Bs *bs, Bs_Sv sv);
void bs_global_set(Bs *bs, Bs_Sv name, Bs_Value value);

// FFI
typedef Bs_Value (*Bs_C_Fn_Ptr)(Bs *bs, Bs_Value *args, size_t arity);

typedef struct {
    const char *name;
    Bs_C_Fn_Ptr fn;
} Bs_Export;

typedef struct {
    Bs_Sv name;
    void (*free)(Bs *bs, void *data);
    void (*write)(Bs_Writer *writer, const void *data);
} Bs_C_Data_Spec;

// Errors
void bs_error(Bs *bs, const char *fmt, ...);
bool bs_check_arity(Bs *bs, size_t actual, size_t expected);
bool bs_check_value_type(Bs *bs, Bs_Value value, Bs_Value_Type expected, const char *label);
bool bs_check_object_type(Bs *bs, Bs_Value value, Bs_Object_Type expected, const char *label);
bool bs_check_object_c_type(Bs *bs, Bs_Value value, const Bs_C_Data_Spec *spec, const char *label);
bool bs_check_whole_number(Bs *bs, Bs_Value value, const char *label);

// Interpreter
int bs_run(Bs *bs, const char *path, Bs_Sv input, bool step);

// Dynamic Array
#define BS_DA_INIT_CAP 128

#define bs_da_free(bs, l)                                                                          \
    do {                                                                                           \
        bs_realloc((bs), (l)->data, (l)->capacity * sizeof(*(l)->data), 0);                        \
        memset((l), 0, sizeof(*(l)));                                                              \
    } while (0)

#define bs_da_push(bs, l, v)                                                                       \
    do {                                                                                           \
        if ((l)->count >= (l)->capacity) {                                                         \
            const size_t new_capacity = (l)->capacity == 0 ? BS_DA_INIT_CAP : (l)->capacity * 2;   \
                                                                                                   \
            (l)->data = bs_realloc(                                                                \
                (bs),                                                                              \
                (l)->data,                                                                         \
                (l)->capacity * sizeof(*(l)->data),                                                \
                new_capacity * sizeof(*(l)->data));                                                \
                                                                                                   \
            assert((l)->data);                                                                     \
            (l)->capacity = new_capacity;                                                          \
        }                                                                                          \
                                                                                                   \
        (l)->data[(l)->count] = (v);                                                               \
        (l)->count++;                                                                              \
    } while (0)

#define bs_da_push_many(bs, l, v, c)                                                               \
    do {                                                                                           \
        if ((l)->count + (c) > (l)->capacity) {                                                    \
            size_t new_capacity = (l)->capacity ? (l)->capacity : BS_DA_INIT_CAP;                  \
            while ((l)->count + (c) > new_capacity) {                                              \
                new_capacity *= 2;                                                                 \
            }                                                                                      \
                                                                                                   \
            (l)->data = bs_realloc(                                                                \
                (bs),                                                                              \
                (l)->data,                                                                         \
                (l)->capacity * sizeof(*(l)->data),                                                \
                new_capacity * sizeof(*(l)->data));                                                \
                                                                                                   \
            assert((l)->data);                                                                     \
            (l)->capacity = new_capacity;                                                          \
        }                                                                                          \
                                                                                                   \
        if ((v)) {                                                                                 \
            memcpy((l)->data + (l)->count, (v), (c) * sizeof(*(l)->data));                         \
            (l)->count += (c);                                                                     \
        }                                                                                          \
    } while (0)

// Buffer
struct Bs_Buffer {
    Bs *bs;
    char *data;
    size_t count;
    size_t capacity;
};

#define bs_buffer_free bs_da_free
#define bs_buffer_push bs_da_push

#endif // BS_H
