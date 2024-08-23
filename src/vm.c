#include <stdio.h>

#include "debug.h"
#include "vm.h"

static void vm_push(VM *vm, Value value) {
    values_push(&vm->stack, value);
}

static Value vm_pop(VM *vm) {
    return vm->stack.data[--vm->stack.count];
}

static Value vm_peek(VM *vm, size_t offset) {
    return vm->stack.data[vm->stack.count - offset - 1];
}

static size_t vm_read_int(VM *vm) {
    const size_t index = *(size_t *)vm->frame->ip;
    vm->frame->ip += sizeof(index);
    return index;
}

static Value *vm_read_const(VM *vm) {
    return &vm->frame->closure->fn->chunk.constants.data[vm_read_int(vm)];
}

static_assert(COUNT_OBJECTS == 4, "Update gc_free_object()");
static void gc_free_object(GC *gc, Object *object) {
    switch (object->type) {
    case OBJECT_FN: {
        ObjectFn *fn = (ObjectFn *)object;
        chunk_free(&fn->chunk);
        gc_realloc(gc, fn, sizeof(*fn), 0);
    } break;

    case OBJECT_STR: {
        ObjectStr *str = (ObjectStr *)object;
        gc_realloc(gc, str, sizeof(*str) + str->size, 0);
    } break;

    case OBJECT_UPVALUE:
        gc_realloc(gc, object, sizeof(ObjectUpvalue), 0);
        break;

    case OBJECT_CLOSURE: {
        ObjectClosure *closure = (ObjectClosure *)object;
        gc_realloc(gc, closure, sizeof(*closure) + sizeof(ObjectUpvalue *) * closure->upvalues, 0);
    } break;

    default:
        assert(false && "unreachable");
    }
}

void vm_free(VM *vm) {
    Object *object = vm->gc.objects;
    while (object) {
        Object *next = object->next;
        gc_free_object(&vm->gc, object);
        object = next;
    }

    values_free(&vm->stack);
    frames_free(&vm->frames);
    table_free(&vm->globals, &vm->gc);
}

