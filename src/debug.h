#ifndef DEBUG_H
#define DEBUG_H

#include "value.h"

void debug_chunk(const Chunk *chunk);
void debug_op(const Chunk *chunk, size_t *offset);

#endif // DEBUG_H
