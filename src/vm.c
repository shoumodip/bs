#include "bs.h"
#include "debug.h"

// #define GC_DEBUG_LOG
// #define GC_DEBUG_STRESS
#define GC_GROW_FACTOR 2

Object *object_new(Vm *vm, ObjectType type, size_t size) {
    Object *object = vm_realloc(vm, NULL, 0, size);
    object->type = type;
    object->next = vm->objects;
    object->marked = false;
    vm->objects = object;

#ifdef GC_DEBUG_LOG
    printf("[GC] Allocate %p (%zu bytes); Type: %d\n", object, size, type);
#endif // GC_DEBUG_LOG

    return object;
}

static_assert(COUNT_OBJECTS == 5, "Update object_free()");
static void object_free(Vm *vm, Object *object) {

#ifdef GC_DEBUG_LOG
    printf("[GC] Free %p; Type: %d\n", object, object->type);
#endif // GC_DEBUG_LOG

    switch (object->type) {
    case OBJECT_FN: {
        ObjectFn *fn = (ObjectFn *)object;
        chunk_free(vm, &fn->chunk);
        vm_realloc(vm, fn, sizeof(*fn), 0);
    } break;

    case OBJECT_STR: {
        ObjectStr *str = (ObjectStr *)object;
        vm_realloc(vm, str, sizeof(*str) + str->size, 0);
    } break;

    case OBJECT_ARRAY: {
        ObjectArray *array = (ObjectArray *)object;
        vm_realloc(vm, array->data, sizeof(*array->data) * array->capacity, 0);
        vm_realloc(vm, array, sizeof(*array), 0);
    } break;

    case OBJECT_CLOSURE: {
        ObjectClosure *closure = (ObjectClosure *)object;
        vm_realloc(vm, closure, sizeof(*closure) + sizeof(ObjectUpvalue *) * closure->upvalues, 0);
    } break;

    case OBJECT_UPVALUE:
        vm_realloc(vm, object, sizeof(ObjectUpvalue), 0);
        break;

    default:
        assert(false && "unreachable");
    }
}

static_assert(COUNT_OBJECTS == 5, "Update object_mark()");
static void object_mark(Vm *vm, Object *object) {
    if (!object || object->marked) {
        return;
    }
    object->marked = true;

#ifdef GC_DEBUG_LOG
    printf("[GC] Mark %p ", object);
    value_print(value_object(object), stdout);
    printf("\n");
#endif // GC_DEBUG_LOG

    switch (object->type) {
    case OBJECT_FN: {
        ObjectFn *fn = (ObjectFn *)object;
        object_mark(vm, (Object *)fn->name);

        for (size_t i = 0; i < fn->chunk.constants.count; i++) {
            const Value value = fn->chunk.constants.data[i];
            if (value.type == VALUE_OBJECT) {
                object_mark(vm, value.as.object);
            }
        }
    } break;

    case OBJECT_STR:
        break;

    case OBJECT_ARRAY: {
        ObjectArray *array = (ObjectArray *)object;
        for (size_t i = 0; i < array->count; i++) {
            const Value value = array->data[i];
            if (value.type == VALUE_OBJECT) {
                object_mark(vm, value.as.object);
            }
        }
    } break;

    case OBJECT_CLOSURE: {
        ObjectClosure *closure = (ObjectClosure *)object;
        object_mark(vm, (Object *)closure->fn);

        for (size_t i = 0; i < closure->upvalues; i++) {
            object_mark(vm, (Object *)closure->data[i]);
        }
    } break;

    case OBJECT_UPVALUE: {
        ObjectUpvalue *upvalue = (ObjectUpvalue *)object;
        if (upvalue->closed) {
            if (upvalue->value.type == VALUE_OBJECT) {
                object_mark(vm, upvalue->value.as.object);
            }
        }
    } break;

    default:
        assert(false && "unreachable");
    }
}

