#ifndef VM_H
#define VM_H

#include "value.h"

typedef struct {
    const ObjectClosure *closure;
    const uint8_t *ip;
    size_t base;
} Frame;

typedef struct {
    Frame *data;
    size_t count;
    size_t capacity;
} Frames;

#define frames_free da_free
#define frames_push da_push

typedef struct {
    GC gc;
    Values stack;

    Frame *frame;
    Frames frames;

    Table globals;
    ObjectUpvalue *upvalues;
} VM;

void vm_free(VM *vm);
bool vm_run(VM *vm, const ObjectFn *fn, bool step);
void vm_trace(VM *vm);

#endif // VM_H
