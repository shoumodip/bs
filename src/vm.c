#include <stdarg.h>

#include "debug.h"
#include "hash.h"

// #define GC_DEBUG_LOG
// #define GC_DEBUG_STRESS
#define GC_GROW_FACTOR 2

typedef struct {
    size_t base;
    ObjectClosure *closure;

    const uint8_t *ip;
} Frame;

typedef struct {
    Frame *data;
    size_t count;
    size_t capacity;
} Frames;

#define frames_free da_free
#define frames_push da_push

typedef struct {
    Writer meta;

    Vm *vm;
    char *data;
    size_t count;
    size_t capacity;
} WriterStr;

#define writer_str_free da_free

static void vm_writer_str_fmt(Writer *w, const char *fmt, ...) {
    WriterStr *w1 = (WriterStr *)w;

    va_list args;
    va_start(args, fmt);
    int count = vsnprintf(NULL, 0, fmt, args);
    assert(count >= 0);
    va_end(args);

    da_push_many(w1->vm, w1, NULL, count + 1);
    va_start(args, fmt);
    vsnprintf(w1->data + w1->count, count + 1, fmt, args);
    va_end(args);

    w1->count += count;
}

typedef struct {
    Writer meta;
    FILE *file;
} WriterFile;

static void vm_writer_file_fmt(Writer *w, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(((WriterFile *)w)->file, fmt, args);
    va_end(args);
}

struct Vm {
    Values stack;

    Frame *frame;
    Frames frames;

    ObjectTable globals;
    ObjectTable strings;
    ObjectUpvalue *upvalues;

    bool gc_on;
    size_t gc_max;
    size_t gc_bytes;
    Object *objects;

    WriterStr writer_str;
    WriterFile writer_stdout;
    WriterFile writer_stderr;
};

