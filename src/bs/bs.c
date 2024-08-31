#include <dlfcn.h>
#include <stdio.h>

#include "bs/compiler.h"
#include "bs/debug.h"
#include "bs/hash.h"

// #define BS_GC_DEBUG_LOG
// #define BS_GC_DEBUG_STRESS
#define BS_GC_GROW_FACTOR 2

typedef struct {
    size_t base;
    Bs_Closure *closure;

    const uint8_t *ip;
} Bs_Frame;

typedef struct {
    Bs_Frame *data;
    size_t count;
    size_t capacity;
} Bs_Frames;

#define bs_frames_free bs_da_free
#define bs_frames_push bs_da_push

typedef struct {
    Bs_Writer meta;

    Bs *bs;
    char *data;
    size_t count;
    size_t capacity;
} Bs_Str_Writer;

#define bs_str_writer_free bs_da_free

static void bs_str_writer_fmt(Bs_Writer *w, const char *fmt, va_list args) {
    Bs_Str_Writer *w1 = (Bs_Str_Writer *)w;

    int count = 0;
    {
        va_list copy;
        va_copy(copy, args);
        count = vsnprintf(NULL, 0, fmt, copy);
        assert(count >= 0);
        va_end(copy);
    }

    bs_da_push_many(w1->bs, w1, NULL, count + 1);
    {
        va_list copy;
        va_copy(copy, args);
        vsnprintf(w1->data + w1->count, count + 1, fmt, copy);
        va_end(copy);
    }
    w1->count += count;
}

typedef struct {
    Bs_Writer meta;
    FILE *file;
} Bs_File_Writer;

static void bs_file_writer_fmt(Bs_Writer *w, const char *fmt, va_list args) {
    va_list copy;
    va_copy(copy, args);
    vfprintf(((Bs_File_Writer *)w)->file, fmt, copy);
    va_end(copy);
}

typedef struct {
    bool done;
    Bs_Str *name;

    Bs_Value result;
} Bs_Module;

typedef struct {
    Bs_Module *data;
    size_t count;
    size_t capacity;
} Bs_Modules;

#define bs_modules_free bs_da_free

static bool bs_modules_find(Bs_Modules *m, Bs_Str *name, size_t *index) {
    for (size_t i = 0; i < m->count; i++) {
        if (bs_str_eq(m->data[i].name, name)) {
            *index = i;
            return true;
        }
    }

    return false;
}

struct Bs {
    Bs_Values stack;

    Bs_Frame *frame;
    Bs_Frames frames;

    Bs_Modules modules;

    Bs_Table core;
    Bs_Table globals;
    Bs_Table strings;
    Bs_Upvalue *upvalues;

    bool gc_on;
    size_t gc_max;
    size_t gc_bytes;
    Bs_Object *objects;

    Bs_Str_Writer str_writer;
    Bs_Str_Writer paths_writer;
    Bs_File_Writer stdout_writer;
    Bs_File_Writer stderr_writer;

    size_t op_index;
};

// Garbage collector
static_assert(BS_COUNT_OBJECTS == 9, "Update bs_free_object()");
static void bs_free_object(Bs *bs, Bs_Object *object) {
#ifdef BS_GC_DEBUG_LOG
    bs_write(bs_stdout_writer(bs), "[GC] Free %p; Type: %d\n", object, object->type);
#endif // BS_GC_DEBUG_LOG

    switch (object->type) {
    case BS_OBJECT_FN: {
        Bs_Fn *fn = (Bs_Fn *)object;
        bs_chunk_free(bs, &fn->chunk);
        bs_realloc(bs, fn, sizeof(*fn), 0);
    } break;

    case BS_OBJECT_STR: {
        Bs_Str *str = (Bs_Str *)object;
        bs_realloc(bs, str, sizeof(*str) + str->size, 0);
    } break;

    case BS_OBJECT_ARRAY: {
        Bs_Array *array = (Bs_Array *)object;
        bs_realloc(bs, array->data, sizeof(*array->data) * array->capacity, 0);
        bs_realloc(bs, array, sizeof(*array), 0);
    } break;

    case BS_OBJECT_TABLE: {
        bs_table_free(bs, (Bs_Table *)object);
        bs_realloc(bs, object, sizeof(Bs_Table), 0);
    } break;

    case BS_OBJECT_CLOSURE: {
        Bs_Closure *closure = (Bs_Closure *)object;
        bs_realloc(bs, closure, sizeof(*closure) + sizeof(Bs_Upvalue *) * closure->upvalues, 0);
    } break;

    case BS_OBJECT_UPVALUE:
        bs_realloc(bs, object, sizeof(Bs_Upvalue), 0);
        break;

    case BS_OBJECT_C_FN:
        bs_realloc(bs, object, sizeof(Bs_C_Fn), 0);
        break;

    case BS_OBJECT_C_LIB: {
        Bs_C_Lib *library = (Bs_C_Lib *)object;
        bs_table_free(bs, &library->functions);

        dlclose(library->data);
        bs_realloc(bs, library, sizeof(*library), 0);
    } break;

    case BS_OBJECT_C_DATA: {
        Bs_C_Data *data = (Bs_C_Data *)object;
        if (data->spec->free) {
            data->spec->free(bs, data->data);
        }

        bs_realloc(bs, data, sizeof(*data), 0);
    } break;

    default:
        assert(false && "unreachable");
    }
}