static bool vm_binary_op(VM *vm, Value *a, Value *b, const char *op) {
    *b = vm_pop(vm);
    *a = vm_pop(vm);

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

static bool vm_call(VM *vm, const ObjectClosure *closure, size_t arity) {
    if (arity != closure->fn->arity) {
        fprintf(stderr, "error: expected %zu arguments, got %zu\n", closure->fn->arity, arity);
        return false;
    }

    const Frame frame = {
        .closure = closure,
        .ip = closure->fn->chunk.data,
        .base = vm->stack.count - arity - 1,
    };

    frames_push(&vm->frames, frame);
    vm->frame = &vm->frames.data[vm->frames.count - 1];

    return true;
}

static ObjectUpvalue *vm_capture_upvalue(VM *vm, size_t index) {
    ObjectUpvalue *previous = NULL;
    ObjectUpvalue *upvalue = vm->upvalues;
    while (upvalue && upvalue->index > index) {
        previous = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue && upvalue->index == index) {
        return upvalue;
    }

    ObjectUpvalue *created = gc_new_object_upvalue(&vm->gc, index);
    created->next = upvalue;

    if (previous) {
        previous->next = created;
    } else {
        vm->upvalues = created;
    }

    return created;
}

static void vm_close_upvalues(VM *vm, size_t index) {
    while (vm->upvalues && vm->upvalues->index >= index) {
        ObjectUpvalue *upvalue = vm->upvalues;
        upvalue->closed = true;
        upvalue->value = vm->stack.data[upvalue->index];
        vm->upvalues = upvalue->next;
    }
}

static_assert(COUNT_OPS == 32, "Update vm_run()");
bool vm_run(VM *vm, const ObjectFn *fn, bool debug) {
    if (debug) {
        debug_chunks(vm->gc.objects);
    }

    vm_push(vm, value_object(fn));

    ObjectClosure *closure = gc_new_object_closure(&vm->gc, fn);
    vm_pop(vm);

    vm_push(vm, value_object(closure));
    vm_call(vm, closure, 0);

    while (true) {
        if (debug) {
            printf("----------------------------------------\n");
            printf("Stack:\n");
            for (size_t i = 0; i < vm->stack.count; i++) {
                printf("    ");
                value_print(vm->stack.data[i], stdout);
                printf("\n");
            }
            printf("\n");

            size_t offset = vm->frame->ip - vm->frame->closure->fn->chunk.data;
            debug_op(&vm->frame->closure->fn->chunk, &offset);
            printf("----------------------------------------\n");
            getchar();
        }

        const Op op = *vm->frame->ip++;
        switch (op) {
        case OP_RET: {
            const Value result = vm_pop(vm);
            vm_close_upvalues(vm, vm->frame->base);
            vm->frames.count--;

            if (!vm->frames.count) {
                vm->frame = NULL;
                vm->stack.count = 0;
                return true;
            }

            vm->stack.count = vm->frame->base;
            vm_push(vm, result);

            vm->frame = &vm->frames.data[vm->frames.count - 1];
        } break;

        case OP_CALL: {
            const size_t arity = vm_read_int(vm);
            const Value value = vm_peek(vm, arity);

            if (value.type != VALUE_OBJECT) {
                fprintf(stderr, "error: cannot call %s value\n", value_type_name(value.type));
                return false;
            }

            static_assert(COUNT_OBJECTS == 4, "Update vm_run()");
            switch (value.as.object->type) {
            case OBJECT_CLOSURE:
                if (!vm_call(vm, (const ObjectClosure *)value.as.object, arity)) {
                    return false;
                }
                break;

            case OBJECT_STR:
                fprintf(stderr, "error: cannot call %s value\n", value_type_name(value.type));
                return false;

            default:
                assert(false && "unreachable");
            }

            vm->frame = &vm->frames.data[vm->frames.count - 1];
        } break;

        case OP_CLOSURE: {
            ObjectFn *fn = (ObjectFn *)vm_read_const(vm)->as.object;
            ObjectClosure *closure = gc_new_object_closure(&vm->gc, fn);
            vm_push(vm, value_object(closure));

            for (size_t i = 0; i < closure->upvalues; i++) {
                const bool local = *vm->frame->ip++;
                const size_t index = vm_read_int(vm);

                if (local) {
                    closure->data[i] = vm_capture_upvalue(vm, vm->frame->base + index);
                } else {
                    closure->data[i] = vm->frame->closure->data[index];
                }
            }
        } break;

        case OP_DROP:
            vm_pop(vm);
            break;

        case OP_UCLOSE:
            vm_close_upvalues(vm, vm->stack.count - 1);
            vm_pop(vm);
            break;

        case OP_NIL:
            vm_push(vm, value_nil);
            break;

        case OP_TRUE:
            vm_push(vm, value_bool(true));
            break;

        case OP_FALSE:
            vm_push(vm, value_bool(false));
            break;

        case OP_CONST:
            vm_push(vm, *vm_read_const(vm));
            break;

        case OP_ADD: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, "+")) {
                return false;
            }

            vm_push(vm, value_num(a.as.number + b.as.number));
        } break;

        case OP_SUB: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, "-")) {
                return false;
            }

            vm_push(vm, value_num(a.as.number - b.as.number));
        } break;

        case OP_MUL: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, "*")) {
                return false;
            }

            vm_push(vm, value_num(a.as.number * b.as.number));
        } break;

        case OP_DIV: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, "/")) {
                return false;
            }

            vm_push(vm, value_num(a.as.number / b.as.number));
        } break;

        case OP_NEG: {
            const Value a = vm_pop(vm);
            if (a.type != VALUE_NUM) {
                fprintf(
                    stderr, "error: invalid operands to unary (-): %s\n", value_type_name(a.type));
                return false;
            }

            vm_push(vm, value_num(-a.as.number));
        } break;

        case OP_NOT:
            vm_push(vm, value_bool(value_is_falsey(vm_pop(vm))));
            break;

        case OP_GT: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, ">")) {
                return false;
            }

            vm_push(vm, value_bool(a.as.number > b.as.number));
        } break;

        case OP_GE: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, ">=")) {
                return false;
            }

            vm_push(vm, value_bool(a.as.number >= b.as.number));
        } break;

        case OP_LT: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, "<")) {
                return false;
            }

            vm_push(vm, value_bool(a.as.number < b.as.number));
        } break;

        case OP_LE: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, "<=")) {
                return false;
            }

            vm_push(vm, value_bool(a.as.number <= b.as.number));
        } break;

        case OP_EQ: {
            const Value b = vm_pop(vm);
            const Value a = vm_pop(vm);
            vm_push(vm, value_bool(value_equal(a, b)));
        } break;

        case OP_NE: {
            const Value b = vm_pop(vm);
            const Value a = vm_pop(vm);
            vm_push(vm, value_bool(!value_equal(a, b)));
        } break;

        case OP_GDEF:
            table_set(
                &vm->globals, &vm->gc, (ObjectStr *)vm_read_const(vm)->as.object, vm_peek(vm, 0));
            vm_pop(vm);
            break;

        case OP_GGET: {
            ObjectStr *name = (ObjectStr *)vm_read_const(vm)->as.object;

            Value value;
            if (!table_get(&vm->globals, name, &value)) {
                fprintf(stderr, "error: undefined variable '" SVFmt "'\n", SVArg(*name));
                return false;
            }

            vm_push(vm, value);
        } break;

        case OP_GSET: {
            ObjectStr *name = (ObjectStr *)vm_read_const(vm)->as.object;

            if (table_set(&vm->globals, &vm->gc, name, vm_peek(vm, 0))) {
                table_remove(&vm->globals, name);
                fprintf(stderr, "error: undefined variable '" SVFmt "'\n", SVArg(*name));
                return false;
            }
        } break;

        case OP_LGET:
            vm_push(vm, vm->stack.data[vm->frame->base + vm_read_int(vm)]);
            break;

        case OP_LSET:
            vm->stack.data[vm->frame->base + vm_read_int(vm)] = vm_peek(vm, 0);
            break;

        case OP_UGET: {
            ObjectUpvalue *upvalue = vm->frame->closure->data[vm_read_int(vm)];
            if (upvalue->closed) {
                vm_push(vm, upvalue->value);
            } else {
                vm_push(vm, vm->stack.data[upvalue->index]);
            }
        } break;

        case OP_USET: {
            const Value value = vm_peek(vm, 0);
            ObjectUpvalue *upvalue = vm->frame->closure->data[vm_read_int(vm)];
            if (upvalue->closed) {
                upvalue->value = value;
            } else {
                vm->stack.data[upvalue->index] = value;
            }
        } break;

        case OP_JUMP:
            vm->frame->ip += vm_read_int(vm);
            break;

        case OP_ELSE: {
            const size_t offset = vm_read_int(vm);
            if (value_is_falsey(vm_peek(vm, 0))) {
                vm->frame->ip += offset;
            }
        } break;

        case OP_THEN: {
            const size_t offset = vm_read_int(vm);
            if (!value_is_falsey(vm_peek(vm, 0))) {
                vm->frame->ip += offset;
            }
        } break;

        case OP_PRINT:
            value_print(vm_pop(vm), stdout);
            printf("\n");
            break;

        default:
            fprintf(
                stderr,
                "error: invalid op %d at offset %zu\n",
                op,
                vm->frame->ip - vm->frame->closure->fn->chunk.data);
            return false;
        }
    }
}

void vm_trace(VM *vm) {
    for (size_t i = vm->frames.count; i > 0; i--) {
        const Frame *frame = &vm->frames.data[i - 1];
        const ObjectFn *fn = frame->closure->fn;
        value_print(value_object(fn), stderr);
        fprintf(stderr, "\n");
    }

    vm->stack.count = 0;
    vm->frames.count = 0;
}
