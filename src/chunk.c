#include <stdio.h>

#include "chunk.h"

static_assert(COUNT_OPS == 28, "Update op_get_to_set()");
Op op_get_to_set(Op op) {
    switch (op) {
    case OP_GGET:
        return OP_GSET;

    case OP_LGET:
        return OP_LSET;

    default:
        return OP_HALT;
    }
}

void chunk_free(Chunk *chunk) {
    values_free(&chunk->constants);
    da_free(chunk);
}

void chunk_push_op(Chunk *chunk, Op op) {
    chunk->last = chunk->count;
    da_push(chunk, op);
}

void chunk_push_int(Chunk *chunk, Op op, size_t value) {
    const size_t bytes = sizeof(value);
    chunk_push_op(chunk, op);
    da_push_many(chunk, &value, bytes);
}

void chunk_push_value(Chunk *chunk, Op op, Value value) {
    const size_t index = chunk->constants.count;
    const size_t bytes = sizeof(index);
    values_push(&chunk->constants, value);

    chunk_push_op(chunk, op);
    da_push_many(chunk, &index, bytes);
}
