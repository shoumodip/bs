#ifndef VM_H
#define VM_H

#include "op.h"

typedef struct {
    Chunk *chunk;
    uint8_t *ip;

    GC gc;
    Values stack;

    Table globals;
} VM;

void vm_free(VM *vm);
bool vm_run(VM *vm, Chunk *chunk, bool step);

#endif // VM_H
