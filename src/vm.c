#include <stdio.h>

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
    const size_t index = *(size_t *)vm->ip;
    vm->ip += sizeof(index);
    return index;
}

static Value *vm_read_const(VM *vm) {
    return &vm->chunk->constants.data[vm_read_int(vm)];
}

static void gc_free_object(GC *gc, Object *object) {
    switch (object->type) {
    case OBJECT_STR:
        gc_realloc(gc, object, sizeof(ObjectStr) + ((ObjectStr *)object)->size, 0);
        break;
    }
}

void vm_free(VM *vm) {
    values_free(&vm->stack);
    table_free(&vm->globals, &vm->gc);

    Object *object = vm->gc.objects;
    while (object) {
        Object *next = object->next;
        gc_free_object(&vm->gc, object);
        object = next;
    }
}

static_assert(COUNT_OPS == 16, "Update vm_run()");
bool vm_run(VM *vm, Chunk *chunk) {
    vm->chunk = chunk;
    vm->ip = vm->chunk->data;

    while (true) {
        const Op op = *vm->ip++;
        switch (op) {
        case OP_HALT:
            return true;

        case OP_DROP:
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
            const Value b = vm_pop(vm);
            const Value a = vm_pop(vm);

            if (a.type != VALUE_NUM || b.type != VALUE_NUM) {
                fprintf(
                    stderr,
                    "error: invalid operands to binary (+): %s, %s\n",
                    value_type_name(a.type),
                    value_type_name(b.type));
                return false;
            }

            vm_push(vm, value_num(a.as.number + b.as.number));
        } break;

        case OP_SUB: {
            const Value b = vm_pop(vm);
            const Value a = vm_pop(vm);

            if (a.type != VALUE_NUM || b.type != VALUE_NUM) {
                fprintf(
                    stderr,
                    "error: invalid operands to binary (-): %s, %s\n",
                    value_type_name(a.type),
                    value_type_name(b.type));
                return false;
            }

            vm_push(vm, value_num(a.as.number - b.as.number));
        } break;

        case OP_MUL: {
            const Value b = vm_pop(vm);
            const Value a = vm_pop(vm);

            if (a.type != VALUE_NUM || b.type != VALUE_NUM) {
                fprintf(
                    stderr,
                    "error: invalid operands to binary (*): %s, %s\n",
                    value_type_name(a.type),
                    value_type_name(b.type));
                return false;
            }

            vm_push(vm, value_num(a.as.number * b.as.number));
        } break;

        case OP_DIV: {
            const Value b = vm_pop(vm);
            const Value a = vm_pop(vm);

            if (a.type != VALUE_NUM || b.type != VALUE_NUM) {
                fprintf(
                    stderr,
                    "error: invalid operands to binary (/): %s, %s\n",
                    value_type_name(a.type),
                    value_type_name(b.type));
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

            if (table_set(&vm->globals, &vm->gc, name, vm_pop(vm))) {
                table_remove(&vm->globals, name);
                fprintf(stderr, "error: undefined variable '" SVFmt "'\n", SVArg(*name));
                return false;
            }
        } break;

        case OP_PRINT:
            value_print(vm_pop(vm));
            putchar('\n');
            break;

        default:
            fprintf(stderr, "error: invalid op %d\n", op);
            return false;
        }
    }
}
