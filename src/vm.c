#include <stdio.h>

#include "bs.h"
#include "debug.h"

static void vm_push(Memory *m, Value value) {
    values_push(&m->stack, value);
}

static Value vm_pop(Memory *m) {
    return m->stack.data[--m->stack.count];
}

static Value vm_peek(Memory *m, size_t offset) {
    return m->stack.data[m->stack.count - offset - 1];
}

static size_t vm_read_int(Memory *m) {
    const size_t index = *(size_t *)m->frame->ip;
    m->frame->ip += sizeof(index);
    return index;
}

static Value *vm_read_const(Memory *m) {
    return &m->frame->closure->fn->chunk.constants.data[vm_read_int(m)];
}

static bool vm_binary_op(Memory *m, Value *a, Value *b, const char *op) {
    *b = vm_pop(m);
    *a = vm_pop(m);

    if (a->type != VALUE_NUM || b->type != VALUE_NUM) {
        fprintf(
            stderr,
            "error: invalid operands to binary (%s): %s, %s\n",
            op,
            value_type_name(a->type),
            value_type_name(b->type));
        return false;
    }

    return true;
}

static bool vm_call(Memory *m, ObjectClosure *closure, size_t arity) {
    if (arity != closure->fn->arity) {
        fprintf(stderr, "error: expected %zu arguments, got %zu\n", closure->fn->arity, arity);
        return false;
    }

    const Frame frame = {
        .closure = closure,
        .ip = closure->fn->chunk.data,
        .base = m->stack.count - arity - 1,
    };

    frames_push(&m->frames, frame);
    m->frame = &m->frames.data[m->frames.count - 1];

    return true;
}

static ObjectUpvalue *vm_capture_upvalue(Bs *bs, size_t index) {
    Memory *m = &bs->memory;

    ObjectUpvalue *previous = NULL;
    ObjectUpvalue *upvalue = m->upvalues;
    while (upvalue && upvalue->index > index) {
        previous = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue && upvalue->index == index) {
        return upvalue;
    }

    ObjectUpvalue *created = bs_new_object_upvalue(bs, index);
    created->next = upvalue;

    if (previous) {
        previous->next = created;
    } else {
        m->upvalues = created;
    }

    return created;
}

static void vm_close_upvalues(Memory *m, size_t index) {
    while (m->upvalues && m->upvalues->index >= index) {
        ObjectUpvalue *upvalue = m->upvalues;
        upvalue->closed = true;
        upvalue->value = m->stack.data[upvalue->index];
        m->upvalues = upvalue->next;
    }
}

