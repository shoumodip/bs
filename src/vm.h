#ifndef VM_H
#define VM_H

#include "chunk.h"

typedef struct {
    Chunk *chunk;
    uint8_t *ip;

    GC gc;
    Values stack;

    Table globals;
} VM;

void vm_free(VM *vm);
bool vm_run(VM *vm, Chunk *chunk);

#endif // VM_H
