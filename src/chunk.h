#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>

#include "value.h"

typedef enum {
    OP_HALT,
    OP_DROP,
    OP_DROPS,

    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_CONST,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NEG,
    OP_NOT,

    OP_GDEF,
    OP_GGET,
    OP_GSET,
    OP_LGET,
    OP_LSET,

    OP_PRINT,
    COUNT_OPS,
} Op;

typedef struct {
    uint8_t *data;
    size_t last;
    size_t count;
    size_t capacity;

    Values constants;
} Chunk;

void chunk_free(Chunk *chunk);
void chunk_print(Chunk *chunk);

void chunk_push_op(Chunk *chunk, Op op);
void chunk_push_int(Chunk *chunk, Op op, size_t value);
void chunk_push_value(Chunk *chunk, Op op, Value value);

#endif // CHUNK_H