static_assert(BS_COUNT_OBJECTS == 9, "Update bs_mark_object()");
static void bs_mark_object(Bs *bs, Bs_Object *object) {
    if (!object || object->marked) {
        return;
    }
    object->marked = true;

#ifdef BS_GC_DEBUG_LOG
    Bs_Writer *w = bs_stdout_writer(bs);
    bs_write(w, "[GC] Mark %p ", object);
    bs_value_write(w, bs_value_object(object));
    bs_write(w, "\n");
#endif // BS_GC_DEBUG_LOG

    switch (object->type) {
    case BS_OBJECT_FN: {
        Bs_Fn *fn = (Bs_Fn *)object;
        bs_mark_object(bs, (Bs_Object *)fn->name);

        for (size_t i = 0; i < fn->chunk.constants.count; i++) {
            const Bs_Value value = fn->chunk.constants.data[i];
            if (value.type == BS_VALUE_OBJECT) {
                bs_mark_object(bs, value.as.object);
            }
        }
    } break;

    case BS_OBJECT_STR:
        break;

    case BS_OBJECT_ARRAY: {
        Bs_Array *array = (Bs_Array *)object;
        for (size_t i = 0; i < array->count; i++) {
            const Bs_Value value = array->data[i];
            if (value.type == BS_VALUE_OBJECT) {
                bs_mark_object(bs, value.as.object);
            }
        }
    } break;

    case BS_OBJECT_TABLE: {
        Bs_Table *table = (Bs_Table *)object;

        for (size_t i = 0; i < table->capacity; i++) {
            Entry *entry = &table->data[i];
            bs_mark_object(bs, (Bs_Object *)entry->key);

            if (entry->value.type == BS_VALUE_OBJECT) {
                bs_mark_object(bs, entry->value.as.object);
            }
        }
    } break;

    case BS_OBJECT_CLOSURE: {
        Bs_Closure *closure = (Bs_Closure *)object;
        bs_mark_object(bs, (Bs_Object *)closure->fn);

        for (size_t i = 0; i < closure->upvalues; i++) {
            bs_mark_object(bs, (Bs_Object *)closure->data[i]);
        }
    } break;

    case BS_OBJECT_UPVALUE: {
        Bs_Upvalue *upvalue = (Bs_Upvalue *)object;
        if (upvalue->closed) {
            if (upvalue->value.type == BS_VALUE_OBJECT) {
                bs_mark_object(bs, upvalue->value.as.object);
            }
        }
    } break;

    case BS_OBJECT_C_FN:
        bs_mark_object(bs, (Bs_Object *)((Bs_C_Fn *)object)->library);
        break;

    case BS_OBJECT_C_LIB: {
        Bs_C_Lib *library = ((Bs_C_Lib *)object);
        bs_mark_object(bs, (Bs_Object *)library->path);

        library->functions.meta.marked = false;
        bs_mark_object(bs, (Bs_Object *)&library->functions);
    } break;

    case BS_OBJECT_C_DATA:
        break;

    default:
        assert(false && "unreachable");
    }
}

static void bs_collect(Bs *bs) {
#ifdef BS_GC_DEBUG_LOG
    Bs_Writer *w = bs_stdout_writer(bs);
    bs_write(w, "\n-------- GC Begin --------\n");
    const size_t before = bs->gc_bytes;
#endif // BS_GC_DEBUG_LOG

    // Mark
    for (size_t i = 0; i < bs->stack.count; i++) {
        const Bs_Value value = bs->stack.data[i];
        if (value.type == BS_VALUE_OBJECT) {
            bs_mark_object(bs, value.as.object);
        }
    }

    for (size_t i = 0; i < bs->frames.count; i++) {
        bs_mark_object(bs, (Bs_Object *)bs->frames.data[i].closure);
    }

    for (size_t i = 0; i < bs->modules.count; i++) {
        Bs_Module *m = &bs->modules.data[i];

        bs_mark_object(bs, (Bs_Object *)m->name);
        if (m->result.type == BS_VALUE_OBJECT) {
            bs_mark_object(bs, (Bs_Object *)m->result.as.object);
        }
    }

    for (Bs_Upvalue *upvalue = bs->upvalues; upvalue; upvalue = upvalue->next) {
        bs_mark_object(bs, (Bs_Object *)upvalue);
    }

    bs->core.meta.marked = false;
    bs_mark_object(bs, (Bs_Object *)&bs->core);

    bs->globals.meta.marked = false;
    bs_mark_object(bs, (Bs_Object *)&bs->globals);

    // Sweep
    Bs_Object *previous = NULL;
    Bs_Object *object = bs->objects;

    while (object) {
        if (object->marked) {
            object->marked = false;
            previous = object;
            object = object->next;
        } else {
            Bs_Object *lost = object;
            object = object->next;
            if (previous) {
                previous->next = object;
            } else {
                bs->objects = object;
            }

            bs_free_object(bs, lost);
        }
    }

    bs->gc_max = bs_max(bs->gc_max, bs->gc_bytes * BS_GC_GROW_FACTOR);

#ifdef BS_GC_DEBUG_LOG
    if (before != bs->gc_bytes) {
        bs_write(
            w,
            "\n[GC] Collected %zu bytes (%zu -> %zu); Next run at %zu bytes\n",
            before - bs->gc_bytes,
            before,
            bs->gc_bytes,
            bs->gc_max);
    }

    bs_write(w, "-------- GC End ----------\n\n");
#endif // BS_GC_DEBUG_LOG
}