static_assert(COUNT_OPS == 35, "Update bs_interpret()");
bool bs_interpret(Bs *bs, const ObjectFn *fn, bool debug) {
    Memory *m = &bs->memory;

    if (debug) {
        debug_chunks(m->objects);
    }

    vm_push(m, value_object(fn));
    ObjectClosure *closure = bs_new_object_closure(bs, fn);
    vm_pop(m);

    vm_push(m, value_object(closure));
    vm_call(m, closure, 0);

    while (true) {
        if (debug) {
            printf("----------------------------------------\n");
            printf("Stack:\n");
            for (size_t i = 0; i < m->stack.count; i++) {
                printf("    ");
                value_print(m->stack.data[i], stdout);
                printf("\n");
            }
            printf("\n");

            size_t offset = m->frame->ip - m->frame->closure->fn->chunk.data;
            debug_op(&m->frame->closure->fn->chunk, &offset);
            printf("----------------------------------------\n");
            getchar();
        }

        const Op op = *m->frame->ip++;
        switch (op) {
        case OP_RET: {
            const Value result = vm_pop(m);
            vm_close_upvalues(m, m->frame->base);
            m->frames.count--;

            if (!m->frames.count) {
                m->frame = NULL;
                m->stack.count = 0;
                return true;
            }

            m->stack.count = m->frame->base;
            vm_push(m, result);

            m->frame = &m->frames.data[m->frames.count - 1];
        } break;

        case OP_CALL: {
            const size_t arity = vm_read_int(m);
            const Value value = vm_peek(m, arity);

            if (value.type != VALUE_OBJECT) {
                fprintf(stderr, "error: cannot call %s value\n", value_type_name(value.type));
                return false;
            }

            static_assert(COUNT_OBJECTS == 5, "Update bs_interpret()");
            switch (value.as.object->type) {
            case OBJECT_CLOSURE:
                if (!vm_call(m, (ObjectClosure *)value.as.object, arity)) {
                    return false;
                }
                break;

            default:
                fprintf(stderr, "error: cannot call %s value\n", value_type_name(value.type));
                return false;
            }

            m->frame = &m->frames.data[m->frames.count - 1];
        } break;

        case OP_CLOSURE: {
            ObjectFn *fn = (ObjectFn *)vm_read_const(m)->as.object;
            ObjectClosure *closure = bs_new_object_closure(bs, fn);
            vm_push(m, value_object(closure));

            for (size_t i = 0; i < closure->upvalues; i++) {
                const bool local = *m->frame->ip++;
                const size_t index = vm_read_int(m);

                if (local) {
                    closure->data[i] = vm_capture_upvalue(bs, m->frame->base + index);
                } else {
                    closure->data[i] = m->frame->closure->data[index];
                }
            }
        } break;

        case OP_DROP:
            vm_pop(m);
            break;

        case OP_UCLOSE:
            vm_close_upvalues(m, m->stack.count - 1);
            vm_pop(m);
            break;

        case OP_NIL:
            vm_push(m, value_nil);
            break;

        case OP_TRUE:
            vm_push(m, value_bool(true));
            break;

        case OP_FALSE:
            vm_push(m, value_bool(false));
            break;

        case OP_ARRAY:
            vm_push(m, value_object(bs_new_object_array(bs)));
            break;

        case OP_CONST:
            vm_push(m, *vm_read_const(m));
            break;

        case OP_ADD: {
            Value a, b;
            if (!vm_binary_op(m, &a, &b, "+")) {
                return false;
            }

            vm_push(m, value_num(a.as.number + b.as.number));
        } break;

        case OP_SUB: {
            Value a, b;
            if (!vm_binary_op(m, &a, &b, "-")) {
                return false;
            }

            vm_push(m, value_num(a.as.number - b.as.number));
        } break;

        case OP_MUL: {
            Value a, b;
            if (!vm_binary_op(m, &a, &b, "*")) {
                return false;
            }

            vm_push(m, value_num(a.as.number * b.as.number));
        } break;

        case OP_DIV: {
            Value a, b;
            if (!vm_binary_op(m, &a, &b, "/")) {
                return false;
            }

            vm_push(m, value_num(a.as.number / b.as.number));
        } break;

        case OP_NEG: {
            const Value a = vm_pop(m);
            if (a.type != VALUE_NUM) {
                fprintf(
                    stderr, "error: invalid operands to unary (-): %s\n", value_type_name(a.type));
                return false;
            }

            vm_push(m, value_num(-a.as.number));
        } break;

        case OP_NOT:
            vm_push(m, value_bool(value_is_falsey(vm_pop(m))));
            break;

        case OP_GT: {
            Value a, b;
            if (!vm_binary_op(m, &a, &b, ">")) {
                return false;
            }

            vm_push(m, value_bool(a.as.number > b.as.number));
        } break;

        case OP_GE: {
            Value a, b;
            if (!vm_binary_op(m, &a, &b, ">=")) {
                return false;
            }

            vm_push(m, value_bool(a.as.number >= b.as.number));
        } break;

        case OP_LT: {
            Value a, b;
            if (!vm_binary_op(m, &a, &b, "<")) {
                return false;
            }

            vm_push(m, value_bool(a.as.number < b.as.number));
        } break;

        case OP_LE: {
            Value a, b;
            if (!vm_binary_op(m, &a, &b, "<=")) {
                return false;
            }

            vm_push(m, value_bool(a.as.number <= b.as.number));
        } break;

        case OP_EQ: {
            const Value b = vm_pop(m);
            const Value a = vm_pop(m);
            vm_push(m, value_bool(value_equal(a, b)));
        } break;

        case OP_NE: {
            const Value b = vm_pop(m);
            const Value a = vm_pop(m);
            vm_push(m, value_bool(!value_equal(a, b)));
        } break;

        case OP_GDEF:
            table_set(&m->globals, bs, (ObjectStr *)vm_read_const(m)->as.object, vm_peek(m, 0));
            vm_pop(m);
            break;

        case OP_GGET: {
            ObjectStr *name = (ObjectStr *)vm_read_const(m)->as.object;

            Value value;
            if (!table_get(&m->globals, name, &value)) {
                fprintf(stderr, "error: undefined variable '" SVFmt "'\n", SVArg(*name));
                return false;
            }

            vm_push(m, value);
        } break;

        case OP_GSET: {
            ObjectStr *name = (ObjectStr *)vm_read_const(m)->as.object;

            if (table_set(&m->globals, bs, name, vm_peek(m, 0))) {
                table_remove(&m->globals, name);
                fprintf(stderr, "error: undefined variable '" SVFmt "'\n", SVArg(*name));
                return false;
            }
        } break;

        case OP_LGET:
            vm_push(m, m->stack.data[m->frame->base + vm_read_int(m)]);
            break;

        case OP_LSET:
            m->stack.data[m->frame->base + vm_read_int(m)] = vm_peek(m, 0);
            break;

        case OP_UGET: {
            ObjectUpvalue *upvalue = m->frame->closure->data[vm_read_int(m)];
            if (upvalue->closed) {
                vm_push(m, upvalue->value);
            } else {
                vm_push(m, m->stack.data[upvalue->index]);
            }
        } break;

        case OP_USET: {
            const Value value = vm_peek(m, 0);
            ObjectUpvalue *upvalue = m->frame->closure->data[vm_read_int(m)];
            if (upvalue->closed) {
                upvalue->value = value;
            } else {
                m->stack.data[upvalue->index] = value;
            }
        } break;

        case OP_AGET: {
            const Value index = vm_peek(m, 0);
            if (index.type != VALUE_NUM) {
                fprintf(stderr, "error: cannot index with %s value\n", value_type_name(index.type));
                return false;
            }

            if (index.as.number != (long)index.as.number) {
                fprintf(stderr, "error: cannot index with fractional value\n");
                return false;
            }

            const Value array = vm_peek(m, 1);
            if (array.type != VALUE_OBJECT || array.as.object->type != OBJECT_ARRAY) {
                fprintf(stderr, "error: cannot index into %s value\n", value_type_name(array.type));
                return false;
            }

            Value value;
            if (!array_get((ObjectArray *)array.as.object, index.as.number, &value)) {
                fprintf(
                    stderr,
                    "error: cannot get value at index %zu in array of length %zu\n",
                    (size_t)index.as.number,
                    ((ObjectArray *)array.as.object)->count);
                return false;
            }

            m->stack.count -= 2;
            vm_push(m, value);
        } break;

        case OP_ASET: {
            const Value value = vm_peek(m, 0);
            const Value index = vm_peek(m, 1);
            if (index.type != VALUE_NUM) {
                fprintf(stderr, "error: cannot index with %s value\n", value_type_name(index.type));
                return false;
            }

            if (index.as.number != (long)index.as.number) {
                fprintf(stderr, "error: cannot index with fractional value\n");
                return false;
            }

            const Value array = vm_peek(m, 2);
            if (array.type != VALUE_OBJECT || array.as.object->type != OBJECT_ARRAY) {
                fprintf(stderr, "error: cannot index into %s value\n", value_type_name(array.type));
                return false;
            }

            array_set((ObjectArray *)array.as.object, bs, index.as.number, value);
            m->stack.count -= 2;
        } break;

        case OP_JUMP:
            m->frame->ip += vm_read_int(m);
            break;

        case OP_ELSE: {
            const size_t offset = vm_read_int(m);
            if (value_is_falsey(vm_peek(m, 0))) {
                m->frame->ip += offset;
            }
        } break;

        case OP_THEN: {
            const size_t offset = vm_read_int(m);
            if (!value_is_falsey(vm_peek(m, 0))) {
                m->frame->ip += offset;
            }
        } break;

        case OP_PRINT:
            value_print(vm_pop(m), stdout);
            printf("\n");
            break;

        default:
            fprintf(
                stderr,
                "error: invalid op %d at offset %zu\n",
                op,
                m->frame->ip - m->frame->closure->fn->chunk.data);
            return false;
        }
    }
}

void bs_trace(Bs *bs) {
    Memory *m = &bs->memory;
    for (size_t i = m->frames.count; i > 0; i--) {
        const Frame *frame = &m->frames.data[i - 1];
        const ObjectFn *fn = frame->closure->fn;
        value_print(value_object(fn), stderr);
        fprintf(stderr, "\n");
    }

    m->stack.count = 0;
    m->frames.count = 0;
}
