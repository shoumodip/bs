#include <dlfcn.h>
#include <setjmp.h>

#include "bs/compiler.h"
#include "bs/debug.h"
#include "bs/hash.h"

// #define BS_GC_DEBUG_LOG
// #define BS_GC_DEBUG_STRESS
#define BS_GC_GROW_FACTOR 2
#define BS_STACK_CAPACITY (1024 * 1024)

typedef struct {
    Bs_Value *base;
    const uint8_t *ip;

    union {
        Bs_Closure *closure;
        const Bs_C_Fn *native;
    };

    bool extended;
} Bs_Frame;

typedef struct {
    Bs_Frame *data;
    size_t count;
    size_t capacity;
} Bs_Frames;

#define bs_frames_free bs_da_free
#define bs_frames_push bs_da_push

typedef struct {
    bool done;
    const Bs_Str *name;

    Bs_Value result;
} Bs_Module;

typedef struct {
    Bs_Module *data;
    size_t count;
    size_t capacity;
} Bs_Modules;

#define bs_modules_free bs_da_free

static bool bs_modules_find(Bs_Modules *m, const Bs_Str *name, size_t *index) {
    for (size_t i = 0; i < m->count; i++) {
        if (bs_str_eq(m->data[i].name, name)) {
            *index = i;
            return true;
        }
    }

    return false;
}

typedef struct {
    Bs_Value *data;
    size_t count;
} Bs_Stack;

struct Bs {
    Bs_Stack stack;

    Bs_Frame *frame;
    Bs_Frames frames;

    Bs_Modules modules;

    Bs_Table globals;
    Bs_Table strings;
    Bs_Upvalue *upvalues;

    bool gc_on;
    size_t gc_max;
    size_t gc_bytes;
    Bs_Object *objects;

    Bs_Buffer paths;
    Bs_Buffer buffer;

    Bs_Writer error;
    Bs_Writer output;
    Bs_Pretty_Printer printer;

    bool step;
    void *userdata;

    int exit;
    bool ok;
    jmp_buf unwind;
};

// Garbage collector
static_assert(BS_COUNT_OBJECTS == 9, "Update bs_free_object()");
static void bs_free_object(Bs *bs, Bs_Object *object) {
#ifdef BS_GC_DEBUG_LOG
    bs_fmt(bs_stdout_get(bs), "[GC] Free %p; Type: %d\n", object, object->type);
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
    Bs_Writer *w = bs_stdout_get(bs);
    bs_fmt(w, "[GC] Mark %p ", object);
    bs_value_write(bs, w, bs_value_object(object));
    bs_fmt(w, "\n");
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
            Bs_Entry *entry = &table->data[i];
            if (entry->key.type == BS_VALUE_OBJECT) {
                bs_mark_object(bs, entry->key.as.object);
            }

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
        if (upvalue->value->type == BS_VALUE_OBJECT) {
            bs_mark_object(bs, upvalue->value->as.object);
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
    const bool gc_on_save = bs->gc_on;
    bs->gc_on = false;

#ifdef BS_GC_DEBUG_LOG
    Bs_Writer *w = bs_stdout_get(bs);
    bs_fmt(w, "\n-------- GC Begin --------\n");
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
        bs_fmt(
            w,
            "\n[GC] Collected %zu bytes (%zu -> %zu); Next run at %zu bytes\n",
            before - bs->gc_bytes,
            before,
            bs->gc_bytes,
            bs->gc_max);
    }

    bs_fmt(w, "-------- GC End ----------\n\n");
#endif // BS_GC_DEBUG_LOG

    bs->gc_on = gc_on_save;
}

// Interface
Bs *bs_new(bool step) {
    Bs *bs = calloc(1, sizeof(Bs));
    assert(bs);

    bs->stack.data = malloc(BS_STACK_CAPACITY * sizeof(*bs->stack.data));
    assert(bs->stack.data);

    bs->gc_max = 1024 * 1024;

    bs->paths.bs = bs;
    bs->buffer.bs = bs;

    bs->error = (Bs_Writer){stderr, bs_file_write};
    bs->output = (Bs_Writer){stdout, bs_file_write};
    bs->printer.bs = bs;

    bs->globals.meta.type = BS_OBJECT_TABLE;

    bs->step = step;
    return bs;
}

