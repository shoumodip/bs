#ifndef COMPILER_H
#define COMPILER_H

#include "chunk.h"
#include "lexer.h"

typedef struct {
    Lexer lexer;
    bool error;

    Chunk *chunk;
} Compiler;

bool compile(Compiler *compiler, Chunk *chunk);

#endif // COMPILER_H