Bs *bs_new(void) {
    Bs *bs = calloc(1, sizeof(Bs));
    bs->gc_max = 1024 * 1024;

    bs->str_writer = (Bs_Str_Writer){.meta.fmt = bs_str_writer_fmt, .bs = bs};
    bs->paths_writer = (Bs_Str_Writer){.meta.fmt = bs_str_writer_fmt, .bs = bs};
    bs->stdout_writer = (Bs_File_Writer){.meta.fmt = bs_file_writer_fmt, .file = stdout};
    bs->stderr_writer = (Bs_File_Writer){.meta.fmt = bs_file_writer_fmt, .file = stderr};

    bs->core.meta.type = BS_OBJECT_TABLE;
    bs->globals.meta.type = BS_OBJECT_TABLE;
    return bs;
}

void bs_free(Bs *bs) {
    free(bs->stack.data);
    memset(&bs->stack, 0, sizeof(bs->stack));

    bs_frames_free(bs, &bs->frames);
    bs_modules_free(bs, &bs->modules);

    bs_table_free(bs, &bs->core);
    bs_table_free(bs, &bs->globals);
    bs_table_free(bs, &bs->strings);

    Bs_Object *object = bs->objects;
    while (object) {
        Bs_Object *next = object->next;
        bs_free_object(bs, object);
        object = next;
    }

    bs_str_writer_free(bs, &bs->str_writer);
    bs_str_writer_free(bs, &bs->paths_writer);
    free(bs);
}

void *bs_realloc(Bs *bs, void *ptr, size_t old_size, size_t new_size) {
    bs->gc_bytes += new_size - old_size;

    if (bs->gc_on && new_size > old_size) {
#ifdef BS_GC_DEBUG_STRESS
        bs_collect(bs);
#else
        if (bs->gc_bytes > bs->gc_max) {
            bs_collect(bs);
        }
#endif // BS_GC_DEBUG_STRESS
    }

    if (!new_size) {
        free(ptr);
        return NULL;
    }

    return realloc(ptr, new_size);
}

// Helpers
void bs_core_set(Bs *bs, Bs_Sv name, Bs_Value value) {
    bs_table_set(bs, &bs->core, bs_object_str_const(bs, name), value);
}

Bs_Writer *bs_str_writer_init(Bs *bs, size_t *start) {
    *start = bs->str_writer.count;
    return (Bs_Writer *)&bs->str_writer;
}

Bs_Sv bs_str_writer_end(Bs *bs, size_t start) {
    const Bs_Sv sv = {bs->str_writer.data + start, bs->str_writer.count - start};
    bs->str_writer.count = start;
    return sv;
}

Bs_Writer *bs_stdout_writer(Bs *bs) {
    return (Bs_Writer *)&bs->stdout_writer;
}

Bs_Writer *bs_stderr_writer(Bs *bs) {
    return (Bs_Writer *)&bs->stderr_writer;
}

Bs_Object *bs_object_new(Bs *bs, Bs_Object_Type type, size_t size) {
    Bs_Object *object = bs_realloc(bs, NULL, 0, size);
    object->type = type;
    object->next = bs->objects;
    object->marked = false;
    bs->objects = object;

#ifdef BS_GC_DEBUG_LOG
    bs_write(bs_stdout_writer(bs), "[GC] Allocate %p (%zu bytes); Type: %d\n", object, size, type);
#endif // BS_GC_DEBUG_LOG

    return object;
}

Bs_Str *bs_object_str_const(Bs *bs, Bs_Sv sv) {
    Entry *entry = bs_entries_find_sv(bs->strings.data, bs->strings.capacity, sv, NULL);
    if (entry && entry->key) {
        return entry->key;
    }

    Bs_Str *str = bs_str_new(bs, sv);
    bs_table_set(bs, &bs->strings, str, bs_value_nil);
    return str;
}

// Errors
static Bs_Loc bs_chunk_get_loc(const Bs_Chunk *c, size_t op_index) {
    for (size_t i = 0; i < c->locations.count; i++) {
        if (c->locations.data[i].index == op_index) {
            return c->locations.data[i].loc;
        }
    }

    assert(false && "unreachable");
}

void bs_error(Bs *bs, const char *fmt, ...) {
    Bs_Writer *w = bs_stderr_writer(bs);

    Bs_Loc loc = bs_chunk_get_loc(&bs->frame->closure->fn->chunk, bs->op_index);
    bs_write(w, Bs_Loc_Fmt "error: ", Bs_Loc_Arg(loc));
    {
        va_list args;
        va_start(args, fmt);
        w->fmt(w, fmt, args);
        va_end(args);
    }
    bs_write(w, "\n");

    for (size_t i = bs->frames.count; i > 1; i--) {
        const Bs_Frame *callee = &bs->frames.data[i - 1];
        const Bs_Frame *caller = &bs->frames.data[i - 2];
        const Bs_Fn *fn = caller->closure->fn;

        size_t op_index = caller->ip - fn->chunk.data - 1;
        if (!callee->closure->fn->module) {
            op_index -= sizeof(size_t);
        }

        loc = bs_chunk_get_loc(&fn->chunk, op_index);
        bs_write(w, Bs_Loc_Fmt "in ", Bs_Loc_Arg(loc));

        bs_value_write(w, bs_value_object(callee->closure->fn));
        bs_write(w, "\n");
    }
}

