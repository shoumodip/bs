#ifndef VM_H
#define VM_H

#include "chunk.h"

typedef struct {
    Chunk *chunk;
    uint8_t *ip;

    Values stack;
} Vm;

void vm_free(Vm *vm);
bool vm_run(Vm *vm, Chunk *chunk);

#endif // VM_H
