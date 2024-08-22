#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"
#include "value.h"

typedef struct {
    Token token;
    size_t depth;
} Local;

typedef struct Scope Scope;

struct Scope {
    Scope *outer;

    Local *data;
    size_t count;
    size_t depth;
    size_t capacity;

    ObjectFn *fn;
};

#define scope_free da_free
#define scope_push da_push

typedef struct {
    Lexer lexer;
    bool error;

    GC *gc;

    Token current;
    Token previous;

    Scope *scope;
} Compiler;

ObjectFn *compile(Compiler *compiler);

#endif // COMPILER_H
