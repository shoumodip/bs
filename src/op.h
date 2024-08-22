#ifndef CHUNK_H
#define CHUNK_H

typedef enum {
    OP_RET,
    OP_CALL,

    OP_DROP,
    OP_DROPS,

    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_CONST,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NEG,
    OP_NOT,

    OP_GT,
    OP_GE,
    OP_LT,
    OP_LE,
    OP_EQ,
    OP_NE,

    OP_GDEF,
    OP_GGET,
    OP_GSET,
    OP_LGET,
    OP_LSET,

    OP_JUMP,
    OP_ELSE,
    OP_THEN,

    OP_PRINT,
    COUNT_OPS,
} Op;

Op op_get_to_set(Op op);

#endif // CHUNK_H
