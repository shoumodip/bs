#include "bs/debug.h"

static void
bs_debug_op_int(Bs_Pretty_Printer *p, const Bs_Chunk *c, size_t *offset, const char *name) {
    const size_t slot = *(const size_t *)&c->data[*offset];
    *offset += sizeof(slot);

    bs_fmt(p->writer, "%-16s %4ld\n", name, slot);
}

static void
bs_debug_op_value(Bs_Pretty_Printer *p, const Bs_Chunk *c, size_t *offset, const char *name) {
    const size_t constant = *(const size_t *)&c->data[*offset];
    *offset += sizeof(constant);

    bs_fmt(p->writer, "%-16s %4zu '", name, constant);
    bs_value_write_impl(p, c->constants.data[constant]);
    bs_fmt(p->writer, "'\n");
}

static void
bs_debug_op_invoke(Bs_Pretty_Printer *p, const Bs_Chunk *c, size_t *offset, const char *name) {
    const size_t constant = *(const size_t *)&c->data[*offset];
    *offset += sizeof(constant);

    const size_t arity = c->data[(*offset)++];
    bs_fmt(p->writer, "%-16s (%zu args) %4zu '", name, arity, constant);
    bs_value_write_impl(p, c->constants.data[constant]);
    bs_fmt(p->writer, "'\n");
}

