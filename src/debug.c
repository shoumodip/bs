#include <assert.h>

#include "basic.h"
#include "debug.h"

static void debug_op_int(Writer *w, const Chunk *c, size_t *offset, const char *name) {
    const size_t slot = *(const size_t *)&c->data[*offset];
    *offset += sizeof(slot);

    w->fmt(w, "%-16s %4ld\n", name, slot);
}

static void debug_op_value(Writer *w, const Chunk *c, size_t *offset, const char *name) {
    const size_t constant = *(const size_t *)&c->data[*offset];
    *offset += sizeof(constant);

    w->fmt(w, "%-16s %4zu '", name, constant);
    value_write(c->constants.data[constant], w);
    w->fmt(w, "'\n");
}

static_assert(COUNT_OPS == 40, "Update debug_op()");
void debug_op(Writer *w, const Chunk *c, size_t *offset) {
    w->fmt(w, "%04zu ", *offset);

    const Op op = c->data[(*offset)++];
    switch (op) {
    case OP_RET:
        w->fmt(w, "OP_RET\n");
        break;

    case OP_CALL:
        debug_op_int(w, c, offset, "OP_CALL");
        break;

    case OP_CLOSURE: {
        const size_t constant = *(const size_t *)&c->data[*offset];
        *offset += sizeof(constant);

        const Value value = c->constants.data[constant];
        w->fmt(w, "%-16s %4zu '", "OP_CLOSURE", constant);
        value_write(value, w);
        w->fmt(w, "'\n");

        const ObjectFn *fn = (const ObjectFn *)value.as.object;
        for (size_t i = 0; i < fn->upvalues; i++) {
            const bool local = c->data[(*offset)++];
            const size_t index = *(const size_t *)&c->data[*offset];
            *offset += sizeof(index);

            w->fmt(
                w,
                "%04zu      |                     %s %zu\n",
                *offset - 1 - sizeof(index),
                local ? "local" : "upvalue",
                index);
        }
    } break;

    case OP_DROP:
        w->fmt(w, "OP_DROP\n");
        break;

    case OP_UCLOSE:
        debug_op_int(w, c, offset, "OP_UCLOSE");
        break;

    case OP_NIL:
        w->fmt(w, "OP_NIL\n");
        break;

    case OP_TRUE:
        w->fmt(w, "OP_TRUE\n");
        break;

    case OP_FALSE:
        w->fmt(w, "OP_FALSE\n");
        break;

    case OP_ARRAY:
        w->fmt(w, "OP_ARRAY\n");
        break;

    case OP_TABLE:
        w->fmt(w, "OP_TABLE\n");
        break;

    case OP_CONST:
        debug_op_value(w, c, offset, "OP_CONST");
        break;

    case OP_ADD:
        w->fmt(w, "OP_ADD\n");
        break;

    case OP_SUB:
        w->fmt(w, "OP_SUB\n");
        break;

    case OP_MUL:
        w->fmt(w, "OP_MUL\n");
        break;

    case OP_DIV:
        w->fmt(w, "OP_DIV\n");
        break;

    case OP_NEG:
        w->fmt(w, "OP_NEG\n");
        break;

    case OP_NOT:
        w->fmt(w, "OP_NOT\n");
        break;

    case OP_GT:
        w->fmt(w, "OP_GT\n");
        break;

    case OP_GE:
        w->fmt(w, "OP_GE\n");
        break;

    case OP_LT:
        w->fmt(w, "OP_LT\n");
        break;

    case OP_LE:
        w->fmt(w, "OP_LE\n");
        break;

    case OP_EQ:
        w->fmt(w, "OP_EQ\n");
        break;

    case OP_NE:
        w->fmt(w, "OP_NE\n");
        break;

    case OP_LEN:
        w->fmt(w, "OP_LEN\n");
        break;

    case OP_JOIN:
        w->fmt(w, "OP_JOIN\n");
        break;

    case OP_IMPORT:
        w->fmt(w, "OP_IMPORT\n");
        break;

    case OP_GDEF:
        debug_op_value(w, c, offset, "OP_GDEF");
        break;

    case OP_GGET:
        debug_op_value(w, c, offset, "OP_GGET");
        break;

    case OP_GSET:
        debug_op_value(w, c, offset, "OP_GSET");
        break;

    case OP_LGET:
        debug_op_int(w, c, offset, "OP_LGET");
        break;

    case OP_LSET:
        debug_op_int(w, c, offset, "OP_LSET");
        break;

    case OP_UGET:
        debug_op_int(w, c, offset, "OP_UGET");
        break;

    case OP_USET:
        debug_op_int(w, c, offset, "OP_USET");
        break;

    case OP_IGET:
        w->fmt(w, "OP_IGET\n");
        break;

    case OP_ISET:
        w->fmt(w, "OP_ISET\n");
        break;

    case OP_ILIT:
        w->fmt(w, "OP_ILIT\n");
        break;

    case OP_JUMP:
        debug_op_int(w, c, offset, "OP_JUMP");
        break;

    case OP_ELSE:
        debug_op_int(w, c, offset, "OP_ELSE");
        break;

    case OP_THEN:
        debug_op_int(w, c, offset, "OP_THEN");
        break;

    case OP_PRINT:
        w->fmt(w, "OP_PRINT\n");
        break;

    default:
        w->fmt(w, "error: unknown opcode %d at offset %zu\n", op, *offset);
        return;
    }
}

void debug_chunk(Writer *w, const Chunk *c) {
    size_t i = 0;
    while (i < c->count) {
        debug_op(w, c, &i);
    }
}

void debug_chunks(Writer *w, const Object *objects) {
    const Object *object = objects;
    while (object) {
        if (object->type == OBJECT_FN) {
            const ObjectFn *fn = (const ObjectFn *)object;

            if (fn->name) {
                w->fmt(w, "==== fn " SVFmt "() ====\n", SVArg(*fn->name));
            } else {
                w->fmt(w, "==== fn () ====\n");
            }

            debug_chunk(w, &fn->chunk);
            w->fmt(w, "\n");
        }
        object = object->next;
    }
}
