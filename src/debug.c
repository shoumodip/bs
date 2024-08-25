#include "debug.h"

static void debug_op_int(const Chunk *c, size_t *offset, const char *name) {
    const size_t slot = *(const size_t *)&c->data[*offset];
    *offset += sizeof(slot);

    printf("%-16s %4ld\n", name, slot);
}

static void debug_op_value(const Chunk *c, size_t *offset, const char *name) {
    const size_t constant = *(const size_t *)&c->data[*offset];
    *offset += sizeof(constant);

    printf("%-16s %4zu '", name, constant);
    value_print(c->constants.data[constant], stdout);
    printf("'\n");
}

static_assert(COUNT_OPS == 35, "Update debug_op()");
void debug_op(const Chunk *c, size_t *offset) {
    printf("%04zu ", *offset);

    const Op op = c->data[(*offset)++];
    switch (op) {
    case OP_RET:
        printf("OP_RET\n");
        break;

    case OP_CALL:
        debug_op_int(c, offset, "OP_CALL");
        break;

    case OP_CLOSURE: {
        const size_t constant = *(const size_t *)&c->data[*offset];
        *offset += sizeof(constant);

        const Value value = c->constants.data[constant];
        printf("%-16s %4zu '", "OP_CLOSURE", constant);
        value_print(value, stdout);
        printf("'\n");

        const ObjectFn *fn = (const ObjectFn *)value.as.object;
        for (size_t i = 0; i < fn->upvalues; i++) {
            const bool local = c->data[(*offset)++];
            const size_t index = *(const size_t *)&c->data[*offset];
            *offset += sizeof(index);

            printf(
                "%04zu      |                     %s %zu\n",
                *offset - 1 - sizeof(index),
                local ? "local" : "upvalue",
                index);
        }
    } break;

    case OP_DROP:
        printf("OP_DROP\n");
        break;

    case OP_UCLOSE:
        debug_op_int(c, offset, "OP_UCLOSE");
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

    case OP_ARRAY:
        printf("OP_ARRAY\n");
        break;

    case OP_CONST:
        debug_op_value(c, offset, "OP_CONST");
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
        debug_op_value(c, offset, "OP_GDEF");
        break;

    case OP_GGET:
        debug_op_value(c, offset, "OP_GGET");
        break;

    case OP_GSET:
        debug_op_value(c, offset, "OP_GSET");
        break;

    case OP_LGET:
        debug_op_int(c, offset, "OP_LGET");
        break;

    case OP_LSET:
        debug_op_int(c, offset, "OP_LSET");
        break;

    case OP_UGET:
        debug_op_int(c, offset, "OP_UGET");
        break;

    case OP_USET:
        debug_op_int(c, offset, "OP_USET");
        break;

    case OP_AGET:
        printf("OP_AGET\n");
        break;

    case OP_ASET:
        printf("OP_ASET\n");
        break;

    case OP_JUMP:
        debug_op_int(c, offset, "OP_JUMP");
        break;

    case OP_ELSE:
        debug_op_int(c, offset, "OP_ELSE");
        break;

    case OP_THEN:
        debug_op_int(c, offset, "OP_THEN");
        break;

    case OP_PRINT:
        printf("OP_PRINT\n");
        break;

    default:
        fprintf(stderr, "error: unknown opcode %d at offset %zu\n", op, *offset);
        return;
    }
}

void debug_chunk(const Chunk *c) {
    size_t i = 0;
    while (i < c->count) {
        debug_op(c, &i);
    }
}

void debug_chunks(const Object *objects) {
    const Object *object = objects;
    while (object) {
        if (object->type == OBJECT_FN) {
            const ObjectFn *fn = (const ObjectFn *)object;

            if (fn->name) {
                printf("==== fn " SVFmt "() ====\n", SVArg(*fn->name));
            } else {
                printf("==== fn () ====\n");
            }

            debug_chunk(&fn->chunk);
            printf("\n");
        }
        object = object->next;
    }
}
