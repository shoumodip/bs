#include <stdio.h>

#include "debug.h"

static void debug_op_int(Chunk *chunk, size_t *offset, const char *name) {
    const size_t slot = *(size_t *)&chunk->data[*offset];
    *offset += sizeof(slot);

    printf("%-16s %4ld\n", name, slot);
}

static void debug_op_value(Chunk *chunk, size_t *offset, const char *name) {
    const size_t constant = *(size_t *)&chunk->data[*offset];
    *offset += sizeof(constant);

    printf("%-16s %4zu '", name, constant);
    value_print(chunk->constants.data[constant]);
    printf("'\n");
}

static_assert(COUNT_OPS == 28, "Update debug_op()");
void debug_op(Chunk *chunk, size_t *offset) {
    printf("%04zu ", *offset);

    const Op op = chunk->data[(*offset)++];
    switch (op) {
    case OP_HALT:
        printf("OP_HALT\n");
        break;

    case OP_DROP:
        printf("OP_DROP\n");
        break;

    case OP_DROPS:
        debug_op_int(chunk, offset, "OP_DROPS");
        break;

    case OP_NIL:
        printf("OP_NIL\n");
        break;

    case OP_TRUE:
        printf("OP_TRUE\n");
        break;

    case OP_FALSE:
        printf("OP_FALSE\n");
        break;

    case OP_CONST:
        debug_op_value(chunk, offset, "OP_CONST");
        break;

    case OP_ADD:
        printf("OP_ADD\n");
        break;

    case OP_SUB:
        printf("OP_SUB\n");
        break;

    case OP_MUL:
        printf("OP_MUL\n");
        break;

    case OP_DIV:
        printf("OP_DIV\n");
        break;

    case OP_NEG:
        printf("OP_NEG\n");
        break;

    case OP_NOT:
        printf("OP_NOT\n");
        break;

    case OP_GT:
        printf("OP_GT\n");
        break;

    case OP_GE:
        printf("OP_GE\n");
        break;

    case OP_LT:
        printf("OP_LT\n");
        break;

    case OP_LE:
        printf("OP_LE\n");
        break;

    case OP_EQ:
        printf("OP_EQ\n");
        break;

    case OP_NE:
        printf("OP_NE\n");
        break;

    case OP_GDEF:
        debug_op_value(chunk, offset, "OP_GDEF");
        break;

    case OP_GGET:
        debug_op_value(chunk, offset, "OP_GGET");
        break;

    case OP_GSET:
        debug_op_value(chunk, offset, "OP_GSET");
        break;

    case OP_LGET:
        debug_op_int(chunk, offset, "OP_LGET");
        break;

    case OP_LSET:
        debug_op_int(chunk, offset, "OP_LSET");
        break;

    case OP_JUMP:
        debug_op_int(chunk, offset, "OP_JUMP");
        break;

    case OP_ELSE:
        debug_op_int(chunk, offset, "OP_ELSE");
        break;

    case OP_THEN:
        debug_op_int(chunk, offset, "OP_THEN");
        break;

    case OP_PRINT:
        printf("OP_PRINT\n");
        break;

    default:
        fprintf(stderr, "error: unknown opcode %d at offset %zu\n", op, *offset);
        return;
    }
}

void debug_chunk(Chunk *chunk) {
    size_t i = 0;
    while (i < chunk->count) {
        debug_op(chunk, &i);
    }
}
