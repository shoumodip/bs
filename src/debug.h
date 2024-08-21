#ifndef DEBUG_H
#define DEBUG_H

#include "chunk.h"

void debug_chunk(Chunk *chunk);
void debug_op(Chunk *chunk, size_t *offset);

#endif // DEBUG_H
