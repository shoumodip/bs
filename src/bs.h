#ifndef BS_H
#define BS_H

#include "lexer.h"
#include "value.h"

bool object_str_eq(ObjectStr *a, ObjectStr *b);

// Compiler
typedef struct {
    Token token;
    size_t depth;
    bool captured;
} Local;

typedef struct {
    bool local;
    size_t index;
} Upvalue;

typedef struct {
    Upvalue *data;
    size_t count;
    size_t capacity;
} Upvalues;

#define upvalues_free da_free
#define upvalues_push da_push

typedef struct Scope Scope;

struct Scope {
    Scope *outer;

    Local *data;
    size_t count;
    size_t depth;
    size_t capacity;

    ObjectFn *fn;
    Upvalues upvalues;
};

#define scope_push da_push

typedef struct {
    Lexer lexer;
    Chunk *chunk;
    Scope *scope;
} Compiler;

// Memory
typedef struct {
    const uint8_t *ip;
    ObjectClosure *closure;
    size_t base;
} Frame;

typedef struct {
    Frame *data;
    size_t count;
    size_t capacity;
} Frames;

#define frames_free da_free
#define frames_push da_push

typedef struct {
    Values stack;

    Frame *frame;
    Frames frames;

    Table globals;
    ObjectUpvalue *upvalues;

    Object *objects;
    size_t gc;
    size_t allocated;
} Memory;

// Bs
typedef struct {
    Memory memory;
    Compiler compiler;
} Bs;

void bs_free(Bs *bs);
void *bs_realloc(Bs *bs, void *previous, size_t old_size, size_t new_size);

bool array_get(ObjectArray *array, size_t index, Value *value);
void array_set(ObjectArray *array, Bs *bs, size_t index, Value value);

void table_free(Table *table, Bs *bs);
bool table_remove(Table *table, ObjectStr *key);

bool table_get(Table *table, ObjectStr *key, Value *value);
bool table_set(Table *table, Bs *bs, ObjectStr *key, Value value);

ObjectFn *bs_new_object_fn(Bs *bs);
ObjectStr *bs_new_object_str(Bs *bs, const char *data, size_t size);
ObjectArray *bs_new_object_array(Bs *bs);
ObjectUpvalue *bs_new_object_upvalue(Bs *bs, size_t index);
ObjectClosure *bs_new_object_closure(Bs *bs, const ObjectFn *fn);

void bs_trace(Bs *bs);
bool bs_interpret(Bs *bs, const ObjectFn *fn, bool step);

ObjectFn *bs_compile(Bs *bs);

#endif // BS_H
