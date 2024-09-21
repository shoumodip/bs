#include <assert.h>

#include "bs/op.h"

static_assert(BS_COUNT_OPS == 58, "Update bs_op_get_to_set()");
Bs_Op bs_op_get_to_set(Bs_Op op) {
    switch (op) {
    case BS_OP_GGET:
        return BS_OP_GSET;

    case BS_OP_LGET:
        return BS_OP_LSET;

    case BS_OP_UGET:
        return BS_OP_USET;

    case BS_OP_IGET:
        return BS_OP_ISET;

    case BS_OP_IGET_CONST:
        return BS_OP_ISET_CONST;

    default:
        return BS_OP_RET;
    }
}
