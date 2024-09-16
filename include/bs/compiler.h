#ifndef BS_COMPILER_H
#define BS_COMPILER_H

#include "object.h"

Bs_Fn *bs_compile_impl(Bs *bs, Bs_Sv path, Bs_Sv input, bool is_main);

#endif // BS_COMPILER_H
