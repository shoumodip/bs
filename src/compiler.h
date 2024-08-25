#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"
#include "object.h"

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
    Scope *scope;

    Vm *vm;
    Chunk *chunk;
} Compiler;

ObjectFn *compile(Compiler *compiler);

#endif // COMPILER_H
