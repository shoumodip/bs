#include <assert.h>

#include "op.h"

static_assert(COUNT_OPS == 35, "Update op_get_to_set()");
Op op_get_to_set(Op op) {
    switch (op) {
    case OP_GGET:
        return OP_GSET;

    case OP_LGET:
        return OP_LSET;

    case OP_UGET:
        return OP_USET;

    case OP_AGET:
        return OP_ASET;

    default:
        return OP_RET;
    }
}
