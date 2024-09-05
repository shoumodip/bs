#ifndef BS_COMPILER_H
#define BS_COMPILER_H

#include "object.h"

Bs_Fn *bs_compile(Bs *bs, const char *path, Bs_Sv input, bool is_main_module);

#endif // BS_COMPILER_H
