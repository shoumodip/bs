#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>

#include "op.h"
#include "token.h"
#include "vm.h"

typedef struct {
    Value *data;
    size_t count;
    size_t capacity;
} Values;

#define values_free da_free
#define values_push da_push

typedef struct {
    Loc loc;
    size_t index;
} OpLoc;

typedef struct {
    OpLoc *data;
    size_t count;
    size_t capacity;
} OpLocs;

#define op_locs_free da_free
#define op_locs_push da_push

typedef struct {
    uint8_t *data;
    size_t last;
    size_t count;
    size_t capacity;

    Values constants;
    OpLocs locations;
} Chunk;

void chunk_free(Vm *vm, Chunk *chunk);
void chunk_push_op(Vm *vm, Chunk *chunk, Op op);
void chunk_push_op_loc(Vm *vm, Chunk *c, Loc loc);
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
    size_t module;
    ObjectStr *name;

    size_t arity;
    size_t upvalues;
};

ObjectFn *object_fn_new(Vm *vm);

struct ObjectStr {
    Object meta;

    bool hashed;
    uint32_t hash;

    size_t size;
    char data[];
};

ObjectStr *object_str_new(Vm *vm, const char *data, size_t size);
bool object_str_eq(const ObjectStr *a, const ObjectStr *b);

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

struct ObjectTable {
    Object meta;
    Entry *data;
    size_t count;
    size_t capacity;
    size_t real_count;
};

ObjectTable *object_table_new(Vm *vm);

void object_table_free(Vm *vm, ObjectTable *table);
bool object_table_remove(Vm *vm, ObjectTable *table, ObjectStr *key);

bool object_table_get(Vm *vm, ObjectTable *table, ObjectStr *key, Value *value);
bool object_table_set(Vm *vm, ObjectTable *table, ObjectStr *key, Value value);

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
