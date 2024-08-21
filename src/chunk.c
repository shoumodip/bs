#include <stdio.h>

#include "chunk.h"

void chunk_free(Chunk *chunk) {
    values_free(&chunk->constants);
    da_free(chunk);
}

static void chunk_print_op_int(Chunk *chunk, size_t *offset, const char *name) {
    const size_t slot = *(size_t *)&chunk->data[*offset];
    *offset += sizeof(slot);

    printf("%-16s %4zu\n", name, slot);
}

static void chunk_print_op_value(Chunk *chunk, size_t *offset, const char *name) {
    const size_t constant = *(size_t *)&chunk->data[*offset];
    *offset += sizeof(constant);

    printf("%-16s %4zu '", name, constant);
    value_print(chunk->constants.data[constant]);
    printf("'\n");
}

static_assert(COUNT_OPS == 19, "Update chunk_print()");
void chunk_print(Chunk *chunk) {
    size_t i = 0;
    while (i < chunk->count) {
        printf("%04zu ", i);

        const Op op = chunk->data[i++];
        switch (op) {
        case OP_HALT:
            printf("OP_HALT\n");
            break;

        case OP_DROP:
            printf("OP_DROP\n");
            break;

        case OP_DROPS:
            chunk_print_op_int(chunk, &i, "OP_DROPS");
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
            chunk_print_op_value(chunk, &i, "OP_CONST");
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

        case OP_GDEF:
            chunk_print_op_value(chunk, &i, "OP_GDEF");
            break;

        case OP_GGET:
            chunk_print_op_value(chunk, &i, "OP_GGET");
            break;

        case OP_GSET:
            chunk_print_op_value(chunk, &i, "OP_GSET");
            break;

        case OP_LGET:
            chunk_print_op_int(chunk, &i, "OP_LGET");
            break;

        case OP_LSET:
            chunk_print_op_int(chunk, &i, "OP_LSET");
            break;

        case OP_PRINT:
            printf("OP_PRINT\n");
            break;

        default:
            fprintf(stderr, "error: unknown opcode %d at offset %zu\n", op, i);
            return;
        }
    }
}

void chunk_push_op(Chunk *chunk, Op op) {
    chunk->last = chunk->count;
    da_push(chunk, op);
}

void chunk_push_int(Chunk *chunk, Op op, size_t value) {
    const size_t bytes = sizeof(value);
    chunk_push_op(chunk, op);
    da_push_many(chunk, &value, bytes);
}

void chunk_push_value(Chunk *chunk, Op op, Value value) {
    const size_t index = chunk->constants.count;
    const size_t bytes = sizeof(index);
    values_push(&chunk->constants, value);

    chunk_push_op(chunk, op);
    da_push_many(chunk, &index, bytes);
}