static void vm_collect(Vm *vm) {
#ifdef GC_DEBUG_LOG
    printf("\n-------- GC Begin --------\n");
    const size_t before = vm->allocated;
#endif // GC_DEBUG_LOG

    // Mark
    for (size_t i = 0; i < vm->stack.count; i++) {
        const Value value = vm->stack.data[i];
        if (value.type == VALUE_OBJECT) {
            object_mark(vm, value.as.object);
        }
    }

    for (size_t i = 0; i < vm->frames.count; i++) {
        object_mark(vm, (Object *)vm->frames.data[i].closure);
    }

    for (ObjectUpvalue *upvalue = vm->upvalues; upvalue; upvalue = upvalue->next) {
        object_mark(vm, (Object *)upvalue);
    }

    for (Scope *scope = vm->compiler->scope; scope; scope = scope->outer) {
        object_mark(vm, (Object *)scope->fn);
    }

    for (size_t i = 0; i < vm->globals.capacity; i++) {
        Entry *entry = &vm->globals.data[i];
        object_mark(vm, (Object *)entry->key);

        if (entry->value.type == VALUE_OBJECT) {
            object_mark(vm, entry->value.as.object);
        }
    }

    // Sweep
    Object *previous = NULL;
    Object *object = vm->objects;

    while (object) {
        if (object->marked) {
            object->marked = false;
            previous = object;
            object = object->next;
        } else {
            Object *lost = object;
            object = object->next;
            if (previous) {
                previous->next = object;
            } else {
                vm->objects = object;
            }

            object_free(vm, lost);
        }
    }

    vm->gc_max = max(vm->gc_max, vm->allocated * GC_GROW_FACTOR);

#ifdef GC_DEBUG_LOG
    if (before != vm->allocated) {
        printf(
            "\n[GC] Collected %zu bytes (%zu -> %zu); Next run at %zu bytes\n",
            before - vm->allocated,
            before,
            vm->allocated,
            vm->gc);
    }

    printf("-------- GC End ----------\n\n");
#endif // GC_DEBUG_LOG
}

void vm_free(Vm *vm) {
    Object *object = vm->objects;
    while (object) {
        Object *next = object->next;
        object_free(vm, object);
        object = next;
    }

    values_free(&vm->stack);
    frames_free(&vm->frames);
    table_free(vm, &vm->globals);
}

void *vm_realloc(Vm *vm, void *ptr, size_t old_size, size_t new_size) {
    vm->allocated += new_size - old_size;

    if (new_size > old_size) {
#ifdef GC_DEBUG_STRESS
        vm_collect(vm);
#endif // GC_DEBUG_STRESS

        if (vm->allocated > vm->gc_max) {
            vm_collect(vm);
        }
    }

    if (!new_size) {
        free(ptr);
        return NULL;
    }

    return realloc(ptr, new_size);
}

void vm_trace(Vm *vm, FILE *file) {
    for (size_t i = vm->frames.count; i > 0; i--) {
        const Frame *frame = &vm->frames.data[i - 1];
        const ObjectFn *fn = frame->closure->fn;
        value_print(value_object(fn), file);
        fprintf(file, "\n");
    }

    vm->stack.count = 0;
    vm->frames.count = 0;
}

static void vm_push(Vm *vm, Value value) {
    values_push(&vm->stack, value);
}

static Value vm_pop(Vm *vm) {
    return vm->stack.data[--vm->stack.count];
}

static Value vm_peek(Vm *vm, size_t offset) {
    return vm->stack.data[vm->stack.count - offset - 1];
}

static size_t vm_read_int(Vm *vm) {
    const size_t index = *(size_t *)vm->frame->ip;
    vm->frame->ip += sizeof(index);
    return index;
}

static Value *vm_read_const(Vm *vm) {
    return &vm->frame->closure->fn->chunk.constants.data[vm_read_int(vm)];
}

static bool vm_binary_op(Vm *vm, Value *a, Value *b, const char *op) {
    *b = vm_pop(vm);
    *a = vm_pop(vm);

    if (a->type != VALUE_NUM || b->type != VALUE_NUM) {
        fprintf(
            stderr,
            "error: invalid operands to binary (%s): %s, %s\n",
            op,
            value_type_name(*a),
            value_type_name(*b));
        return false;
    }

    return true;
}

