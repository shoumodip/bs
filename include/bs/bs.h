#ifndef BS_H
#define BS_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "value.h"

// Interface
typedef struct Bs Bs;

Bs *bs_new(void);
void bs_free(Bs *bs);
void *bs_realloc(Bs *bs, void *ptr, size_t old_size, size_t new_size);

void bs_exit_set(Bs *bs, int code);

Bs_Writer *bs_str_writer_init(Bs *bs, size_t *start);
Bs_Sv bs_str_writer_end(Bs *bs, size_t start);

void bs_str_writer_null(Bs *bs);
size_t bs_str_writer_pos(Bs *bs);
const char *bs_str_writer_get(Bs *bs, size_t pos);

Bs_Writer *bs_stdout_writer(Bs *bs);
Bs_Writer *bs_stderr_writer(Bs *bs);

Bs_Object *bs_object_new(Bs *bs, Bs_Object_Type type, size_t size);
Bs_Str *bs_str_const(Bs *bs, Bs_Sv sv);
void bs_global_set(Bs *bs, Bs_Sv name, Bs_Value value);

// FFI
typedef bool (*Bs_C_Fn_Ptr)(Bs *bs, Bs_Value *args, size_t arity, Bs_Value *result);

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

#endif // BS_H
