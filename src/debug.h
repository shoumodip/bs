#ifndef DEBUG_H
#define DEBUG_H

#include "object.h"

void debug_op(Writer *w, const Chunk *chunk, size_t *offset);
void debug_chunk(Writer *w, const Chunk *chunk);
void debug_chunks(Writer *w, const Object *objects);

#endif // DEBUG_H
