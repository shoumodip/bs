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

typedef struct {
    Writer meta;

    Vm *vm;
    char *data;
    size_t count;
    size_t capacity;
} WriterStr;

#define writer_str_free da_free

typedef struct {
    Writer meta;
    FILE *file;
} WriterFile;

struct Vm {
    Values stack;

    Frame *frame;
    Frames frames;

    ObjectTable globals;
    ObjectTable strings;
    ObjectUpvalue *upvalues;

    bool gc_on;
    size_t gc_max;
    size_t gc_bytes;
    Object *objects;

    WriterStr writer_str;
    WriterFile writer_stdout;
    WriterFile writer_stderr;
};

// The duplicate type definition is to appease the LSP unused include warnings
typedef struct Vm Vm;

#endif // BS_H