bool bs_check_arity(Bs *bs, size_t actual, size_t expected) {
    if (actual != expected) {
        bs_error(bs, "expected %zu arguments, got %zu", expected, actual);
        return false;
    }

    return true;
}

bool bs_check_value_type(Bs *bs, Bs_Value value, Bs_Value_Type expected, const char *label) {
    if (value.type != expected) {
        bs_error(
            bs,
            "expected %s to be %s, got %s",
            label,
            bs_value_type_name((Bs_Value){.type = expected}),
            bs_value_type_name(value));
        return false;
    }

    return true;
}

bool bs_check_object_type(Bs *bs, Bs_Value value, Bs_Object_Type expected, const char *label) {
    if (value.type != BS_VALUE_OBJECT || value.as.object->type != expected) {
        bs_error(
            bs,
            "expected %s to be %s, got %s",
            label,
            bs_object_type_name(expected),
            bs_value_type_name(value));
        return false;
    }

    return true;
}

bool bs_check_object_c_type(
    Bs *bs, Bs_Value value, const Bs_C_Data_Spec *expected, const char *label) {
    if (value.type != BS_VALUE_OBJECT || value.as.object->type != BS_OBJECT_C_DATA) {
        bs_error(
            bs,
            "expected %s to be native " Bs_Sv_Fmt " object, got %s",
            label,
            Bs_Sv_Arg(expected->name),
            bs_value_type_name(value));

        return false;
    }

    const Bs_C_Data *native = (const Bs_C_Data *)value.as.object;
    if (native->spec != expected) {
        bs_error(
            bs,
            "expected %s to be native " Bs_Sv_Fmt " object, got native " Bs_Sv_Fmt " object",
            label,
            Bs_Sv_Arg(expected->name),
            Bs_Sv_Arg(native->spec->name));

        return false;
    }

    return true;
}

bool bs_check_whole_number(Bs *bs, Bs_Value value, const char *label) {
    if (!bs_check_value_type(bs, value, BS_VALUE_NUM, label)) {
        return false;
    }

    if (value.as.number < 0 || value.as.number != (long)value.as.number) {
        bs_error(bs, "expected %s to be positive whole number, got %g", label, value.as.number);
        return false;
    }

    return true;
}

// Interpreter
static void bs_stack_push(Bs *bs, Bs_Value value) {
    // Do not trigger garbage collection in the stack
    if (bs->stack.count >= bs->stack.capacity) {
        bs->stack.capacity = bs->stack.capacity ? bs->stack.capacity * 2 : BS_DA_INIT_CAP;
        bs->stack.data = realloc(bs->stack.data, bs->stack.capacity * sizeof(*bs->stack.data));
        assert(bs->stack.data);
    }

    bs->stack.data[bs->stack.count++] = value;
}

static Bs_Value bs_stack_pop(Bs *bs) {
    return bs->stack.data[--bs->stack.count];
}

static Bs_Value bs_stack_peek(Bs *bs, size_t offset) {
    return bs->stack.data[bs->stack.count - offset - 1];
}

static size_t bs_chunk_read_int(Bs *bs) {
    const size_t index = *(size_t *)bs->frame->ip;
    bs->frame->ip += sizeof(index);
    return index;
}

static Bs_Value *bs_chunk_read_const(Bs *bs) {
    return &bs->frame->closure->fn->chunk.constants.data[bs_chunk_read_int(bs)];
}

static bool bs_binary_op(Bs *bs, Bs_Value *a, Bs_Value *b, const char *op) {
    *b = bs_stack_pop(bs);
    *a = bs_stack_pop(bs);

    if (a->type != BS_VALUE_NUM || b->type != BS_VALUE_NUM) {
        bs_error(
            bs,
            "invalid operands to binary (%s): %s, %s",
            op,
            bs_value_type_name(*a),
            bs_value_type_name(*b));

        return false;
    }

    return true;
}

static bool bs_call(Bs *bs, Bs_Closure *closure, size_t arity) {
    if (!bs_check_arity(bs, closure->fn->arity, arity)) {
        return false;
    }

    const Bs_Frame frame = {
        .closure = closure,
        .ip = closure->fn->chunk.data,
        .base = bs->stack.count - arity - 1,
    };

    bs_frames_push(bs, &bs->frames, frame);
    bs->frame = &bs->frames.data[bs->frames.count - 1];
    return true;
}

static Bs_Upvalue *bs_capture_upvalue(Bs *bs, size_t index) {
    Bs_Upvalue *previous = NULL;
    Bs_Upvalue *upvalue = bs->upvalues;
    while (upvalue && upvalue->index > index) {
        previous = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue && upvalue->index == index) {
        return upvalue;
    }

    Bs_Upvalue *created = bs_upvalue_new(bs, index);
    created->next = upvalue;

    if (previous) {
        previous->next = created;
    } else {
        bs->upvalues = created;
    }

    return created;
}