static bool vm_call(Vm *vm, ObjectClosure *closure, size_t arity) {
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

static ObjectUpvalue *vm_capture_upvalue(Vm *vm, size_t index) {
    ObjectUpvalue *previous = NULL;
    ObjectUpvalue *upvalue = vm->upvalues;
    while (upvalue && upvalue->index > index) {
        previous = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue && upvalue->index == index) {
        return upvalue;
    }

    ObjectUpvalue *created = object_upvalue_new(vm, index);
    created->next = upvalue;

    if (previous) {
        previous->next = created;
    } else {
        vm->upvalues = created;
    }

    return created;
}

static void vm_close_upvalues(Vm *vm, size_t index) {
    while (vm->upvalues && vm->upvalues->index >= index) {
        ObjectUpvalue *upvalue = vm->upvalues;
        upvalue->closed = true;
        upvalue->value = vm->stack.data[upvalue->index];
        vm->upvalues = upvalue->next;
    }
}

static_assert(COUNT_OPS == 35, "Update vm_interpret()");
bool vm_interpret(Vm *vm, const ObjectFn *fn, bool debug) {
    if (debug) {
        debug_chunks(vm->objects);
    }

    vm_push(vm, value_object(fn));
    ObjectClosure *closure = object_closure_new(vm, fn);
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
                fprintf(stderr, "error: cannot call %s value\n", value_type_name(value));
                return false;
            }

            static_assert(COUNT_OBJECTS == 5, "Update vm_interpret()");
            switch (value.as.object->type) {
            case OBJECT_CLOSURE:
                if (!vm_call(vm, (ObjectClosure *)value.as.object, arity)) {
                    return false;
                }
                break;

            default:
                fprintf(stderr, "error: cannot call %s value\n", value_type_name(value));
                return false;
            }

            vm->frame = &vm->frames.data[vm->frames.count - 1];
        } break;

        case OP_CLOSURE: {
            ObjectFn *fn = (ObjectFn *)vm_read_const(vm)->as.object;
            ObjectClosure *closure = object_closure_new(vm, fn);
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

        case OP_ARRAY:
            vm_push(vm, value_object(object_array_new(vm)));
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
                fprintf(stderr, "error: invalid operand to unary (-): %s\n", value_type_name(a));
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
            table_set(vm, &vm->globals, (ObjectStr *)vm_read_const(vm)->as.object, vm_peek(vm, 0));
            vm_pop(vm);
            break;

        case OP_GGET: {
            ObjectStr *name = (ObjectStr *)vm_read_const(vm)->as.object;

            Value value;
            if (!table_get(vm, &vm->globals, name, &value)) {
                fprintf(stderr, "error: undefined variable '" SVFmt "'\n", SVArg(*name));
                return false;
            }

            vm_push(vm, value);
        } break;

        case OP_GSET: {
            ObjectStr *name = (ObjectStr *)vm_read_const(vm)->as.object;

            if (table_set(vm, &vm->globals, name, vm_peek(vm, 0))) {
                table_remove(vm, &vm->globals, name);
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

        case OP_AGET: {
            const Value index = vm_peek(vm, 0);
            if (index.type != VALUE_NUM) {
                fprintf(stderr, "error: cannot index with %s value\n", value_type_name(index));
                return false;
            }

            if (index.as.number != (long)index.as.number) {
                fprintf(stderr, "error: cannot index with fractional value\n");
                return false;
            }

            const Value array = vm_peek(vm, 1);
            if (array.type != VALUE_OBJECT || array.as.object->type != OBJECT_ARRAY) {
                fprintf(stderr, "error: cannot index into %s value\n", value_type_name(array));
                return false;
            }

            Value value;
            if (!object_array_get(vm, (ObjectArray *)array.as.object, index.as.number, &value)) {
                fprintf(
                    stderr,
                    "error: cannot get value at index %zu in array of length %zu\n",
                    (size_t)index.as.number,
                    ((ObjectArray *)array.as.object)->count);
                return false;
            }

            vm->stack.count -= 2;
            vm_push(vm, value);
        } break;

        case OP_ASET: {
            const Value value = vm_peek(vm, 0);
            const Value index = vm_peek(vm, 1);
            if (index.type != VALUE_NUM) {
                fprintf(stderr, "error: cannot index with %s value\n", value_type_name(index));
                return false;
            }

            if (index.as.number != (long)index.as.number) {
                fprintf(stderr, "error: cannot index with fractional value\n");
                return false;
            }

            const Value array = vm_peek(vm, 2);
            if (array.type != VALUE_OBJECT || array.as.object->type != OBJECT_ARRAY) {
                fprintf(stderr, "error: cannot index into %s value\n", value_type_name(array));
                return false;
            }

            object_array_set(vm, (ObjectArray *)array.as.object, index.as.number, value);
            vm->stack.count -= 2;
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
