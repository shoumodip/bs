#ifndef BS_OP_H
#define BS_OP_H

typedef enum {
    BS_OP_RET,
    BS_OP_CALL,
    BS_OP_CLOSURE,

    BS_OP_DROP,
    BS_OP_UCLOSE,

    BS_OP_NIL,
    BS_OP_TRUE,
    BS_OP_FALSE,
    BS_OP_ARRAY,
    BS_OP_TABLE,
    BS_OP_CONST,

    BS_OP_ADD,
    BS_OP_SUB,
    BS_OP_MUL,
    BS_OP_DIV,
    BS_OP_NEG,
    BS_OP_NOT,

    BS_OP_GT,
    BS_OP_GE,
    BS_OP_LT,
    BS_OP_LE,
    BS_OP_EQ,
    BS_OP_NE,

    BS_OP_LEN,
    BS_OP_JOIN,
    BS_OP_IMPORT,
    BS_OP_TYPEOF,

    BS_OP_GDEF,
    BS_OP_GGET,
    BS_OP_GSET,
    BS_OP_LGET,
    BS_OP_LSET,
    BS_OP_UGET,
    BS_OP_USET,
    BS_OP_IGET,
    BS_OP_ISET,
    BS_OP_ILIT,

    BS_OP_JUMP,
    BS_OP_ELSE,
    BS_OP_THEN,

    BS_OP_ITER,
    BS_OP_RANGE,
    BS_COUNT_OPS,
} Bs_Op;

Bs_Op bs_op_get_to_set(Bs_Op op);

#endif // BS_OP_H