static_assert(BS_COUNT_OPS == 64, "Update bs_debug_op()");
void bs_debug_op(Bs_Pretty_Printer *p, const Bs_Chunk *c, size_t *offset) {
    bs_fmt(p->writer, "%04zu ", *offset);

    const Bs_Op op = c->data[(*offset)++];
    switch (op) {
    case BS_OP_RET:
        bs_fmt(p->writer, "OP_RET\n");
        break;

    case BS_OP_CALL:
        bs_fmt(p->writer, "OP_CALL %d\n", c->data[(*offset)++]);
        break;

    case BS_OP_CLOSURE: {
        const size_t constant = *(const size_t *)&c->data[*offset];
        *offset += sizeof(constant);

        const Bs_Value value = c->constants.data[constant];
        bs_fmt(p->writer, "%-16s %4zu '", "OP_CLOSURE", constant);
        bs_value_write_impl(p, value);
        bs_fmt(p->writer, "'\n");

        const Bs_Fn *fn = (const Bs_Fn *)value.as.object;
        for (size_t i = 0; i < fn->upvalues; i++) {
            const bool local = c->data[(*offset)++];
            const size_t index = *(const size_t *)&c->data[*offset];
            *offset += sizeof(index);

            bs_fmt(
                p->writer,
                "%04zu      |                     %s %zu\n",
                *offset - 1 - sizeof(index),
                local ? "local" : "upvalue",
                index);
        }
    } break;

    case BS_OP_DROP:
        bs_fmt(p->writer, "OP_DROP\n");
        break;

    case BS_OP_UCLOSE:
        bs_fmt(p->writer, "OP_UCLOSE\n");
        break;

    case BS_OP_NIL:
        bs_fmt(p->writer, "OP_NIL\n");
        break;

    case BS_OP_TRUE:
        bs_fmt(p->writer, "OP_TRUE\n");
        break;

    case BS_OP_FALSE:
        bs_fmt(p->writer, "OP_FALSE\n");
        break;

    case BS_OP_ARRAY:
        bs_fmt(p->writer, "OP_ARRAY\n");
        break;

    case BS_OP_TABLE:
        bs_fmt(p->writer, "OP_TABLE\n");
        break;

    case BS_OP_CONST:
        bs_debug_op_value(p, c, offset, "OP_CONST");
        break;

    case BS_OP_CLASS:
        bs_debug_op_value(p, c, offset, "OP_CLASS");
        break;

    case BS_OP_INVOKE:
        bs_debug_op_invoke(p, c, offset, "OP_INVOKE");
        break;

    case BS_OP_METHOD:
        bs_debug_op_value(p, c, offset, "OP_METHOD");
        break;

    case BS_OP_INIT_METHOD:
        bs_fmt(p->writer, "OP_INIT_METHOD%s\n", c->data[(*offset)++] ? "      # Can fail" : "");
        break;

    case BS_OP_INHERIT:
        bs_fmt(p->writer, "OP_INHERIT\n");
        break;

    case BS_OP_SUPER_GET:
        bs_debug_op_value(p, c, offset, "OP_SUPER_GET");
        break;

    case BS_OP_SUPER_INVOKE:
        bs_debug_op_invoke(p, c, offset, "OP_SUPER_INVOKE");
        break;

    case BS_OP_ADD:
        bs_fmt(p->writer, "OP_ADD\n");
        break;

    case BS_OP_SUB:
        bs_fmt(p->writer, "OP_SUB\n");
        break;

    case BS_OP_MUL:
        bs_fmt(p->writer, "OP_MUL\n");
        break;

    case BS_OP_DIV:
        bs_fmt(p->writer, "OP_DIV\n");
        break;

    case BS_OP_MOD:
        bs_fmt(p->writer, "OP_MOD\n");
        break;

    case BS_OP_NEG:
        bs_fmt(p->writer, "OP_NEG\n");
        break;

    case BS_OP_BOR:
        bs_fmt(p->writer, "OP_BOR\n");
        break;

    case BS_OP_BAND:
        bs_fmt(p->writer, "OP_BAND\n");
        break;

    case BS_OP_BXOR:
        bs_fmt(p->writer, "OP_BXOR\n");
        break;

    case BS_OP_BNOT:
        bs_fmt(p->writer, "OP_BNOT\n");
        break;

    case BS_OP_LXOR:
        bs_fmt(p->writer, "OP_LXOR\n");
        break;

    case BS_OP_LNOT:
        bs_fmt(p->writer, "OP_LNOT\n");
        break;

    case BS_OP_SHL:
        bs_fmt(p->writer, "OP_SHL\n");
        break;

    case BS_OP_SHR:
        bs_fmt(p->writer, "OP_SHR\n");
        break;

    case BS_OP_GT:
        bs_fmt(p->writer, "OP_GT\n");
        break;

    case BS_OP_GE:
        bs_fmt(p->writer, "OP_GE\n");
        break;

    case BS_OP_LT:
        bs_fmt(p->writer, "OP_LT\n");
        break;

    case BS_OP_LE:
        bs_fmt(p->writer, "OP_LE\n");
        break;

    case BS_OP_EQ:
        bs_fmt(p->writer, "OP_EQ\n");
        break;

    case BS_OP_NE:
        bs_fmt(p->writer, "OP_NE\n");
        break;

    case BS_OP_IN:
        bs_fmt(p->writer, "OP_IN\n");
        break;

    case BS_OP_LEN:
        bs_fmt(p->writer, "OP_LEN\n");
        break;

    case BS_OP_JOIN:
        bs_fmt(p->writer, "OP_JOIN\n");
        break;

    case BS_OP_IMPORT:
        bs_fmt(p->writer, "OP_IMPORT\n");
        break;

    case BS_OP_TYPEOF:
        bs_fmt(p->writer, "OP_TYPEOF\n");
        break;

    case BS_OP_DELETE:
        bs_fmt(p->writer, "OP_DELETE\n");
        break;

    case BS_OP_DELETE_CONST:
        bs_debug_op_value(p, c, offset, "OP_DELETE_CONST");
        break;

    case BS_OP_GDEF:
        bs_debug_op_value(p, c, offset, "OP_GDEF");
        break;

    case BS_OP_GGET:
        bs_debug_op_value(p, c, offset, "OP_GGET");
        break;

    case BS_OP_GSET:
        bs_debug_op_value(p, c, offset, "OP_GSET");
        break;

    case BS_OP_LGET:
        bs_debug_op_int(p, c, offset, "OP_LGET");
        break;

    case BS_OP_LSET:
        bs_debug_op_int(p, c, offset, "OP_LSET");
        break;

    case BS_OP_UGET:
        bs_debug_op_int(p, c, offset, "OP_UGET");
        break;

    case BS_OP_USET:
        bs_debug_op_int(p, c, offset, "OP_USET");
        break;

    case BS_OP_LRECEIVER:
        bs_debug_op_int(p, c, offset, "OP_LRECEIVER");
        break;

    case BS_OP_URECEIVER:
        bs_debug_op_int(p, c, offset, "OP_URECEIVER");
        break;

    case BS_OP_IGET:
        bs_fmt(p->writer, "OP_IGET\n");
        break;

    case BS_OP_ISET:
        bs_fmt(p->writer, "OP_ISET\n");
        break;

    case BS_OP_IGET_CONST:
        bs_debug_op_value(p, c, offset, "OP_IGET_CONST");
        break;

    case BS_OP_ISET_CONST:
        bs_debug_op_value(p, c, offset, "OP_ISET_CONST");
        break;

    case BS_OP_ISET_CHAIN:
        bs_fmt(p->writer, "OP_ISET_CHAIN\n");
        break;

    case BS_OP_JUMP:
        bs_debug_op_int(p, c, offset, "OP_JUMP");
        break;

    case BS_OP_ELSE:
        bs_debug_op_int(p, c, offset, "OP_ELSE");
        break;

    case BS_OP_THEN:
        bs_debug_op_int(p, c, offset, "OP_THEN");
        break;

    case BS_OP_ITER:
        bs_debug_op_int(p, c, offset, "OP_ITER");
        break;

    case BS_OP_RANGE:
        bs_debug_op_int(p, c, offset, "OP_RANGE");
        break;

    default:
        bs_fmt(p->writer, "error: unknown opcode %d at offset %zu\n", op, *offset);
        return;
    }
}

void bs_debug_chunk(Bs_Pretty_Printer *p, const Bs_Chunk *c) {
    size_t i = 0;
    size_t oploc = 0;
    while (i < c->count) {
        bs_debug_op(p, c, &i);
        if (oploc < c->locations.count) {
            bool found = false;
            while (oploc < c->locations.count && c->locations.data[oploc].index == i) {
                found = true;
                bs_fmt(p->writer, Bs_Loc_Fmt "\n", Bs_Loc_Arg(c->locations.data[oploc].loc));
                oploc++;
            }

            if (found) {
                bs_fmt(p->writer, "\n");
            }
        }
    }
}

void bs_debug_chunks(Bs_Pretty_Printer *p, const Bs_Object *objects) {
    const Bs_Object *object = objects;
    while (object) {
        if (object->type == BS_OBJECT_FN) {
            const Bs_Fn *fn = (const Bs_Fn *)object;

            bs_fmt(p->writer, "==== ");
            bs_value_write_impl(p, bs_value_object(fn));
            bs_fmt(p->writer, " ====\n");

            bs_debug_chunk(p, &fn->chunk);
            bs_fmt(p->writer, "\n");
        }
        object = object->next;
    }
}