static_assert(COUNT_OBJECTS == 6, "Update object_free()");
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

    case OBJECT_TABLE: {
        object_table_free(vm, (ObjectTable *)object);
        vm_realloc(vm, object, sizeof(ObjectTable), 0);
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

static_assert(COUNT_OBJECTS == 6, "Update object_mark()");
static void object_mark(Vm *vm, Object *object) {
    if (!object || object->marked) {
        return;
    }
    object->marked = true;

#ifdef GC_DEBUG_LOG
    printf("[GC] Mark %p ", object);
    value_write(value_object(object), (Writer *)&vm->writer_stdout);
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

    case OBJECT_TABLE: {
        ObjectTable *table = (ObjectTable *)object;

        for (size_t i = 0; i < table->capacity; i++) {
            Entry *entry = &table->data[i];
            object_mark(vm, (Object *)entry->key);

            if (entry->value.type == VALUE_OBJECT) {
                object_mark(vm, entry->value.as.object);
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
    const size_t before = vm->gc_bytes;
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

    vm->globals.meta.marked = false;
    object_mark(vm, (Object *)&vm->globals);

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

    vm->gc_max = max(vm->gc_max, vm->gc_bytes * GC_GROW_FACTOR);

#ifdef GC_DEBUG_LOG
    if (before != vm->gc_bytes) {
        printf(
            "\n[GC] Collected %zu bytes (%zu -> %zu); Next run at %zu bytes\n",
            before - vm->gc_bytes,
            before,
            vm->gc_bytes,
            vm->gc_max);
    }

    printf("-------- GC End ----------\n\n");
#endif // GC_DEBUG_LOG
}

Vm *vm_new(void) {
    Vm *vm = calloc(1, sizeof(Vm));
    vm->gc_max = 1024 * 1024;
    vm->globals.meta.type = OBJECT_TABLE;

    vm->writer_str = (WriterStr){.meta.fmt = vm_writer_str_fmt, .vm = vm};
    vm->writer_stdout = (WriterFile){.meta.fmt = vm_writer_file_fmt, .file = stdout};
    vm->writer_stderr = (WriterFile){.meta.fmt = vm_writer_file_fmt, .file = stderr};
    return vm;
}

void vm_free(Vm *vm) {
    Object *object = vm->objects;
    while (object) {
        Object *next = object->next;
        object_free(vm, object);
        object = next;
    }

    free(vm->stack.data);
    memset(&vm->stack, 0, sizeof(vm->stack));

    frames_free(vm, &vm->frames);
    writer_str_free(vm, &vm->writer_str);

    object_table_free(vm, &vm->globals);
    object_table_free(vm, &vm->strings);

    free(vm);
}

void *vm_realloc(Vm *vm, void *ptr, size_t old_size, size_t new_size) {
    vm->gc_bytes += new_size - old_size;

    if (vm->gc_on && new_size > old_size) {
#ifdef GC_DEBUG_STRESS
        vm_collect(vm);
#else
        if (vm->gc_bytes > vm->gc_max) {
            vm_collect(vm);
        }
#endif // GC_DEBUG_STRESS
    }

    if (!new_size) {
        free(ptr);
        return NULL;
    }

    return realloc(ptr, new_size);
}

static void vm_push(Vm *vm, Value value) {
    // Do not trigger garbage collection in the stack
    if (vm->stack.count >= vm->stack.capacity) {
        vm->stack.capacity = vm->stack.capacity ? vm->stack.capacity * 2 : DA_INIT_CAP;
        vm->stack.data = realloc(vm->stack.data, vm->stack.capacity * sizeof(*vm->stack.data));
        assert(vm->stack.data);
    }

    vm->stack.data[vm->stack.count++] = value;
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

static Loc chunk_get_loc(const Chunk *c, size_t op_index) {
    for (size_t i = 0; i < c->locations.count; i++) {
        if (c->locations.data[i].index == op_index) {
            return c->locations.data[i].loc;
        }
    }

    assert(false && "unreachable");
}

static void vm_runtime_error(Vm *vm, size_t op_index, const char *fmt, ...) {
    fflush(stdout);

    Loc loc = chunk_get_loc(&vm->frame->closure->fn->chunk, op_index);
    fprintf(stderr, LocFmt "error: ", LocArg(loc));

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    for (size_t i = vm->frames.count; i > 1; i--) {
        const Frame *frame = &vm->frames.data[i - 2];
        const ObjectFn *fn = frame->closure->fn;

        loc = chunk_get_loc(&fn->chunk, frame->ip - fn->chunk.data - 1 - sizeof(size_t));
        fprintf(stderr, LocFmt "in ", LocArg(loc));
        value_write(value_object(fn), (Writer *)&vm->writer_stderr);
        fprintf(stderr, "\n");
    }
}

static bool vm_binary_op(Vm *vm, Value *a, Value *b, const char *op, size_t op_index) {
    *b = vm_pop(vm);
    *a = vm_pop(vm);

    if (a->type != VALUE_NUM || b->type != VALUE_NUM) {
        vm_runtime_error(
            vm,
            op_index,
            "invalid operands to binary (%s): %s, %s\n",
            op,
            value_type_name(*a),
            value_type_name(*b));

        return false;
    }

    return true;
}

static bool vm_call(Vm *vm, ObjectClosure *closure, size_t arity, size_t op_index) {
    if (arity != closure->fn->arity) {
        vm_runtime_error(
            vm, op_index, "expected %zu arguments, got %zu\n", closure->fn->arity, arity);
        return false;
    }

    const Frame frame = {
        .closure = closure,
        .ip = closure->fn->chunk.data,
        .base = vm->stack.count - arity - 1,
    };

    frames_push(vm, &vm->frames, frame);
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

static_assert(COUNT_OPS == 38, "Update vm_interpret()");
bool vm_interpret(Vm *vm, const ObjectFn *fn, bool debug) {
    bool result = true;

    if (debug) {
        debug_chunks((Writer *)&vm->writer_stdout, vm->objects);
    }

    ObjectClosure *closure = object_closure_new(vm, fn);
    vm_push(vm, value_object(closure));
    vm_call(vm, closure, 0, 0);

    vm->gc_on = true;
    while (true) {
        if (debug) {
            printf("----------------------------------------\n");
            printf("Stack:\n");
            for (size_t i = 0; i < vm->stack.count; i++) {
                printf("    ");
                value_write(vm->stack.data[i], (Writer *)&vm->writer_stdout);
                printf("\n");
            }
            printf("\n");

            size_t offset = vm->frame->ip - vm->frame->closure->fn->chunk.data;
            debug_op((Writer *)&vm->writer_stdout, &vm->frame->closure->fn->chunk, &offset);
            printf("----------------------------------------\n");
            getchar();
        }

        const size_t op_index = vm->frame->ip - vm->frame->closure->fn->chunk.data;
        const Op op = *vm->frame->ip++;
        switch (op) {
        case OP_RET: {
            const Value value = vm_pop(vm);
            vm_close_upvalues(vm, vm->frame->base);
            vm->frames.count--;

            if (!vm->frames.count) {
                return_defer(true);
            }

            vm->stack.count = vm->frame->base;
            vm_push(vm, value);

            vm->frame = &vm->frames.data[vm->frames.count - 1];
        } break;

        case OP_CALL: {
            const size_t arity = vm_read_int(vm);
            const Value value = vm_peek(vm, arity);

            if (value.type != VALUE_OBJECT) {
                vm_runtime_error(vm, op_index, "cannot call %s value\n", value_type_name(value));
                return_defer(false);
            }

            static_assert(COUNT_OBJECTS == 6, "Update vm_interpret()");
            switch (value.as.object->type) {
            case OBJECT_CLOSURE:
                if (!vm_call(vm, (ObjectClosure *)value.as.object, arity, op_index)) {
                    return_defer(false);
                }
                break;

            default:
                vm_runtime_error(vm, op_index, "cannot call %s value\n", value_type_name(value));
                return_defer(false);
            }
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

        case OP_TABLE:
            vm_push(vm, value_object(object_table_new(vm)));
            break;

        case OP_CONST:
            vm_push(vm, *vm_read_const(vm));
            break;

        case OP_ADD: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, "+", op_index)) {
                return_defer(false);
            }

            vm_push(vm, value_num(a.as.number + b.as.number));
        } break;

        case OP_SUB: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, "-", op_index)) {
                return_defer(false);
            }

            vm_push(vm, value_num(a.as.number - b.as.number));
        } break;

        case OP_MUL: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, "*", op_index)) {
                return_defer(false);
            }

            vm_push(vm, value_num(a.as.number * b.as.number));
        } break;

        case OP_DIV: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, "/", op_index)) {
                return_defer(false);
            }

            vm_push(vm, value_num(a.as.number / b.as.number));
        } break;

        case OP_NEG: {
            const Value a = vm_pop(vm);
            if (a.type != VALUE_NUM) {
                vm_runtime_error(
                    vm, op_index, "invalid operand to unary (-): %s\n", value_type_name(a));
                return_defer(false);
            }

            vm_push(vm, value_num(-a.as.number));
        } break;

        case OP_NOT:
            vm_push(vm, value_bool(value_is_falsey(vm_pop(vm))));
            break;

        case OP_GT: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, ">", op_index)) {
                return_defer(false);
            }

            vm_push(vm, value_bool(a.as.number > b.as.number));
        } break;

        case OP_GE: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, ">=", op_index)) {
                return_defer(false);
            }

            vm_push(vm, value_bool(a.as.number >= b.as.number));
        } break;

        case OP_LT: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, "<", op_index)) {
                return_defer(false);
            }

            vm_push(vm, value_bool(a.as.number < b.as.number));
        } break;

        case OP_LE: {
            Value a, b;
            if (!vm_binary_op(vm, &a, &b, "<=", op_index)) {
                return_defer(false);
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

        case OP_LEN: {
            const Value a = vm_peek(vm, 0);
            if (a.type != VALUE_OBJECT) {
                vm_runtime_error(
                    vm, op_index, "cannot get length of %s value\n", value_type_name(a));
                return_defer(false);
            }

            size_t size;
            switch (a.as.object->type) {
            case OBJECT_STR:
                size = ((ObjectStr *)a.as.object)->size;
                break;

            case OBJECT_ARRAY:
                size = ((ObjectArray *)a.as.object)->count;
                break;

            case OBJECT_TABLE:
                size = ((ObjectTable *)a.as.object)->real_count;
                break;

            default:
                vm_runtime_error(
                    vm, op_index, "cannot get length of %s value\n", value_type_name(a));
                return_defer(false);
            }

            vm_pop(vm);
            vm_push(vm, value_num(size));
        } break;

        case OP_JOIN: {
            const bool gc_on = vm->gc_on;
            const size_t start = vm->writer_str.count;

            vm->gc_on = false;

            const Value b = vm_pop(vm);
            const Value a = vm_pop(vm);
            value_write(a, (Writer *)&vm->writer_str);
            value_write(b, (Writer *)&vm->writer_str);

            vm_push(
                vm,
                value_object(
                    object_str_new(vm, vm->writer_str.data + start, vm->writer_str.count - start)));

            vm->writer_str.count = start;
            vm->gc_on = gc_on;
        } break;

        case OP_GDEF:
            object_table_set(
                vm, &vm->globals, (ObjectStr *)vm_read_const(vm)->as.object, vm_peek(vm, 0));

            vm_pop(vm);
            break;

        case OP_GGET: {
            ObjectStr *name = (ObjectStr *)vm_read_const(vm)->as.object;

            Value value;
            if (!object_table_get(vm, &vm->globals, name, &value)) {
                vm_runtime_error(vm, op_index, "undefined variable '" SVFmt "'\n", SVArg(*name));
                return_defer(false);
            }

            vm_push(vm, value);
        } break;

        case OP_GSET: {
            ObjectStr *name = (ObjectStr *)vm_read_const(vm)->as.object;

            if (object_table_set(vm, &vm->globals, name, vm_peek(vm, 0))) {
                object_table_remove(vm, &vm->globals, name);
                vm_runtime_error(vm, op_index, "undefined variable '" SVFmt "'\n", SVArg(*name));
                return_defer(false);
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

        case OP_IGET: {
            Value value;

            const Value container = vm_peek(vm, 1);
            if (container.type != VALUE_OBJECT) {
                vm_runtime_error(
                    vm, op_index, "cannot index into %s value\n", value_type_name(container));
                return_defer(false);
            }

            const Value index = vm_peek(vm, 0);
            if (container.as.object->type == OBJECT_ARRAY) {
                if (index.type != VALUE_NUM) {
                    vm_runtime_error(
                        vm, op_index, "cannot index array with %s value\n", value_type_name(index));

                    return_defer(false);
                }

                if (index.as.number != (long)index.as.number) {
                    vm_runtime_error(vm, op_index, "cannot index array with fractional value\n");
                    return_defer(false);
                }

                if (!object_array_get(
                        vm, (ObjectArray *)container.as.object, index.as.number, &value)) {
                    vm_runtime_error(
                        vm,
                        op_index,
                        "cannot get value at index %zu in array of length %zu\n",
                        (size_t)index.as.number,
                        ((ObjectArray *)container.as.object)->count);

                    return_defer(false);
                }
            } else if (container.as.object->type == OBJECT_TABLE) {
                if (index.type != VALUE_OBJECT || index.as.object->type != OBJECT_STR) {
                    vm_runtime_error(
                        vm, op_index, "cannot index table with %s value\n", value_type_name(index));

                    return_defer(false);
                }

                ObjectStr *key = (ObjectStr *)index.as.object;
                if (!object_table_get(vm, (ObjectTable *)container.as.object, key, &value)) {
                    value = value_nil;
                }
            } else {
                vm_runtime_error(
                    vm, op_index, "cannot index into %s value\n", value_type_name(container));
                return_defer(false);
            }

            vm->stack.count -= 2;
            vm_push(vm, value);
        } break;

        case OP_ISET: {
            const Value container = vm_peek(vm, 2);
            if (container.type != VALUE_OBJECT) {
                vm_runtime_error(
                    vm, op_index, "cannot index into %s value\n", value_type_name(container));
                return_defer(false);
            }

            const Value value = vm_peek(vm, 0);
            const Value index = vm_peek(vm, 1);

            if (container.as.object->type == OBJECT_ARRAY) {
                if (index.type != VALUE_NUM) {
                    vm_runtime_error(
                        vm, op_index, "cannot index with %s value\n", value_type_name(index));
                    return_defer(false);
                }

                if (index.as.number != (long)index.as.number) {
                    vm_runtime_error(vm, op_index, "cannot index with fractional value\n");
                    return_defer(false);
                }

                object_array_set(vm, (ObjectArray *)container.as.object, index.as.number, value);
            } else if (container.as.object->type == OBJECT_TABLE) {
                if (index.type != VALUE_OBJECT || index.as.object->type != OBJECT_STR) {
                    vm_runtime_error(
                        vm, op_index, "cannot index table with %s value\n", value_type_name(index));

                    return_defer(false);
                }

                if (value.type == VALUE_NIL) {
                    object_table_remove(
                        vm, (ObjectTable *)container.as.object, (ObjectStr *)index.as.object);
                } else {
                    object_table_set(
                        vm,
                        (ObjectTable *)container.as.object,
                        (ObjectStr *)index.as.object,
                        value);
                }
            } else {
                vm_runtime_error(
                    vm, op_index, "cannot index into %s value\n", value_type_name(container));
                return_defer(false);
            }

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
            value_write(vm_pop(vm), (Writer *)&vm->writer_stdout);
            printf("\n");
            break;

        default:
            fprintf(stderr, "invalid op %d at offset %zu\n", op, op_index);
            return_defer(false);
        }
    }

defer:
    vm->stack.count = 0;

    vm->frame = NULL;
    vm->frames.count = 0;

    vm->upvalues = NULL;
    vm->gc_on = false;
    return result;
}

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

ObjectStr *object_str_const(Vm *vm, const char *data, size_t size) {
    Entry *entry = entries_find_sv(vm->strings.data, vm->strings.capacity, (SV){data, size}, NULL);
    if (entry && entry->key) {
        return entry->key;
    }

    ObjectStr *str = object_str_new(vm, data, size);
    object_table_set(vm, &vm->strings, str, value_nil);

    return str;
}
