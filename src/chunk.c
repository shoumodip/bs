#include "chunk.h"

void chunk_free(Chunk *chunk) {
    values_free(&chunk->constants);
    da_free(chunk);
}
