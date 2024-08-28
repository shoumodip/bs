#ifndef VM_H
#define VM_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "value.h"

typedef struct Vm Vm;

typedef struct {
    SV name;
    void (*free)(Vm *vm, void *data);
    void (*write)(Writer *writer, const void *data);
} NativeSpec;

Vm *vm_new(void);
void vm_free(Vm *vm);
void *vm_realloc(Vm *vm, void *ptr, size_t old_size, size_t new_size);

void vm_error(Vm *vm, const char *fmt, ...);

bool vm_check_value_type(Vm *vm, Value value, ValueType expected, const char *label);
bool vm_check_object_type(Vm *vm, Value value, ObjectType expected, const char *label);
bool vm_check_object_native_type(Vm *vm, Value value, const NativeSpec *spec, const char *label);
bool vm_check_whole_number(Vm *vm, Value value, const char *label);

bool vm_interpret(Vm *vm, const ObjectFn *fn, bool step);

size_t vm_modules_push(Vm *vm, ObjectStr *name);
void vm_native_define(Vm *vm, SV name, Value value);

Writer *vm_writer_str_begin(Vm *vm, size_t *start);
SV vm_writer_str_end(Vm *vm, size_t start);

Object *object_new(Vm *vm, ObjectType type, size_t size);
ObjectStr *object_str_const(Vm *vm, const char *data, size_t size);

// Dynamic Array
#define DA_INIT_CAP 128

#define da_free(vm, l)                                                                             \
    do {                                                                                           \
        vm_realloc((vm), (l)->data, (l)->capacity * sizeof(*(l)->data), 0);                        \
        memset((l), 0, sizeof(*(l)));                                                              \
    } while (0)

#define da_push(vm, l, v)                                                                          \
    do {                                                                                           \
        if ((l)->count >= (l)->capacity) {                                                         \
            const size_t new_capacity = (l)->capacity == 0 ? DA_INIT_CAP : (l)->capacity * 2;      \
                                                                                                   \
            (l)->data = vm_realloc(                                                                \
                (vm),                                                                              \
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

#define da_push_many(vm, l, v, c)                                                                  \
    do {                                                                                           \
        if ((l)->count + (c) > (l)->capacity) {                                                    \
            size_t new_capacity = (l)->capacity ? (l)->capacity : DA_INIT_CAP;                     \
            while ((l)->count + (c) > new_capacity) {                                              \
                new_capacity *= 2;                                                                 \
            }                                                                                      \
                                                                                                   \
            (l)->data = vm_realloc(                                                                \
                (vm),                                                                              \
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

#endif // VM_H
