#ifndef BS_COMPILER_H
#define BS_COMPILER_H

#include "object.h"

Bs_Closure *bs_compile(Bs *bs, Bs_Sv path, Bs_Sv input, bool is_main, bool is_repl, bool is_meta);

#endif // BS_COMPILER_H
