#ifndef BS_DEBUG_H
#define BS_DEBUG_H

#include "object.h"

void bs_debug_op(Bs_Writer *writer, const Bs_Chunk *chunk, size_t *offset);
void bs_debug_chunk(Bs_Writer *writer, const Bs_Chunk *chunk);
void bs_debug_chunks(Bs_Writer *writer, const Bs_Object *objects);

#endif // BS_DEBUG_H