void bs_free(Bs *bs) {
    free(bs->stack.data);
    memset(&bs->stack, 0, sizeof(bs->stack));

    bs_frames_free(bs, &bs->frames);
    bs_modules_free(bs, &bs->modules);

    bs_table_free(bs, &bs->globals);
    bs_table_free(bs, &bs->strings);

    Bs_Object *object = bs->objects;
    while (object) {
        Bs_Object *next = object->next;
        bs_free_object(bs, object);
        object = next;
    }

    bs_buffer_free(bs, &bs->paths);
    bs_buffer_free(bs, &bs->buffer);

    bs_da_free(bs, &bs->printer);
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
void bs_file_write(Bs_Writer *w, Bs_Sv sv) {
    fwrite(sv.data, sv.size, 1, w->data);
}

void bs_buffer_write(Bs_Writer *w, Bs_Sv sv) {
    Bs_Buffer *b = w->data;
    bs_da_push_many(b->bs, b, sv.data, sv.size);
}

Bs_Sv bs_buffer_reset(Bs_Buffer *b, size_t pos) {
    const Bs_Sv sv = {b->data + pos, b->count - pos};
    b->count = pos;
    return sv;
}

Bs_Writer bs_file_writer(FILE *file) {
    return (Bs_Writer){file, bs_file_write};
}

Bs_Writer bs_buffer_writer(Bs_Buffer *buffer) {
    return (Bs_Writer){buffer, bs_buffer_write};
}

Bs_Buffer *bs_paths_get(Bs *bs) {
    return &bs->paths;
}

Bs_Buffer *bs_buffer_get(Bs *bs) {
    return &bs->buffer;
}

Bs_Writer *bs_stdout_get(Bs *bs) {
    return &bs->output;
}

void bs_stdout_set(Bs *bs, Bs_Writer writer) {
    bs->output = writer;
}

Bs_Writer *bs_stderr_get(Bs *bs) {
    return &bs->error;
}

void bs_stderr_set(Bs *bs, Bs_Writer writer) {
    bs->error = writer;
}

void *bs_userdata_get(Bs *bs) {
    return bs->userdata;
}

void bs_userdata_set(Bs *bs, void *data) {
    bs->userdata = data;
}

Bs_Object *bs_object_new(Bs *bs, Bs_Object_Type type, size_t size) {
    Bs_Object *object = bs_realloc(bs, NULL, 0, size);
    object->type = type;
    object->next = bs->objects;
    object->marked = false;
    bs->objects = object;

#ifdef BS_GC_DEBUG_LOG
    bs_fmt(bs_stdout_get(bs), "[GC] Allocate %p (%zu bytes); Type: %d\n", object, size, type);
#endif // BS_GC_DEBUG_LOG

    return object;
}

Bs_Str *bs_str_const(Bs *bs, Bs_Sv sv) {
    Bs_Entry *entry = bs_entries_find_sv(bs->strings.data, bs->strings.capacity, sv);
    if (entry && entry->key.type != BS_VALUE_NIL) {
        return (Bs_Str *)entry->key.as.object;
    }

    Bs_Str *str = bs_str_new(bs, sv);
    bs_table_set(bs, &bs->strings, bs_value_object(str), bs_value_nil);
    return str;
}

void bs_global_set(Bs *bs, Bs_Sv name, Bs_Value value) {
    bs_table_set(bs, &bs->globals, bs_value_object(bs_str_const(bs, name)), value);
}

static Bs_Pretty_Printer *bs_pretty_printer(Bs *bs, Bs_Writer *w) {
    bs->printer.writer = w;
    bs->printer.extended = bs->frame ? bs->frame->extended : false;
    bs->printer.depth = 0;
    bs->printer.count = 0;
    return &bs->printer;
}

void bs_value_write(Bs *bs, Bs_Writer *w, Bs_Value value) {
    bs_value_print_impl(bs_pretty_printer(bs, w), value);
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
    bs->gc_on = false;

    Bs_Writer *w = bs_stderr_get(bs);

    if (bs->frame->ip) {
        const Bs_Loc loc = bs_chunk_get_loc(
            &bs->frame->closure->fn->chunk, bs->frame->ip - bs->frame->closure->fn->chunk.data);
        bs_fmt(w, Bs_Loc_Fmt, Bs_Loc_Arg(loc));
    } else {
        bs_fmt(w, "[C]: ");
    }

    bs_fmt(w, "error: ");
    {
        Bs_Buffer *b = bs_buffer_get(bs);
        const size_t start = b->count;

        int count;
        {
            va_list args;
            va_start(args, fmt);
            count = vsnprintf(NULL, 0, fmt, args);
            assert(count >= 0);
            va_end(args);
        }

        bs_da_push_many(b->bs, b, NULL, count + 1);
        {
            va_list args;
            va_start(args, fmt);
            vsnprintf(b->data + b->count, count + 1, fmt, args);
            va_end(args);
        }
        b->count += count;

        Bs_Writer *w = bs_stderr_get(bs);
        w->write(w, bs_buffer_reset(b, start));
    }

    if (fmt[strlen(fmt) - 1] != '\n' || bs->frames.count > 1) {
        bs_fmt(w, "\n");
    }

    for (size_t i = bs->frames.count; i > 1; i--) {
        const Bs_Frame *callee = &bs->frames.data[i - 1];
        const Bs_Frame *caller = &bs->frames.data[i - 2];

        if (caller->ip) {
            const Bs_Fn *fn = caller->closure->fn;

            const Bs_Loc loc = bs_chunk_get_loc(&fn->chunk, caller->ip - fn->chunk.data);
            if (*loc.path == '\0') {
                continue;
            }

            bs_fmt(w, Bs_Loc_Fmt, Bs_Loc_Arg(loc));
        } else {
            bs_fmt(w, "[C]: ");
        }

        bs_fmt(w, "in ");
        if (callee->ip) {
            bs_value_write(bs, w, bs_value_object(callee->closure->fn));
        } else {
            const Bs_C_Fn *fn = callee->native;
            if (fn->library) {
                bs_fmt(w, "native fn ");

                Bs_Sv path = Bs_Sv(fn->library->path->data, fn->library->path->size);
                for (size_t i = path.size; i > 0; i--) {
                    if (path.data[i - 1] == '.') {
                        path.size = i - 1;
                        break;
                    }
                }

                bs_fmt(w, Bs_Sv_Fmt ".%s()", Bs_Sv_Arg(path), fn->name);
            } else {
                bs_fmt(w, "native fn %s()", fn->name);
            }
        }

        bs_fmt(w, "\n");
    }

    bs->ok = false;
    bs_unwind(bs, 1);
}

void bs_unwind(Bs *bs, unsigned char exit) {
    bs->exit = exit;
    longjmp(bs->unwind, 1);
}

void bs_check_arity(Bs *bs, size_t actual, size_t expected) {
    if (actual != expected) {
        bs_error(
            bs, "expected %zu argument%s, got %zu", expected, expected == 1 ? "" : "s", actual);
    }
}

void bs_check_callable(Bs *bs, Bs_Value value, const char *label) {
    if (value.type != BS_VALUE_OBJECT ||
        (value.as.object->type != BS_OBJECT_CLOSURE && value.as.object->type != BS_OBJECT_C_FN)) {
        bs_error(
            bs,
            "expected %s to be callable object, got %s",
            label,
            bs_value_type_name(value, bs->frame->extended));
    }
}

void bs_check_value_type(Bs *bs, Bs_Value value, Bs_Value_Type expected, const char *label) {
    if (value.type != expected) {
        bs_error(
            bs,
            "expected %s to be %s, got %s",
            label,
            bs_value_type_name((Bs_Value){.type = expected}, bs->frame->extended),
            bs_value_type_name(value, bs->frame->extended));
    }
}

void bs_check_object_type(Bs *bs, Bs_Value value, Bs_Object_Type expected, const char *label) {
    if (value.type != BS_VALUE_OBJECT || value.as.object->type != expected) {
        bs_error(
            bs,
            "expected %s to be %s, got %s",
            label,
            bs_object_type_name(expected),
            bs_value_type_name(value, bs->frame->extended));
    }
}

void bs_check_object_c_type(
    Bs *bs, Bs_Value value, const Bs_C_Data_Spec *expected, const char *label) {
    if (value.type != BS_VALUE_OBJECT || value.as.object->type != BS_OBJECT_C_DATA) {
        bs_error(
            bs,
            "expected %s to be native " Bs_Sv_Fmt " object, got %s",
            label,
            Bs_Sv_Arg(expected->name),
            bs_value_type_name(value, bs->frame->extended));
    }

    const Bs_C_Data *native = (const Bs_C_Data *)value.as.object;
    if (native->spec != expected) {
        bs_error(
            bs,
            "expected %s to be native " Bs_Sv_Fmt " object, got native " Bs_Sv_Fmt " object",
            label,
            Bs_Sv_Arg(expected->name),
            Bs_Sv_Arg(native->spec->name));
    }
}

void bs_check_integer(Bs *bs, Bs_Value value, const char *label) {
    bs_check_value_type(bs, value, BS_VALUE_NUM, label);
    if (value.as.number != (long)value.as.number) {
        bs_error(bs, "expected %s to be integer, got %g", label, value.as.number);
    }
}

void bs_check_whole_number(Bs *bs, Bs_Value value, const char *label) {
    bs_check_value_type(bs, value, BS_VALUE_NUM, label);
    if (value.as.number < 0 || value.as.number != (long)value.as.number) {
        bs_error(bs, "expected %s to be positive whole number, got %g", label, value.as.number);
    }
}

// Interpreter
static void bs_stack_push(Bs *bs, Bs_Value value) {
    if (bs->stack.count >= BS_STACK_CAPACITY) {
        bs_fmt(bs_stderr_get(bs), "error: stack overflow\n");

        bs->ok = false;
        bs_unwind(bs, 1);
    }

    bs->stack.data[bs->stack.count++] = value;
}

static Bs_Value bs_stack_pop(Bs *bs) {
    return bs->stack.data[--bs->stack.count];
}

static Bs_Value bs_stack_peek(Bs *bs, size_t offset) {
    return bs->stack.data[bs->stack.count - offset - 1];
}

static void bs_stack_set(Bs *bs, size_t offset, Bs_Value value) {
    bs->stack.data[bs->stack.count - offset - 1] = value;
}

static size_t bs_chunk_read_int(Bs *bs) {
    const size_t index = *(size_t *)bs->frame->ip;
    bs->frame->ip += sizeof(index);
    return index;
}

static Bs_Value bs_chunk_read_const(Bs *bs) {
    return bs->frame->closure->fn->chunk.constants.data[bs_chunk_read_int(bs)];
}

static void bs_binary_op(Bs *bs, Bs_Value *a, Bs_Value *b, const char *op) {
    *b = bs_stack_pop(bs);
    *a = bs_stack_pop(bs);

    if (a->type != BS_VALUE_NUM || b->type != BS_VALUE_NUM) {
        bs_error(
            bs,
            "invalid operands to binary (%s): %s, %s",
            op,
            bs_value_type_name(*a, bs->frame->extended),
            bs_value_type_name(*b, bs->frame->extended));
    }
}

static_assert(BS_COUNT_OBJECTS == 9, "Update bs_call_value()");
static bool bs_call_value(Bs *bs, size_t arity) {
    const Bs_Value value = bs_stack_peek(bs, arity);

    if (value.type != BS_VALUE_OBJECT) {
        bs_error(bs, "cannot call %s value", bs_value_type_name(value, bs->frame->extended));
        return false;
    }

    switch (value.as.object->type) {
    case BS_OBJECT_CLOSURE: {
        Bs_Closure *closure = (Bs_Closure *)value.as.object;
        bs_check_arity(bs, arity, closure->fn->arity);

        const Bs_Frame frame = {
            .base = &bs->stack.data[bs->stack.count - arity - 1],
            .ip = closure->fn->chunk.data,

            .closure = closure,
            .extended = closure->fn->extended,
        };

        bs_frames_push(bs, &bs->frames, frame);
        bs->frame = &bs->frames.data[bs->frames.count - 1];
    } break;

    case BS_OBJECT_C_FN: {
        const Bs_C_Fn *native = (Bs_C_Fn *)value.as.object;
        const bool gc_on_save = bs->gc_on;
        bs->gc_on = false;

        const Bs_Frame frame = {
            .base = &bs->stack.data[bs->stack.count - arity],

            .native = native,
            .extended = bs->frame ? bs->frame->extended : false,
        };

        bs_frames_push(bs, &bs->frames, frame);
        bs->frame = &bs->frames.data[bs->frames.count - 1];

        const Bs_Value value = native->fn(bs, &bs->stack.data[bs->stack.count - arity], arity);
        bs->frames.count--;
        bs->frame = &bs->frames.data[bs->frames.count - 1];

        bs->stack.count -= arity;
        bs->stack.data[bs->stack.count - 1] = value;

        bs->gc_on = gc_on_save;
    } break;

    default:
        bs_error(bs, "cannot call %s value", bs_value_type_name(value, bs->frame->extended));
        return false;
    }

    return true;
}

static Bs_Upvalue *bs_capture_upvalue(Bs *bs, Bs_Value *value) {
    Bs_Upvalue *previous = NULL;
    Bs_Upvalue *upvalue = bs->upvalues;
    while (upvalue && upvalue->value > value) {
        previous = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue && upvalue->value == value) {
        return upvalue;
    }

    Bs_Upvalue *created = bs_upvalue_new(bs, value);
    created->next = upvalue;

    if (previous) {
        previous->next = created;
    } else {
        bs->upvalues = created;
    }

    return created;
}

static void bs_close_upvalues(Bs *bs, Bs_Value *last) {
    while (bs->upvalues && bs->upvalues->value >= last) {
        Bs_Upvalue *upvalue = bs->upvalues;
        upvalue->closed = *upvalue->value;
        upvalue->value = &upvalue->closed;
        bs->upvalues = upvalue->next;
    }
}

// Interpreter
static bool bs_import_language(Bs *bs, const char *path) {
    size_t size = 0;
    char *contents = bs_read_file(path, &size);
    if (!contents) {
        return false;
    }

    Bs_Fn *fn = bs_compile(bs, path, Bs_Sv(contents, size), false, false);
    free(contents);

    if (!fn) {
        bs_unwind(bs, 1);
    }

    bs_da_push(bs, &bs->modules, (Bs_Module){.name = fn->name});
    fn->module = bs->modules.count;

    bs_stack_push(bs, bs_value_object(bs_closure_new(bs, fn)));
    bs_call_value(bs, 0);
    return true;
}

static void bs_import(Bs *bs) {
    const Bs_Value a = bs_stack_pop(bs);
    bs_check_object_type(bs, a, BS_OBJECT_STR, "module name");

    const Bs_Str *name = (const Bs_Str *)a.as.object;
    if (!name->size) {
        bs_error(bs, "module name cannot be empty");
    }

    size_t index;
    if (bs_modules_find(&bs->modules, name, &index)) {
        const Bs_Module *m = &bs->modules.data[index];
        if (!m->done) {
            bs_error(bs, "import loop detected");
        }

        bs_stack_push(bs, m->result);
        return;
    }

    const bool gc_on_save = bs->gc_on;
    bs->gc_on = false;

    Bs_Buffer *b = bs_paths_get(bs);
    const size_t start = b->count;

    bs_da_push_many(bs, b, name->data, name->size);

    // Normal
    {
        bs_da_push_many(bs, b, ".bs", 4);
        if (bs_import_language(bs, b->data + start)) {
            bs->gc_on = gc_on_save;
            return;
        }
        b->count -= 4;
    }

    // Extended
    {
        bs_da_push_many(bs, b, ".bsx", 5);
        if (bs_import_language(bs, b->data + start)) {
            bs->gc_on = gc_on_save;
            return;
        }
        b->count -= 5;
    }

    // Native
    {
        bs_da_push_many(bs, b, ".so", 4);
        void *data = dlopen(b->data + start, RTLD_NOW);
        if (!data) {
            bs_error(bs, "could not import module '" Bs_Sv_Fmt "'", Bs_Sv_Arg(*name));
        }

        Bs_C_Lib *library = bs_c_lib_new(bs, data, name);

        const Bs_Export *exports = dlsym(data, "bs_exports");
        const size_t *exports_count = dlsym(data, "bs_exports_count");

        if (!exports || !exports_count) {
            bs_error(
                bs,
                "invalid native library '" Bs_Sv_Fmt "'\n\n"
                "A BS native library should define 'bs_exports' and "
                "'bs_exports_count':\n\n"
                "```\n"
                "const Bs_Export bs_exports[] = {\n"
                "    {\"function1_name\", function1_ptr},\n"
                "    {\"function2_name\", function2_ptr},\n"
                "    /* ... */\n"
                "};\n\n"
                "const size_t bs_exports_count = bs_c_array_size(bs_exports);\n"
                "```\n",

                Bs_Sv_Arg(*name));
        }

        for (size_t i = 0; i < *exports_count; i++) {
            const Bs_Export export = exports[i];

            Bs_C_Fn *fn = bs_c_fn_new(bs, export.name, export.fn);
            fn->library = library;

            if (!bs_table_set(
                    bs,
                    &library->functions,
                    bs_value_object(bs_str_const(bs, bs_sv_from_cstr(export.name))),
                    bs_value_object(fn))) {

                bs_error(bs, "redefinition of function '%s' in FFI", export.name);
            }
        }

        const Bs_Value value = bs_value_object(library);
        bs_da_push(bs, &bs->modules, ((Bs_Module){.done = true, .name = name, .result = value}));

        bs_stack_push(bs, value);
        bs->gc_on = gc_on_save;
    }
}

static_assert(BS_COUNT_OPS == 49, "Update bs_interpret()");
static void bs_interpret(Bs *bs, Bs_Value *output) {
    const bool gc_on_save = bs->gc_on;
    const size_t frames_count_save = bs->frames.count;

    bs->gc_on = true;
    while (true) {
        if (bs->step) {
            Bs_Writer *w = bs_stdout_get(bs);
            bs_fmt(w, "\n----------------------------------------\n");
            bs_fmt(w, "Frames:\n");
            for (size_t i = 0; i < bs->frames.count; i++) {
                const Bs_Frame *frame = &bs->frames.data[i];
                if (frame->ip) {
                    bs_value_write(bs, w, bs_value_object(frame->closure));
                } else {
                    bs_fmt(w, "[C]");
                }
                bs_fmt(w, "\n");
            }
            bs_fmt(w, "\n");

            bs_fmt(w, "Stack:\n");
            for (size_t i = 0; i < bs->stack.count; i++) {
                bs_value_write(bs, w, bs->stack.data[i]);
                bs_fmt(w, "\n");
            }
            bs_fmt(w, "\n");

            size_t offset = bs->frame->ip - bs->frame->closure->fn->chunk.data;
            bs_debug_op(bs_pretty_printer(bs, w), &bs->frame->closure->fn->chunk, &offset);
            bs_fmt(w, "----------------------------------------\n");
            getchar();
        }

        const Bs_Op op = *bs->frame->ip++;
        switch (op) {
        case BS_OP_RET: {
            const Bs_Value value = bs_stack_pop(bs);
            bs_close_upvalues(bs, bs->frame->base);
            bs->frames.count--;

            if (bs->frames.count + 1 == frames_count_save) {
                if (output) {
                    *output = value;
                }

                bs->gc_on = gc_on_save;
                return;
            }

            const Bs_Frame *frame = &bs->frames.data[bs->frames.count];
            bs->stack.count = bs->frame->base - bs->stack.data;
            bs_stack_push(bs, value);

            bs->frame = &bs->frames.data[bs->frames.count - 1];
            if (frame->closure->fn->module) {
                Bs_Module *m = &bs->modules.data[frame->closure->fn->module - 1];
                m->done = true;
                m->result = value;
            }
        } break;

        case BS_OP_CALL:
            bs_call_value(bs, bs_chunk_read_int(bs));
            break;

        case BS_OP_CLOSURE: {
            const Bs_Fn *fn = (const Bs_Fn *)bs_chunk_read_const(bs).as.object;
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
            bs_close_upvalues(bs, bs->stack.data + bs->stack.count - 1);
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
            bs_stack_push(bs, bs_chunk_read_const(bs));
            break;

        case BS_OP_ADD: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);

            if (a.type != BS_VALUE_NUM || b.type != BS_VALUE_NUM) {
                if ((a.type == BS_VALUE_OBJECT && a.as.object->type == BS_OBJECT_STR) ||
                    (b.type == BS_VALUE_OBJECT && b.as.object->type == BS_OBJECT_STR)) {
                    bs_error(
                        bs,
                        "invalid operands to binary (+): %s, %s\n\n"
                        "Use (..) for string concatenation, or use string interpolation instead\n\n"
                        "```\n"
                        "\"Hello, \"..\"world!\";\n"
                        "\"Hello, \"..69;\n"
                        "\"Hello, \\(34 + 35) nice!\";\n"
                        "```\n",

                        bs_value_type_name(a, bs->frame->extended),
                        bs_value_type_name(b, bs->frame->extended));
                } else {
                    bs_error(
                        bs,
                        "invalid operands to binary (+): %s, %s",
                        bs_value_type_name(a, bs->frame->extended),
                        bs_value_type_name(b, bs->frame->extended));
                }
            }

            bs_stack_push(bs, bs_value_num(a.as.number + b.as.number));
        } break;

        case BS_OP_SUB: {
            Bs_Value a, b;
            bs_binary_op(bs, &a, &b, "-");
            bs_stack_push(bs, bs_value_num(a.as.number - b.as.number));
        } break;

        case BS_OP_MUL: {
            Bs_Value a, b;
            bs_binary_op(bs, &a, &b, "*");
            bs_stack_push(bs, bs_value_num(a.as.number * b.as.number));
        } break;

        case BS_OP_DIV: {
            Bs_Value a, b;
            bs_binary_op(bs, &a, &b, "/");
            bs_stack_push(bs, bs_value_num(a.as.number / b.as.number));
        } break;

        case BS_OP_NEG: {
            const Bs_Value a = bs_stack_pop(bs);
            if (a.type != BS_VALUE_NUM) {
                bs_error(
                    bs,
                    "invalid operand to unary (-): %s",
                    bs_value_type_name(a, bs->frame->extended));
            }

            bs_stack_push(bs, bs_value_num(-a.as.number));
        } break;

        case BS_OP_BOR: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_check_integer(bs, a, "operand #1 to binary (|)");
            bs_check_integer(bs, b, "operand #2 to binary (|)");

            const size_t a1 = a.as.number;
            const size_t b1 = b.as.number;
            bs_stack_push(bs, bs_value_num(a1 | b1));
        } break;

        case BS_OP_BAND: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_check_integer(bs, a, "operand #1 to binary (&)");
            bs_check_integer(bs, b, "operand #2 to binary (&)");

            const size_t a1 = a.as.number;
            const size_t b1 = b.as.number;
            bs_stack_push(bs, bs_value_num(a1 & b1));
        } break;

        case BS_OP_BXOR: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_check_integer(bs, a, "operand #1 to binary (^)");
            bs_check_integer(bs, b, "operand #2 to binary (^)");

            const size_t a1 = a.as.number;
            const size_t b1 = b.as.number;
            bs_stack_push(bs, bs_value_num(a1 ^ b1));
        } break;

        case BS_OP_BNOT: {
            const Bs_Value a = bs_stack_pop(bs);
            bs_check_integer(bs, a, "operand to unary (~)");

            const size_t a1 = a.as.number;
            bs_stack_push(bs, bs_value_num(~a1));
        } break;

        case BS_OP_LXOR: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_check_value_type(bs, a, BS_VALUE_BOOL, "operand #1 to binary (^^)");
            bs_check_value_type(bs, b, BS_VALUE_BOOL, "operand #2 to binary (^^)");

            const size_t a1 = a.as.boolean;
            const size_t b1 = b.as.boolean;
            bs_stack_push(bs, bs_value_bool(a1 != b1));
        } break;

        case BS_OP_LNOT:
            bs_stack_push(bs, bs_value_bool(bs_value_is_falsey(bs_stack_pop(bs))));
            break;

        case BS_OP_SHL: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_check_integer(bs, a, "operand #1 to binary (<<)");
            bs_check_integer(bs, b, "operand #2 to binary (<<)");

            const size_t a1 = a.as.number;
            const size_t b1 = b.as.number;
            bs_stack_push(bs, bs_value_num(a1 << b1));
        } break;

        case BS_OP_SHR: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_check_integer(bs, a, "operand #1 to binary (>>)");
            bs_check_integer(bs, b, "operand #2 to binary (>>)");

            const size_t a1 = a.as.number;
            const size_t b1 = b.as.number;
            bs_stack_push(bs, bs_value_num(a1 >> b1));
        } break;

        case BS_OP_GT: {
            Bs_Value a, b;
            bs_binary_op(bs, &a, &b, ">");
            bs_stack_push(bs, bs_value_bool(a.as.number > b.as.number));
        } break;

        case BS_OP_GE: {
            Bs_Value a, b;
            bs_binary_op(bs, &a, &b, ">=");
            bs_stack_push(bs, bs_value_bool(a.as.number >= b.as.number));
        } break;

        case BS_OP_LT: {
            Bs_Value a, b;
            bs_binary_op(bs, &a, &b, "<");
            bs_stack_push(bs, bs_value_bool(a.as.number < b.as.number));
        } break;

        case BS_OP_LE: {
            Bs_Value a, b;
            bs_binary_op(bs, &a, &b, "<=");
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
                bs_error(
                    bs,
                    "cannot get length of %s value",
                    bs_value_type_name(a, bs->frame->extended));
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
                size = ((Bs_Table *)a.as.object)->length;
                break;

            default:
                bs_error(
                    bs,
                    "cannot get length of %s value",
                    bs_value_type_name(a, bs->frame->extended));
            }

            bs_stack_pop(bs);
            bs_stack_push(bs, bs_value_num(size));
        } break;

        case BS_OP_JOIN: {
            const bool gc_on_save = bs->gc_on;
            bs->gc_on = false;

            Bs_Buffer *buffer = bs_buffer_get(bs);
            const size_t start = buffer->count;

            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);

            Bs_Writer w = bs_buffer_writer(buffer);
            bs_value_write(bs, &w, a);
            bs_value_write(bs, &w, b);

            const Bs_Sv sv = bs_buffer_reset(buffer, start);
            bs_stack_push(bs, bs_value_object(bs_str_new(bs, sv)));

            bs->gc_on = gc_on_save;
        } break;

        case BS_OP_IMPORT:
            bs_import(bs);
            break;

        case BS_OP_TYPEOF: {
            const char *name = bs_value_type_name(bs_stack_peek(bs, 0), bs->frame->extended);
            bs_stack_set(bs, 0, bs_value_object(bs_str_new(bs, bs_sv_from_cstr(name))));
        } break;

        case BS_OP_GDEF: {
            const Bs_Value name = bs_chunk_read_const(bs);

            if (!bs_table_set(bs, &bs->globals, name, bs_stack_peek(bs, 0))) {
                bs_error(
                    bs,
                    "redefinition of global identifier '" Bs_Sv_Fmt "'",
                    Bs_Sv_Arg(*(const Bs_Str *)name.as.object));
            }

            bs_stack_pop(bs);
        } break;

        case BS_OP_GGET: {
            const Bs_Value name = bs_chunk_read_const(bs);

            Bs_Value value;
            if (!bs_table_get(bs, &bs->globals, name, &value)) {
                bs_error(
                    bs,
                    "undefined identifier '" Bs_Sv_Fmt "'",
                    Bs_Sv_Arg(*(const Bs_Str *)name.as.object));
            }

            bs_stack_push(bs, value);
        } break;

        case BS_OP_GSET: {
            const Bs_Value name = bs_chunk_read_const(bs);
            const Bs_Value value = bs_stack_peek(bs, 0);

            if (bs_table_set(bs, &bs->globals, name, value)) {
                bs_table_remove(bs, &bs->globals, name);

                bs_error(
                    bs,
                    "undefined identifier '" Bs_Sv_Fmt "'",
                    Bs_Sv_Arg(*(const Bs_Str *)name.as.object));
            }
        } break;

        case BS_OP_LGET:
            bs_stack_push(bs, bs->frame->base[bs_chunk_read_int(bs)]);
            break;

        case BS_OP_LSET:
            bs->frame->base[bs_chunk_read_int(bs)] = bs_stack_peek(bs, 0);
            break;

        case BS_OP_UGET: {
            Bs_Upvalue *upvalue = bs->frame->closure->data[bs_chunk_read_int(bs)];
            bs_stack_push(bs, *upvalue->value);
        } break;

        case BS_OP_USET: {
            const Bs_Value value = bs_stack_peek(bs, 0);
            Bs_Upvalue *upvalue = bs->frame->closure->data[bs_chunk_read_int(bs)];
            *upvalue->value = value;
        } break;

        case BS_OP_IGET: {
            Bs_Value value;

            const Bs_Value container = bs_stack_peek(bs, 1);
            if (container.type != BS_VALUE_OBJECT) {
                bs_error(
                    bs,
                    "cannot index into %s value",
                    bs_value_type_name(container, bs->frame->extended));
            }

            const Bs_Value index = bs_stack_peek(bs, 0);
            if (container.as.object->type == BS_OBJECT_ARRAY) {
                bs_check_whole_number(bs, index, "array index");

                Bs_Array *array = (Bs_Array *)container.as.object;
                if (!bs_array_get(bs, array, index.as.number, &value)) {
                    bs_error(
                        bs,
                        "cannot get value at index %zu in array of length %zu",
                        (size_t)index.as.number,
                        array->count);
                }
            } else if (container.as.object->type == BS_OBJECT_TABLE) {
                if (index.type == BS_VALUE_NIL) {
                    bs_error(
                        bs,
                        "cannot use '%s' as table key",
                        bs_value_type_name(index, bs->frame->extended));
                }

                if (!bs_table_get(bs, (Bs_Table *)container.as.object, index, &value)) {
                    value = bs_value_nil;
                }
            } else if (container.as.object->type == BS_OBJECT_C_LIB) {
                bs_check_object_type(bs, index, BS_OBJECT_STR, "library symbol name");

                Bs_Str *name = (Bs_Str *)index.as.object;
                if (!name->size) {
                    bs_error(bs, "library symbol name cannot be empty");
                }

                Bs_C_Lib *c = (Bs_C_Lib *)container.as.object;
                if (!bs_table_get(bs, &c->functions, index, &value)) {
                    bs_error(
                        bs,
                        "symbol '" Bs_Sv_Fmt "' doesn't exist in native library '" Bs_Sv_Fmt "'",
                        Bs_Sv_Arg(*name),
                        Bs_Sv_Arg(*c->path));
                }
            } else {
                bs_error(
                    bs,
                    "cannot index into %s value",
                    bs_value_type_name(container, bs->frame->extended));
            }

            bs->stack.count -= 2;
            bs_stack_push(bs, value);
        } break;

        case BS_OP_ISET:
        case BS_OP_ILIT: {
            const Bs_Value container = bs_stack_peek(bs, 2);
            if (container.type != BS_VALUE_OBJECT) {
                bs_error(
                    bs,
                    "cannot take mutable index into %s value",
                    bs_value_type_name(container, bs->frame->extended));
            }

            const Bs_Value value = bs_stack_peek(bs, 0);
            const Bs_Value index = bs_stack_peek(bs, 1);

            if (container.as.object->type == BS_OBJECT_ARRAY) {
                bs_check_whole_number(bs, index, "array index");
                bs_array_set(bs, (Bs_Array *)container.as.object, index.as.number, value);
            } else if (container.as.object->type == BS_OBJECT_TABLE) {
                if (index.type == BS_VALUE_NIL) {
                    bs_error(
                        bs,
                        "cannot use '%s' as table key",
                        bs_value_type_name(index, bs->frame->extended));
                }

                Bs_Table *table = (Bs_Table *)container.as.object;
                if (value.type == BS_VALUE_NIL) {
                    bs_table_remove(bs, table, index);
                } else {
                    bs_table_set(bs, table, index, value);
                }
            } else {
                bs_error(
                    bs,
                    "cannot take mutable index into %s value",
                    bs_value_type_name(container, bs->frame->extended));
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

        case BS_OP_ITER: {
            const size_t offset = bs_chunk_read_int(bs);

            const Bs_Value iterator = bs_stack_peek(bs, 0);
            const Bs_Value container = bs_stack_peek(bs, 1);

            if (container.type != BS_VALUE_OBJECT) {
                bs_error(
                    bs,
                    "cannot iterate over %s value",
                    bs_value_type_name(container, bs->frame->extended));
            }

            if (container.as.object->type == BS_OBJECT_ARRAY) {
                const Bs_Array *array = (const Bs_Array *)container.as.object;

                size_t index;
                if (iterator.type == BS_VALUE_NIL) {
                    index = 0;
                } else {
                    index = iterator.as.number + 1;
                }

                if (index >= array->count) {
                    bs->frame->ip += offset;
                } else {
                    bs_stack_set(bs, 0, bs_value_num(index)); // Iterator
                    bs_stack_push(bs, bs_value_num(index));   // Key
                    bs_stack_push(bs, array->data[index]);    // Value
                }
            } else if (container.as.object->type == BS_OBJECT_TABLE) {
                const Bs_Table *table = (const Bs_Table *)container.as.object;

                size_t index;
                if (iterator.type == BS_VALUE_NIL) {
                    index = 0;
                } else {
                    index = iterator.as.number + 1;
                }

                while (index < table->capacity && table->data[index].key.type == BS_VALUE_NIL) {
                    index++;
                }

                if (index >= table->capacity) {
                    bs->frame->ip += offset;
                } else {
                    const Bs_Entry entry = table->data[index];
                    bs_stack_set(bs, 0, bs_value_num(index)); // Iterator
                    bs_stack_push(bs, entry.key);             // Key
                    bs_stack_push(bs, entry.value);           // Value
                }
            } else {
                bs_error(
                    bs,
                    "cannot iterate over %s value",
                    bs_value_type_name(container, bs->frame->extended));
            }
        } break;

        case BS_OP_RANGE: {
            const size_t offset = bs_chunk_read_int(bs);

            const Bs_Value start = bs_stack_peek(bs, 2);
            bs_check_value_type(bs, start, BS_VALUE_NUM, "range start");

            const Bs_Value end = bs_stack_peek(bs, 1);
            bs_check_value_type(bs, end, BS_VALUE_NUM, "range end");

            Bs_Value step = bs_stack_peek(bs, 0);
            if (step.type == BS_VALUE_NIL) {
                step = bs_value_num((end.as.number > start.as.number) ? 1 : -1);
                bs_stack_set(bs, 0, step);
            }
            bs_check_value_type(bs, step, BS_VALUE_NUM, "range step");

            const bool over = step.as.number > 0 ? (start.as.number >= end.as.number)
                                                 : (start.as.number <= end.as.number);
            if (over) {
                bs->frame->ip += offset;
            } else {
                bs_stack_set(bs, 2, bs_value_num(start.as.number + step.as.number));
                bs_stack_push(bs, start);
            }
        } break;

        default:
            bs_fmt(
                bs_stderr_get(bs),
                "error: invalid op %d at offset %zu\n",
                op,
                bs->frame->ip - bs->frame->closure->fn->chunk.data - 1);
        }
    }
}

