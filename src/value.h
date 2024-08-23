#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>
#include <stdio.h>

#include "basic.h"
#include "op.h"

typedef enum {
    VALUE_NIL,
    VALUE_NUM,
    VALUE_BOOL,
    VALUE_OBJECT
} ValueType;

const char *value_type_name(ValueType type);

typedef enum {
    OBJECT_FN,
    OBJECT_STR,
    OBJECT_UPVALUE,
    OBJECT_CLOSURE,
    COUNT_OBJECTS
} ObjectType;

typedef struct Object Object;

struct Object {
    ObjectType type;
    Object *next;
};

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Object *object;
    } as;
} Value;

#define value_nil ((Value){VALUE_NIL})
#define value_num(v) ((Value){VALUE_NUM, .as.number = (v)})
#define value_bool(v) ((Value){VALUE_BOOL, .as.boolean = (v)})
#define value_object(v) ((Value){VALUE_OBJECT, .as.object = (Object *)(v)})

bool value_is_falsey(Value value);
void value_print(Value value, FILE *file);
bool value_equal(Value a, Value b);

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

void chunk_free(Chunk *chunk);
void chunk_push_op(Chunk *chunk, Op op);
void chunk_push_op_int(Chunk *chunk, Op op, size_t value);
void chunk_push_op_value(Chunk *chunk, Op op, Value value);

typedef struct {
    Object meta;
    size_t size;
    char data[];
} ObjectStr;

bool object_str_eq(ObjectStr *a, ObjectStr *b);

typedef struct {
    Object meta;
    Chunk chunk;
    size_t arity;
    size_t upvalues;
    ObjectStr *name;
} ObjectFn;

typedef struct ObjectUpvalue ObjectUpvalue;

struct ObjectUpvalue {
    Object meta;
    bool closed;
    union {
        size_t index;
        Value value;
    };
    ObjectUpvalue *next;
};

typedef struct {
    Object meta;
    const ObjectFn *fn;

    size_t upvalues;
    ObjectUpvalue *data[];
} ObjectClosure;

typedef struct {
    ObjectStr *key;
    Value value;
} Entry;

#define TABLE_MAX_LOAD 0.75

typedef struct {
    Entry *data;
    size_t count;
    size_t capacity;
} Table;

typedef struct {
    Object *objects;
} GC;

void *gc_realloc(GC *gc, void *previous, size_t old_size, size_t new_size);

void table_free(Table *table, GC *gc);
bool table_remove(Table *table, ObjectStr *key);

bool table_get(Table *table, ObjectStr *key, Value *value);
bool table_set(Table *table, GC *gc, ObjectStr *key, Value value);

ObjectFn *gc_new_object_fn(GC *gc);
ObjectStr *gc_new_object_str(GC *gc, const char *data, size_t size);
ObjectUpvalue *gc_new_object_upvalue(GC *gc, size_t index);
ObjectClosure *gc_new_object_closure(GC *gc, const ObjectFn *fn);

#endif // VALUE_H
