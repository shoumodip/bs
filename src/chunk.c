#include "chunk.h"

void chunk_free(Chunk *chunk) {
    values_free(&chunk->constants);
    da_free(chunk);
}

void chunk_const(Chunk *chunk, Op op, Value value) {
    const size_t index = chunk->constants.count;
    const size_t bytes = sizeof(index);

    values_push(&chunk->constants, value);

    chunk_push(chunk, op);
    da_push_many(chunk, &index, bytes);
}
