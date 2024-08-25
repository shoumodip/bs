#ifndef BS_H
#define BS_H

#include "object.h"

typedef struct {
    size_t base;
    ObjectClosure *closure;

    const uint8_t *ip;
} Frame;

typedef struct {
    Frame *data;
    size_t count;
    size_t capacity;
} Frames;

#define frames_free da_free
#define frames_push da_push

struct Vm {
    Values stack;

    Frame *frame;
    Frames frames;

    Table globals;
    ObjectUpvalue *upvalues;

    bool gc_on;
    size_t gc_max;
    size_t gc_bytes;
    Object *objects;
};

// The duplicate type definition is to appease the LSP unused include warnings
typedef struct Vm Vm;

#endif // BS_H
