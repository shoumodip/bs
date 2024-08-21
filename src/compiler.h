#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"
#include "value.h"

typedef struct {
    Token token;
    size_t depth;
} Local;

typedef struct {
    Local *data;
    size_t count;
    size_t depth;
    size_t capacity;
} Scope;

typedef struct {
    Lexer lexer;
    bool error;

    GC *gc;
    Chunk *chunk;

    Token current;
    Token previous;

    Scope *scope;
} Compiler;

bool compile(Compiler *compiler, Chunk *chunk);

#endif // COMPILER_H
