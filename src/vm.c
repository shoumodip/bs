#include <stdio.h>

#include "vm.h"

static void vm_push(Vm *vm, Value value) {
    values_push(&vm->stack, value);
}

static Value vm_pop(Vm *vm) {
    return vm->stack.data[--vm->stack.count];
}

static size_t vm_read_int(Vm *vm) {
    const size_t index = *(size_t *)vm->ip;
    vm->ip += sizeof(index);
    return index;
}

void vm_free(Vm *vm) {
    values_free(&vm->stack);
}

static_assert(COUNT_OPS == 11, "Update vm_run()");
static_assert(COUNT_VALUES == 3, "Update vm_run()");
bool vm_run(Vm *vm, Chunk *chunk) {
    vm->chunk = chunk;
    vm->ip = vm->chunk->data;

    while (true) {
        const Op op = *vm->ip++;
        switch (op) {
        case OP_HALT:
            value_print(vm_pop(vm));
            printf("\n");
            return true;

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
            vm_push(vm, vm->chunk->constants.data[vm_read_int(vm)]);
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

        default:
            fprintf(stderr, "error: invalid op %d\n", op);
            return false;
        }
    }
}