Bs_Result bs_run(Bs *bs, const char *path, Bs_Sv input, bool is_repl) {
    bs->ok = true;
    bs->exit = -1;
    Bs_Result result = {0};

    Bs_Fn *fn = bs_compile(bs, path, input, true, is_repl);
    if (!fn) {
        return (Bs_Result){.exit = 1};
    }

    if (bs->step) {
        bs_debug_chunk(bs_pretty_printer(bs, bs_stdout_get(bs)), &fn->chunk);
    }

    bs_da_push(bs, &bs->modules, (Bs_Module){.name = fn->name});
    fn->module = bs->modules.count;

    if (setjmp(bs->unwind)) {
        goto end;
    }

    result.value = bs_call(bs, bs_value_object(bs_closure_new(bs, fn)), NULL, 0);

end:
    bs->stack.count = 0;

    bs->frame = NULL;
    bs->frames.count = 0;

    bs->upvalues = NULL;
    bs->gc_on = false;

    if (bs->step) {
        bs_fmt(bs_stdout_get(bs), "Stopping BS with exit code %d\n", bs->exit);
    }

    result.ok = bs->ok;
    result.exit = bs->exit;
    return result;
}

Bs_Value bs_call(Bs *bs, Bs_Value fn, const Bs_Value *args, size_t arity) {
    const size_t stack_count_save = bs->stack.count;
    const size_t frames_count_save = bs->frames.count;

    bs_stack_push(bs, fn);
    for (size_t i = 0; i < arity; i++) {
        bs_stack_push(bs, args[i]);
    }
    bs_call_value(bs, arity);

    Bs_Value result;
    if (fn.as.object->type == BS_OBJECT_C_FN) {
        result = bs_stack_peek(bs, 0);
    } else {
        bs_interpret(bs, &result);
    }

    bs->stack.count = stack_count_save;
    bs->frames.count = frames_count_save;
    bs->frame = bs->frames.count ? &bs->frames.data[bs->frames.count - 1] : NULL;
    return result;
}
