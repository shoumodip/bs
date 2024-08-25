#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>

#include "op.h"
#include "vm.h"

typedef struct {
    Value *data;
    size_t count;
    size_t capacity;
} Values;

#define values_push da_push
#define values_free da_free

typedef struct {
    uint8_t *data;
    size_t last;
    size_t count;
    size_t capacity;

    Values constants;
} Chunk;

void chunk_free(Vm *vm, Chunk *chunk);
void chunk_push_op(Vm *vm, Chunk *chunk, Op op);
void chunk_push_op_int(Vm *vm, Chunk *chunk, Op op, size_t value);
void chunk_push_op_value(Vm *vm, Chunk *chunk, Op op, Value value);

struct Object {
    ObjectType type;
    Object *next;
    bool marked;
};

// The duplicate type definition is to appease the LSP unused include warnings
typedef struct Object Object;

struct ObjectFn {
    Object meta;

    Chunk chunk;
    ObjectStr *name;

    size_t arity;
    size_t upvalues;
};

ObjectFn *object_fn_new(Vm *vm);

struct ObjectStr {
    Object meta;
    size_t size;
    char data[];
};

ObjectStr *object_str_new(Vm *vm, const char *data, size_t size);
bool object_str_eq(ObjectStr *a, ObjectStr *b);

struct ObjectArray {
    Object meta;
    Value *data;
    size_t count;
    size_t capacity;
};

ObjectArray *object_array_new(Vm *vm);
bool object_array_get(Vm *vm, ObjectArray *array, size_t index, Value *value);
void object_array_set(Vm *vm, ObjectArray *array, size_t index, Value value);

typedef struct {
    ObjectStr *key;
    Value value;
} Entry;

typedef struct {
    Entry *data;
    size_t count;
    size_t capacity;
} Table;

void table_free(Vm *vm, Table *table);
bool table_remove(Vm *vm, Table *table, ObjectStr *key);

bool table_get(Vm *vm, Table *table, ObjectStr *key, Value *value);
bool table_set(Vm *vm, Table *table, ObjectStr *key, Value value);

struct ObjectClosure {
    Object meta;
    const ObjectFn *fn;

    size_t upvalues;
    ObjectUpvalue *data[];
};

ObjectClosure *object_closure_new(Vm *vm, const ObjectFn *fn);

struct ObjectUpvalue {
    Object meta;
    bool closed;
    union {
        size_t index;
        Value value;
    };
    ObjectUpvalue *next;
};

ObjectUpvalue *object_upvalue_new(Vm *vm, size_t index);

#endif // OBJECT_H
