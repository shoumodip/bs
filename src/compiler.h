#ifndef COMPILER_H
#define COMPILER_H

#include "object.h"

const ObjectFn *compile(Vm *vm, const char *path, SV sv);

#endif // COMPILER_H
