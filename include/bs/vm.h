#ifndef BS_VM_H
#define BS_VM_H

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

Bs_Str *bs_str_new(Bs *bs, Bs_Sv sv);
Bs_Object *bs_object_new(Bs *bs, Bs_Object_Type type, size_t size);

bool bs_update_cwd(Bs *bs);
void bs_global_set(Bs *bs, Bs_Sv name, Bs_Value value);

void bs_value_write(Bs *bs, Bs_Writer *writer, Bs_Value value);

// Buffer
typedef struct {
    Bs *bs;
    char *data;
    size_t count;
    size_t capacity;
} Bs_Buffer;

Bs_Writer bs_buffer_writer(Bs_Buffer *buffer);

Bs_Sv bs_buffer_reset(Bs_Buffer *buffer, size_t pos);
Bs_Sv bs_buffer_absolute_path(Bs_Buffer *buffer, Bs_Sv path);
Bs_Sv bs_buffer_relative_path(Bs_Buffer *buffer, Bs_Sv path);

// Context
typedef struct {
    void *userdata;

    Bs_Writer log;
    Bs_Writer error;
    Bs_Buffer buffer;
} Bs_Config;

Bs_Config *bs_config(Bs *bs);

// FFI
typedef Bs_Value (*Bs_C_Fn_Ptr)(Bs *bs, Bs_Value *args, size_t arity);

typedef struct {
    const char *name;
    Bs_C_Fn_Ptr ptr;
} Bs_FFI;

// Errors
void bs_unwind(Bs *bs, unsigned char exit);

void bs_error_at(Bs *bs, size_t location, const char *fmt, ...)
    __attribute__((__format__(__printf__, 3, 4)));

void bs_check_arity_at(Bs *bs, size_t location, size_t actual, size_t expected);
void bs_check_callable_at(Bs *bs, size_t location, Bs_Value value, const char *label);

void bs_check_value_type_at(
    Bs *bs, size_t location, Bs_Value value, Bs_Value_Type expected, const char *label);
void bs_check_object_type_at(
    Bs *bs, size_t location, Bs_Value value, Bs_Object_Type expected, const char *label);

void bs_check_integer_at(Bs *bs, size_t location, Bs_Value value, const char *label);
void bs_check_whole_number_at(Bs *bs, size_t location, Bs_Value value, const char *label);

#define bs_error(bs, ...) bs_error_at(bs, 0, __VA_ARGS__)
#define bs_check_arity(bs, actual, expected) bs_check_arity_at(bs, 0, actual, expected)
#define bs_check_callable(bs, value, label) bs_check_callable_at(bs, 0, value, label)

#define bs_check_value_type(bs, value, expected, label)                                            \
    bs_check_value_type_at(bs, 0, value, expected, label)
#define bs_check_object_type(bs, value, expected, label)                                           \
    bs_check_object_type_at(bs, 0, value, expected, label)
#define bs_check_object_c_type(bs, value, expected, label)                                         \
    bs_check_object_c_type_at(bs, 0, value, expected, label)

#define bs_check_integer(bs, value, label) bs_check_integer_at(bs, 0, value, label)
#define bs_check_whole_number(bs, value, lable) bs_check_whole_number_at(bs, 0, value, label)

#define bs_arg_check_callable(bs, args, index)                                                     \
    bs_check_callable_at(bs, (index) + 1, (args)[index], NULL)

#define bs_arg_check_value_type(bs, args, index, expected)                                         \
    bs_check_value_type_at(bs, (index) + 1, (args)[index], expected, NULL)

#define bs_arg_check_object_type(bs, args, index, expected)                                        \
    bs_check_object_type_at(bs, (index) + 1, (args)[index], expected, NULL)

#define bs_arg_check_object_c_type(bs, args, index, expected)                                      \
    bs_check_object_c_type_at(bs, (index) + 1, (args)[index], expected, NULL)

#define bs_arg_check_integer(bs, args, index)                                                      \
    bs_check_integer_at(bs, (index) + 1, (args)[index], NULL)

#define bs_arg_check_whole_number(bs, args, index)                                                 \
    bs_check_whole_number_at(bs, (index) + 1, (args)[index], NULL)

// Interpreter
typedef struct {
    bool ok;
    int exit;
    Bs_Value value;
} Bs_Result;

const Bs_Fn *bs_compile(Bs *bs, Bs_Sv path, Bs_Sv input, bool is_main);

Bs_Result bs_run(Bs *bs, Bs_Sv path, Bs_Sv input);
Bs_Value bs_call(Bs *bs, Bs_Value fn, const Bs_Value *args, size_t arity);

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

#endif // BS_VM_H
