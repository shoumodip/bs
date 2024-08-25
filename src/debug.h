#ifndef DEBUG_H
#define DEBUG_H

#include "object.h"

void debug_op(const Chunk *chunk, size_t *offset);
void debug_chunk(const Chunk *chunk);
void debug_chunks(const Object *objects);

#endif // DEBUG_H