static void bs_close_upvalues(Bs *bs, size_t index) {
    while (bs->upvalues && bs->upvalues->index >= index) {
        Bs_Upvalue *upvalue = bs->upvalues;
        upvalue->closed = true;
        upvalue->value = bs->stack.data[upvalue->index];
        bs->upvalues = upvalue->next;
    }
}

static_assert(BS_COUNT_OPS == 41, "Update bs_run()");
bool bs_run(Bs *bs, const char *path, Bs_Sv input, bool step) {
    bool result = true;

    {
        Bs_Fn *fn = bs_compile(bs, path, input);
        if (!fn) {
            bs_return_defer(false);
        }

        bs_da_push(bs, &bs->modules, (Bs_Module){.name = fn->name});
        fn->module = bs->modules.count;

        Bs_Closure *closure = bs_closure_new(bs, fn);
        bs_stack_push(bs, bs_value_object(closure));
        bs_call(bs, closure, 0);
    }

    bs->gc_on = true;
    while (true) {
        if (step) {
            Bs_Writer *w = bs_stdout_writer(bs);
            bs_write(w, "----------------------------------------\n");
            bs_write(w, "Globals: ");
            bs_value_write(w, bs_value_object(&bs->globals));
            bs_write(w, "\n\n");

            bs_write(w, "Stack:\n");
            for (size_t i = 0; i < bs->stack.count; i++) {
                bs_write(w, "    ");
                bs_value_write(w, bs->stack.data[i]);
                bs_write(w, "\n");
            }
            bs_write(w, "\n");

            size_t offset = bs->frame->ip - bs->frame->closure->fn->chunk.data;
            bs_debug_op(w, &bs->frame->closure->fn->chunk, &offset);
            bs_write(w, "----------------------------------------\n");
            getchar();
        }

        bs->op_index = bs->frame->ip - bs->frame->closure->fn->chunk.data;
        const Bs_Op op = *bs->frame->ip++;
        switch (op) {
        case BS_OP_RET: {
            const Bs_Value value = bs_stack_pop(bs);
            bs_close_upvalues(bs, bs->frame->base);
            bs->frames.count--;

            if (!bs->frames.count) {
                bs_return_defer(true);
            }

            const Bs_Frame *frame = &bs->frames.data[bs->frames.count];
            bs->stack.count = bs->frame->base;
            bs_stack_push(bs, value);

            bs->frame = &bs->frames.data[bs->frames.count - 1];

            if (frame->closure->fn->module) {
                Bs_Module *m = &bs->modules.data[frame->closure->fn->module - 1];
                m->done = true;
                m->result = value;
            }
        } break;

        case BS_OP_CALL: {
            const size_t arity = bs_chunk_read_int(bs);
            const Bs_Value value = bs_stack_peek(bs, arity);

            if (value.type != BS_VALUE_OBJECT) {
                bs_error(bs, "cannot call %s value", bs_value_type_name(value));
                bs_return_defer(false);
            }

            static_assert(BS_COUNT_OBJECTS == 9, "Update bs_run()");
            switch (value.as.object->type) {
            case BS_OBJECT_CLOSURE:
                if (!bs_call(bs, (Bs_Closure *)value.as.object, arity)) {
                    bs_return_defer(false);
                }
                break;

            case BS_OBJECT_C_FN: {
                const Bs_C_Fn *native = (Bs_C_Fn *)value.as.object;
                const bool gc_on = bs->gc_on;
                bs->gc_on = false;

                Bs_Value value = bs_value_nil;
                if (!native->fn(bs, &bs->stack.data[bs->stack.count - arity], arity, &value)) {
                    bs_return_defer(false);
                }

                bs->stack.count -= arity;
                bs->stack.data[bs->stack.count - 1] = value;

                bs->gc_on = gc_on;
            } break;

            default:
                bs_error(bs, "cannot call %s value", bs_value_type_name(value));
                bs_return_defer(false);
            }
        } break;

        case BS_OP_CLOSURE: {
            Bs_Fn *fn = (Bs_Fn *)bs_chunk_read_const(bs)->as.object;
            Bs_Closure *closure = bs_closure_new(bs, fn);
            bs_stack_push(bs, bs_value_object(closure));

            for (size_t i = 0; i < closure->upvalues; i++) {
                const bool local = *bs->frame->ip++;
                const size_t index = bs_chunk_read_int(bs);

                if (local) {
                    closure->data[i] = bs_capture_upvalue(bs, bs->frame->base + index);
                } else {
                    closure->data[i] = bs->frame->closure->data[index];
                }
            }
        } break;

        case BS_OP_DROP:
            bs_stack_pop(bs);
            break;

        case BS_OP_UCLOSE:
            bs_close_upvalues(bs, bs->stack.count - 1);
            bs_stack_pop(bs);
            break;

        case BS_OP_NIL:
            bs_stack_push(bs, bs_value_nil);
            break;

        case BS_OP_TRUE:
            bs_stack_push(bs, bs_value_bool(true));
            break;

        case BS_OP_FALSE:
            bs_stack_push(bs, bs_value_bool(false));
            break;

        case BS_OP_ARRAY:
            bs_stack_push(bs, bs_value_object(bs_array_new(bs)));
            break;

        case BS_OP_TABLE:
            bs_stack_push(bs, bs_value_object(bs_table_new(bs)));
            break;

        case BS_OP_CONST:
            bs_stack_push(bs, *bs_chunk_read_const(bs));
            break;

        case BS_OP_ADD: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);

            if (a.type != BS_VALUE_NUM || b.type != BS_VALUE_NUM) {
                if ((a.type == BS_VALUE_OBJECT && a.as.object->type == BS_OBJECT_STR) ||
                    (b.type == BS_VALUE_OBJECT && b.as.object->type == BS_OBJECT_STR)) {
                    bs_error(
                        bs,
                        "invalid operands to binary (+): %s, %s; use (..) for string "
                        "concatenation",
                        bs_value_type_name(a),
                        bs_value_type_name(b));
                } else {
                    bs_error(
                        bs,
                        "invalid operands to binary (+): %s, %s",
                        bs_value_type_name(a),
                        bs_value_type_name(b));
                }

                bs_return_defer(false);
            }

            bs_stack_push(bs, bs_value_num(a.as.number + b.as.number));
        } break;

        case BS_OP_SUB: {
            Bs_Value a, b;
            if (!bs_binary_op(bs, &a, &b, "-")) {
                bs_return_defer(false);
            }

            bs_stack_push(bs, bs_value_num(a.as.number - b.as.number));
        } break;

        case BS_OP_MUL: {
            Bs_Value a, b;
            if (!bs_binary_op(bs, &a, &b, "*")) {
                bs_return_defer(false);
            }

            bs_stack_push(bs, bs_value_num(a.as.number * b.as.number));
        } break;

        case BS_OP_DIV: {
            Bs_Value a, b;
            if (!bs_binary_op(bs, &a, &b, "/")) {
                bs_return_defer(false);
            }

            bs_stack_push(bs, bs_value_num(a.as.number / b.as.number));
        } break;

        case BS_OP_NEG: {
            const Bs_Value a = bs_stack_pop(bs);
            if (a.type != BS_VALUE_NUM) {
                bs_error(bs, "invalid operand to unary (-): %s", bs_value_type_name(a));
                bs_return_defer(false);
            }

            bs_stack_push(bs, bs_value_num(-a.as.number));
        } break;

        case BS_OP_NOT:
            bs_stack_push(bs, bs_value_bool(bs_value_is_falsey(bs_stack_pop(bs))));
            break;

        case BS_OP_GT: {
            Bs_Value a, b;
            if (!bs_binary_op(bs, &a, &b, ">")) {
                bs_return_defer(false);
            }

            bs_stack_push(bs, bs_value_bool(a.as.number > b.as.number));
        } break;

        case BS_OP_GE: {
            Bs_Value a, b;
            if (!bs_binary_op(bs, &a, &b, ">=")) {
                bs_return_defer(false);
            }

            bs_stack_push(bs, bs_value_bool(a.as.number >= b.as.number));
        } break;

        case BS_OP_LT: {
            Bs_Value a, b;
            if (!bs_binary_op(bs, &a, &b, "<")) {
                bs_return_defer(false);
            }

            bs_stack_push(bs, bs_value_bool(a.as.number < b.as.number));
        } break;

        case BS_OP_LE: {
            Bs_Value a, b;
            if (!bs_binary_op(bs, &a, &b, "<=")) {
                bs_return_defer(false);
            }

            bs_stack_push(bs, bs_value_bool(a.as.number <= b.as.number));
        } break;

        case BS_OP_EQ: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_stack_push(bs, bs_value_bool(bs_value_equal(a, b)));
        } break;

        case BS_OP_NE: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_stack_push(bs, bs_value_bool(!bs_value_equal(a, b)));
        } break;

        case BS_OP_LEN: {
            const Bs_Value a = bs_stack_peek(bs, 0);
            if (a.type != BS_VALUE_OBJECT) {
                bs_error(bs, "cannot get length of %s value", bs_value_type_name(a));
                bs_return_defer(false);
            }

            size_t size;
            switch (a.as.object->type) {
            case BS_OBJECT_STR:
                size = ((Bs_Str *)a.as.object)->size;
                break;

            case BS_OBJECT_ARRAY:
                size = ((Bs_Array *)a.as.object)->count;
                break;

            case BS_OBJECT_TABLE:
                size = ((Bs_Table *)a.as.object)->real_count;
                break;

            default:
                bs_error(bs, "cannot get length of %s value", bs_value_type_name(a));
                bs_return_defer(false);
            }

            bs_stack_pop(bs);
            bs_stack_push(bs, bs_value_num(size));
        } break;

        case BS_OP_JOIN: {
            const bool gc_on = bs->gc_on;
            bs->gc_on = false;

            size_t start;
            Bs_Writer *w = bs_str_writer_init(bs, &start);

            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_value_write(w, a);
            bs_value_write(w, b);

            const Bs_Sv sv = bs_str_writer_end(bs, start);
            bs_stack_push(bs, bs_value_object(bs_str_new(bs, sv)));

            bs->gc_on = gc_on;
        } break;

        case BS_OP_IMPORT: {
            const Bs_Value a = bs_stack_pop(bs);
            if (!bs_check_object_type(bs, a, BS_OBJECT_STR, "import path")) {
                bs_return_defer(false);
            }
            Bs_Str *name = (Bs_Str *)a.as.object;

            if (!name->size) {
                bs_error(bs, "import path cannot be empty");
                bs_return_defer(false);
            }

            size_t index;
            if (bs_modules_find(&bs->modules, name, &index)) {
                const Bs_Module *m = &bs->modules.data[index];
                if (!m->done) {
                    bs_error(bs, "import loop detected");
                    bs_return_defer(false);
                }

                bs_stack_push(bs, m->result);
            } else {
                const bool gc_on = bs->gc_on;
                bs->gc_on = false;

                bs_value_write((Bs_Writer *)&bs->paths_writer, a);
                bs->paths_writer.count++; // Account for the NULL terminator
                const char *path = bs->paths_writer.data + bs->paths_writer.count - name->size - 1;

                if (bs_sv_suffix(bs_sv_from_parts(name->data, name->size), Bs_Sv_Static(".so"))) {
                    void *library = dlopen(path, RTLD_NOW);
                    if (!library) {
                        bs_error(bs, "could not load library '%s': %s", path, dlerror());
                        bs_return_defer(false);
                    }

                    bs_stack_push(bs, bs_value_object(bs_c_lib_new(bs, library, name)));
                } else {
                    size_t size = 0;
                    char *contents = bs_read_file(path, &size);
                    if (!contents) {
                        bs_error(bs, "could not read file '%s'", path);
                        bs_return_defer(false);
                    }

                    Bs_Fn *fn = bs_compile(bs, path, bs_sv_from_parts(contents, size));
                    free(contents);

                    if (!fn) {
                        bs_return_defer(false);
                    }
                    bs_da_push(bs, &bs->modules, (Bs_Module){.name = fn->name});
                    fn->module = bs->modules.count;

                    Bs_Closure *closure = bs_closure_new(bs, fn);
                    bs_stack_push(bs, bs_value_object(closure));
                    bs_call(bs, closure, 0);
                }

                bs->gc_on = gc_on;
            }
        } break;

        case BS_OP_GDEF:
            bs_table_set(
                bs,
                &bs->globals,
                (Bs_Str *)bs_chunk_read_const(bs)->as.object,
                bs_stack_peek(bs, 0));

            bs_stack_pop(bs);
            break;

        case BS_OP_GGET: {
            Bs_Str *name = (Bs_Str *)bs_chunk_read_const(bs)->as.object;

            Bs_Value value;
            if (!bs_table_get(bs, &bs->globals, name, &value)) {
                bs_error(bs, "undefined variable '" Bs_Sv_Fmt "'", Bs_Sv_Arg(*name));
                bs_return_defer(false);
            }

            bs_stack_push(bs, value);
        } break;

        case BS_OP_GSET: {
            Bs_Str *name = (Bs_Str *)bs_chunk_read_const(bs)->as.object;
            const Bs_Value value = bs_stack_peek(bs, 0);

            if (bs_table_set(bs, &bs->globals, name, value)) {
                bs_table_remove(bs, &bs->globals, name);
                bs_error(bs, "undefined variable '" Bs_Sv_Fmt "'", Bs_Sv_Arg(*name));
                bs_return_defer(false);
            }
        } break;

        case BS_OP_CGET: {
            Bs_Str *name = (Bs_Str *)bs_chunk_read_const(bs)->as.object;

            Bs_Value value;
            if (!bs_table_get(bs, &bs->core, name, &value)) {
                bs_error(bs, "undefined native value '" Bs_Sv_Fmt "'", Bs_Sv_Arg(*name));
                bs_return_defer(false);
            }

            bs_stack_push(bs, value);
        } break;

        case BS_OP_LGET:
            bs_stack_push(bs, bs->stack.data[bs->frame->base + bs_chunk_read_int(bs)]);
            break;

        case BS_OP_LSET:
            bs->stack.data[bs->frame->base + bs_chunk_read_int(bs)] = bs_stack_peek(bs, 0);
            break;

        case BS_OP_UGET: {
            Bs_Upvalue *upvalue = bs->frame->closure->data[bs_chunk_read_int(bs)];
            if (upvalue->closed) {
                bs_stack_push(bs, upvalue->value);
            } else {
                bs_stack_push(bs, bs->stack.data[upvalue->index]);
            }
        } break;

        case BS_OP_USET: {
            const Bs_Value value = bs_stack_peek(bs, 0);
            Bs_Upvalue *upvalue = bs->frame->closure->data[bs_chunk_read_int(bs)];
            if (upvalue->closed) {
                upvalue->value = value;
            } else {
                bs->stack.data[upvalue->index] = value;
            }
        } break;

        case BS_OP_IGET: {
            Bs_Value value;

            const Bs_Value container = bs_stack_peek(bs, 1);
            if (container.type != BS_VALUE_OBJECT) {
                bs_error(bs, "cannot index into %s value", bs_value_type_name(container));
                bs_return_defer(false);
            }

            const Bs_Value index = bs_stack_peek(bs, 0);
            if (container.as.object->type == BS_OBJECT_ARRAY) {
                if (!bs_check_whole_number(bs, index, "array index")) {
                    bs_return_defer(false);
                }

                Bs_Array *array = (Bs_Array *)container.as.object;
                if (!bs_array_get(bs, array, index.as.number, &value)) {
                    bs_error(
                        bs,
                        "cannot get value at index %zu in array of length %zu",
                        (size_t)index.as.number,
                        array->count);

                    bs_return_defer(false);
                }
            } else if (container.as.object->type == BS_OBJECT_TABLE) {
                if (!bs_check_object_type(bs, index, BS_OBJECT_STR, "table key")) {
                    bs_return_defer(false);
                }

                Bs_Str *key = (Bs_Str *)index.as.object;
                if (!bs_table_get(bs, (Bs_Table *)container.as.object, key, &value)) {
                    value = bs_value_nil;
                }
            } else if (container.as.object->type == BS_OBJECT_C_LIB) {
                Bs_C_Lib *library = (Bs_C_Lib *)container.as.object;

                if (!bs_check_object_type(bs, index, BS_OBJECT_STR, "library symbol name")) {
                    bs_return_defer(false);
                }

                Bs_Str *name = (Bs_Str *)index.as.object;
                if (!name->size) {
                    bs_error(bs, "library symbol name cannot be empty");
                    bs_return_defer(false);
                }

                if (!bs_table_get(bs, &library->functions, name, &value)) {
                    const bool gc_on = bs->gc_on;
                    bs->gc_on = false;

                    size_t start;
                    Bs_Writer *w = bs_str_writer_init(bs, &start);
                    bs_value_write(w, index);

                    Bs_C_Fn_Ptr fn = dlsym(library->data, bs_str_writer_end(bs, start).data);
                    if (!fn) {
                        bs_error(
                            bs,
                            "symbol '" Bs_Sv_Fmt "' doesn't exist in native library '" Bs_Sv_Fmt
                            "'",
                            Bs_Sv_Arg(*name),
                            Bs_Sv_Arg(*library->path));

                        bs_return_defer(false);
                    }

                    Bs_C_Fn *fn_object = bs_c_fn_new(bs, fn);
                    fn_object->library = library;

                    value = bs_value_object(fn_object);
                    bs_table_set(bs, &library->functions, name, value);

                    bs->gc_on = gc_on;
                }
            } else {
                bs_error(bs, "cannot index into %s value", bs_value_type_name(container));
                bs_return_defer(false);
            }

            bs->stack.count -= 2;
            bs_stack_push(bs, value);
        } break;

        case BS_OP_ISET:
        case BS_OP_ILIT: {
            const Bs_Value container = bs_stack_peek(bs, 2);
            if (container.type != BS_VALUE_OBJECT) {
                bs_error(bs, "cannot index into %s value", bs_value_type_name(container));
                bs_return_defer(false);
            }

            const Bs_Value value = bs_stack_peek(bs, 0);
            const Bs_Value index = bs_stack_peek(bs, 1);

            if (container.as.object->type == BS_OBJECT_ARRAY) {
                if (!bs_check_whole_number(bs, index, "array index")) {
                    bs_return_defer(false);
                }

                bs_array_set(bs, (Bs_Array *)container.as.object, index.as.number, value);
            } else if (container.as.object->type == BS_OBJECT_TABLE) {
                if (!bs_check_object_type(bs, index, BS_OBJECT_STR, "table key")) {
                    bs_return_defer(false);
                }

                Bs_Table *table = (Bs_Table *)container.as.object;
                if (value.type == BS_VALUE_NIL) {
                    bs_table_remove(bs, table, (Bs_Str *)index.as.object);
                } else {
                    bs_table_set(bs, table, (Bs_Str *)index.as.object, value);
                }
            } else if (container.as.object->type == BS_OBJECT_C_LIB) {
                bs_error(bs, "cannot modify %s value", bs_value_type_name(container));
                bs_return_defer(false);
            } else {
                bs_error(bs, "cannot index into %s value", bs_value_type_name(container));
                bs_return_defer(false);
            }

            bs->stack.count -= 2;
        } break;

        case BS_OP_JUMP:
            bs->frame->ip += bs_chunk_read_int(bs);
            break;

        case BS_OP_ELSE: {
            const size_t offset = bs_chunk_read_int(bs);
            if (bs_value_is_falsey(bs_stack_peek(bs, 0))) {
                bs->frame->ip += offset;
            }
        } break;

        case BS_OP_THEN: {
            const size_t offset = bs_chunk_read_int(bs);
            if (!bs_value_is_falsey(bs_stack_peek(bs, 0))) {
                bs->frame->ip += offset;
            }
        } break;

        case BS_OP_PRINT: {
            Bs_Writer *w = bs_stdout_writer(bs);
            bs_value_write(w, bs_stack_pop(bs));
            bs_write(w, "\n");
        } break;

        default:
            bs_write(bs_stderr_writer(bs), "invalid op %d at offset %zu", op, bs->op_index);
            bs_return_defer(false);
        }
    }

defer:
    bs->stack.count = 0;

    bs->frame = NULL;
    bs->frames.count = 0;

    bs->upvalues = NULL;
    bs->gc_on = false;
    return result;
}