#ifndef COMPILER_H
#define COMPILER_H

#include "chunk.h"
#include "lexer.h"

typedef struct {
    Lexer lexer;
    bool error;

    GC *gc;
    Chunk *chunk;
    size_t last_op;

    Token current;
    Token previous;
} Compiler;

bool compile(Compiler *compiler, Chunk *chunk);

#endif // COMPILER_H
