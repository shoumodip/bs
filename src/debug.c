#include "bs/debug.h"

static void bs_debug_op_int(Bs_Writer *w, const Bs_Chunk *c, size_t *offset, const char *name) {
    const size_t slot = *(const size_t *)&c->data[*offset];
    *offset += sizeof(slot);

    bs_write(w, "%-16s %4ld\n", name, slot);
}

static void bs_debug_op_value(Bs_Writer *w, const Bs_Chunk *c, size_t *offset, const char *name) {
    const size_t constant = *(const size_t *)&c->data[*offset];
    *offset += sizeof(constant);

    bs_write(w, "%-16s %4zu '", name, constant);
    bs_value_write(w, c->constants.data[constant]);
    bs_write(w, "'\n");
}

static_assert(BS_COUNT_OPS == 41, "Update bs_debug_op()");
void bs_debug_op(Bs_Writer *w, const Bs_Chunk *c, size_t *offset) {
    bs_write(w, "%04zu ", *offset);

    const Bs_Op op = c->data[(*offset)++];
    switch (op) {
    case BS_OP_RET:
        bs_write(w, "OP_RET\n");
        break;

    case BS_OP_CALL:
        bs_debug_op_int(w, c, offset, "OP_CALL");
        break;

    case BS_OP_CLOSURE: {
        const size_t constant = *(const size_t *)&c->data[*offset];
        *offset += sizeof(constant);

        const Bs_Value value = c->constants.data[constant];
        bs_write(w, "%-16s %4zu '", "OP_CLOSURE", constant);
        bs_value_write(w, value);
        bs_write(w, "'\n");

        const Bs_Fn *fn = (const Bs_Fn *)value.as.object;
        for (size_t i = 0; i < fn->upvalues; i++) {
            const bool local = c->data[(*offset)++];
            const size_t index = *(const size_t *)&c->data[*offset];
            *offset += sizeof(index);

            bs_write(
                w,
                "%04zu      |                     %s %zu\n",
                *offset - 1 - sizeof(index),
                local ? "local" : "upvalue",
                index);
        }
    } break;

    case BS_OP_DROP:
        bs_write(w, "OP_DROP\n");
        break;

    case BS_OP_UCLOSE:
        bs_debug_op_int(w, c, offset, "OP_UCLOSE");
        break;

    case BS_OP_NIL:
        bs_write(w, "OP_NIL\n");
        break;

    case BS_OP_TRUE:
        bs_write(w, "OP_TRUE\n");
        break;

    case BS_OP_FALSE:
        bs_write(w, "OP_FALSE\n");
        break;

    case BS_OP_ARRAY:
        bs_write(w, "OP_ARRAY\n");
        break;

    case BS_OP_TABLE:
        bs_write(w, "OP_TABLE\n");
        break;

    case BS_OP_CONST:
        bs_debug_op_value(w, c, offset, "OP_CONST");
        break;

    case BS_OP_ADD:
        bs_write(w, "OP_ADD\n");
        break;

    case BS_OP_SUB:
        bs_write(w, "OP_SUB\n");
        break;

    case BS_OP_MUL:
        bs_write(w, "OP_MUL\n");
        break;

    case BS_OP_DIV:
        bs_write(w, "OP_DIV\n");
        break;

    case BS_OP_NEG:
        bs_write(w, "OP_NEG\n");
        break;

    case BS_OP_NOT:
        bs_write(w, "OP_NOT\n");
        break;

    case BS_OP_GT:
        bs_write(w, "OP_GT\n");
        break;

    case BS_OP_GE:
        bs_write(w, "OP_GE\n");
        break;

    case BS_OP_LT:
        bs_write(w, "OP_LT\n");
        break;

    case BS_OP_LE:
        bs_write(w, "OP_LE\n");
        break;

    case BS_OP_EQ:
        bs_write(w, "OP_EQ\n");
        break;

    case BS_OP_NE:
        bs_write(w, "OP_NE\n");
        break;

    case BS_OP_LEN:
        bs_write(w, "OP_LEN\n");
        break;

    case BS_OP_JOIN:
        bs_write(w, "OP_JOIN\n");
        break;

    case BS_OP_IMPORT:
        bs_write(w, "OP_IMPORT\n");
        break;

    case BS_OP_GDEF:
        bs_debug_op_value(w, c, offset, "OP_GDEF");
        break;

    case BS_OP_GGET:
        bs_debug_op_value(w, c, offset, "OP_GGET");
        break;

    case BS_OP_GSET:
        bs_debug_op_value(w, c, offset, "OP_GSET");
        break;

    case BS_OP_CGET:
        bs_debug_op_value(w, c, offset, "OP_NGET");
        break;

    case BS_OP_LGET:
        bs_debug_op_int(w, c, offset, "OP_LGET");
        break;

    case BS_OP_LSET:
        bs_debug_op_int(w, c, offset, "OP_LSET");
        break;

    case BS_OP_UGET:
        bs_debug_op_int(w, c, offset, "OP_UGET");
        break;

    case BS_OP_USET:
        bs_debug_op_int(w, c, offset, "OP_USET");
        break;

    case BS_OP_IGET:
        bs_write(w, "OP_IGET\n");
        break;

    case BS_OP_ISET:
        bs_write(w, "OP_ISET\n");
        break;

    case BS_OP_ILIT:
        bs_write(w, "OP_ILIT\n");
        break;

    case BS_OP_JUMP:
        bs_debug_op_int(w, c, offset, "OP_JUMP");
        break;

    case BS_OP_ELSE:
        bs_debug_op_int(w, c, offset, "OP_ELSE");
        break;

    case BS_OP_THEN:
        bs_debug_op_int(w, c, offset, "OP_THEN");
        break;

    case BS_OP_PRINT:
        bs_write(w, "OP_PRINT\n");
        break;

    default:
        bs_write(w, "error: unknown opcode %d at offset %zu\n", op, *offset);
        return;
    }
}

void bs_debug_chunk(Bs_Writer *w, const Bs_Chunk *c) {
    size_t i = 0;
    while (i < c->count) {
        bs_debug_op(w, c, &i);
    }
}

void bs_debug_chunks(Bs_Writer *w, const Bs_Object *objects) {
    const Bs_Object *object = objects;
    while (object) {
        if (object->type == BS_OBJECT_FN) {
            const Bs_Fn *fn = (const Bs_Fn *)object;

            bs_write(w, "==== ");
            bs_value_write(w, bs_value_object(fn));
            bs_write(w, " ====\n");

            bs_debug_chunk(w, &fn->chunk);
            bs_write(w, "\n");
        }
        object = object->next;
    }
}
