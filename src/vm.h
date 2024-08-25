#ifndef VM_H
#define VM_H

#include "value.h"

typedef struct Vm Vm;

Object *object_new(Vm *vm, ObjectType type, size_t size);

void vm_free(Vm *vm);
void *vm_realloc(Vm *vm, void *ptr, size_t old_size, size_t new_size);

void vm_trace(Vm *vm, FILE *file);
bool vm_interpret(Vm *vm, const ObjectFn *fn, bool step);

#endif // VM_H
