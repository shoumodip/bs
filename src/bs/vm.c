#include <errno.h>
#include <math.h>
#include <setjmp.h>

#if defined(_WIN32) || defined(_WIN64)
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>

#    define bs_issep(c) ((c) == '/' || (c) == '\\')
#else
#    include <dlfcn.h>
#    include <unistd.h>

#    define bs_issep(c) ((c) == '/')
#endif // _WIN32

#include "bs/compiler.h"
#include "bs/core.h"

// #define BS_GC_DEBUG_LOG
// #define BS_GC_DEBUG_STRESS
#define BS_GC_GROW_FACTOR 2

// #define BS_STEP_DEBUG
#ifdef BS_STEP_DEBUG
#    include "bs/debug.h"
#endif // BS_STEP_DEBUG

typedef struct {
    Bs_Value *base;
    const uint8_t *ip;

    union {
        Bs_Closure *closure;
        const Bs_C_Fn *native;
    };

    size_t locations_offset;
} Bs_Frame;

typedef struct {
    Bs_Frame *data;
    size_t count;
} Bs_Frames;

typedef struct {
    bool done;
    Bs_Value result;

    Bs_Str *name;
    size_t length;

    const char *source;
} Bs_Module;

static void bs_module_free(Bs_Module *m) {
    // The const correctness is *not* a bug. See: https://yarchive.net/comp/const.html
    free((char *)m->source);
}

typedef struct {
    Bs_Module *data;
    size_t count;
    size_t capacity;
} Bs_Modules;

static void bs_modules_free(Bs *bs, Bs_Modules *m) {
    for (size_t i = 0; i < m->count; i++) {
        bs_module_free(&m->data[i]);
    }
    bs_da_free(bs, m);
}

#define bs_modules_push bs_da_push

static bool bs_modules_find(Bs_Modules *modules, Bs_Sv name, size_t *index) {
    // Start at 1 to skip the Repl module
    for (size_t i = 1; i < modules->count; i++) {
        const Bs_Module *m = &modules->data[i];
        if (bs_sv_eq(Bs_Sv(m->name->data, m->length), name)) {
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

typedef struct {
    Bs_Object **data;
    size_t count;
    size_t capacity;
} Bs_Object_List;

static void bs_object_list_push(Bs_Object_List *l, Bs_Object *object) {
    if (l->count >= l->capacity) {
        l->capacity = l->capacity ? l->capacity * 2 : BS_DA_INIT_CAP;
        l->data = realloc(l->data, l->capacity * sizeof(Bs_Object *));
        assert(l->data);
    }

    l->data[l->count++] = object;
}

static_assert(BS_COUNT_OBJECTS == 13, "Update bs.builtin_methods");
struct Bs {
    // Stack
    Bs_Stack stack;
    Bs_Frame *frame;
    Bs_Frames frames;

    // Filesystem
    Bs_Buffer paths;
    Bs_Modules modules;

    // Methods for builtin types:
    //   BS_VALUE_NUM
    //   BS_OBJECT_STR
    //   BS_OBJECT_ARRAY
    //   BS_OBJECT_TABLE
    Bs_Map builtin_methods[4];

    // Roots
    Bs_Map globals;
    Bs_Map strings;
    Bs_Upvalue *upvalues;

    // Garbage Collector
    bool gc_on;
    size_t gc_max;
    size_t gc_bytes;

    Bs_Object_List grays;
    Bs_Object *objects;

    // Handles
    bool handles_on;
    Bs_Object_List handles;

    Bs_Config config;
    Bs_Pretty_Printer printer;

    // Exit
    bool running;
};

// Garbage collector
static_assert(BS_COUNT_OBJECTS == 13, "Update bs_free_object()");
static void bs_free_object(Bs *bs, Bs_Object *object) {
#ifdef BS_GC_DEBUG_LOG
    bs_fmt(&bs->config.log, "[GC] Free %p; Type: %d\n", object, object->type);
#endif // BS_GC_DEBUG_LOG

    switch (object->type) {
    case BS_OBJECT_FN: {
        Bs_Fn *fn = (Bs_Fn *)object;
        bs_chunk_free(bs, &fn->chunk);
        bs_realloc(bs, fn, sizeof(*fn), 0);
    } break;

    case BS_OBJECT_STR: {
        Bs_Str *str = (Bs_Str *)object;
        bs_realloc(bs, str, sizeof(*str) + str->size + 1, 0);
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

    case BS_OBJECT_CLASS: {
        Bs_Class *class = (Bs_Class *)object;
        bs_map_free(bs, &class->methods);
        bs_realloc(bs, class, sizeof(*class), 0);
    } break;

    case BS_OBJECT_INSTANCE: {
        Bs_Instance *instance = (Bs_Instance *)object;
        bs_map_free(bs, &instance->properties);
        bs_realloc(bs, instance, sizeof(*instance), 0);
    } break;

    case BS_OBJECT_C_CLASS: {
        Bs_C_Class *class = (Bs_C_Class *)object;
        bs_map_free(bs, &class->methods);
        bs_realloc(bs, class, sizeof(*class), 0);
    } break;

    case BS_OBJECT_C_INSTANCE: {
        Bs_C_Instance *instance = (Bs_C_Instance *)object;
        if (instance->class->free) {
            instance->class->free(bs->config.userdata, instance->data);
        }
        bs_realloc(bs, instance, sizeof(*instance) + instance->class->size, 0);
    } break;

    case BS_OBJECT_BOUND_METHOD: {
        Bs_Bound_Method *method = (Bs_Bound_Method *)object;
        bs_realloc(bs, method, sizeof(*method), 0);
    } break;

    case BS_OBJECT_C_FN:
        bs_realloc(bs, object, sizeof(Bs_C_Fn), 0);
        break;

    case BS_OBJECT_C_LIB: {
        Bs_C_Lib *library = (Bs_C_Lib *)object;
        bs_map_free(bs, &library->map);

#if defined(_WIN32) || defined(_WIN64)
        FreeLibrary(library->handle);
#else
        dlclose(library->handle);
#endif // _WIN32
        bs_realloc(bs, library, sizeof(*library), 0);
    } break;

    default:
        assert(false && "unreachable");
    }
}

static void bs_mark_value(Bs *bs, Bs_Value value) {
    if (value.type == BS_VALUE_OBJECT) {
        bs_mark(bs, value.as.object);
    }
}

static void bs_mark_map(Bs *bs, Bs_Map *map) {
    for (size_t i = 0; i < map->capacity; i++) {
        bs_mark_value(bs, map->data[i].key);
        bs_mark_value(bs, map->data[i].value);
    }
}

static_assert(BS_COUNT_OBJECTS == 13, "Update bs_blacken_object()");
static void bs_blacken_object(Bs *bs, Bs_Object *object) {
#ifdef BS_GC_DEBUG_LOG
    Bs_Writer *w = &bs->config.log;
    bs_fmt(w, "[GC] Blacken %p ", object);
    bs_value_write(bs, w, bs_value_object(object));
    bs_fmt(w, "\n");
#endif // BS_GC_DEBUG_LOG

    switch (object->type) {
    case BS_OBJECT_FN: {
        Bs_Fn *fn = (Bs_Fn *)object;
        bs_mark(bs, (Bs_Object *)fn->name);
        bs_mark(bs, (Bs_Object *)fn->source);

        for (size_t i = 0; i < fn->chunk.constants.count; i++) {
            bs_mark_value(bs, fn->chunk.constants.data[i]);
        }
    } break;

    case BS_OBJECT_STR:
    case BS_OBJECT_C_FN:
        break;

    case BS_OBJECT_ARRAY: {
        Bs_Array *array = (Bs_Array *)object;
        for (size_t i = 0; i < array->count; i++) {
            bs_mark_value(bs, array->data[i]);
        }
    } break;

    case BS_OBJECT_TABLE: {
        Bs_Table *table = (Bs_Table *)object;
        bs_mark_map(bs, &table->map);
    } break;

    case BS_OBJECT_CLOSURE: {
        Bs_Closure *closure = (Bs_Closure *)object;
        bs_mark(bs, (Bs_Object *)closure->fn);

        for (size_t i = 0; i < closure->upvalues; i++) {
            bs_mark(bs, (Bs_Object *)closure->data[i]);
        }
    } break;

    case BS_OBJECT_UPVALUE:
        bs_mark_value(bs, ((Bs_Upvalue *)object)->closed);
        break;

    case BS_OBJECT_CLASS: {
        Bs_Class *class = (Bs_Class *)object;
        bs_mark(bs, (Bs_Object *)class->name);
        bs_mark(bs, (Bs_Object *)class->init);
        bs_mark_map(bs, &class->methods);
    } break;

    case BS_OBJECT_INSTANCE: {
        Bs_Instance *instance = (Bs_Instance *)object;
        bs_mark(bs, (Bs_Object *)instance->class);
        bs_mark_map(bs, &instance->properties);
    } break;

    case BS_OBJECT_C_CLASS: {
        Bs_C_Class *class = (Bs_C_Class *)object;
        bs_mark(bs, (Bs_Object *)class->init);
        bs_mark_map(bs, &class->methods);
    } break;

    case BS_OBJECT_C_INSTANCE: {
        Bs_C_Instance *instance = (Bs_C_Instance *)object;
        bs_mark(bs, (Bs_Object *)instance->class);
        if (instance->class->mark) {
            instance->class->mark(bs, instance->data);
        }
    } break;

    case BS_OBJECT_BOUND_METHOD: {
        Bs_Bound_Method *method = (Bs_Bound_Method *)object;
        bs_mark_value(bs, method->this);
        bs_mark_value(bs, method->fn);
    } break;

    case BS_OBJECT_C_LIB: {
        Bs_C_Lib *library = ((Bs_C_Lib *)object);
        bs_mark_map(bs, &library->map);
    } break;

    default:
        assert(false && "unreachable");
    }
}

static void bs_collect(Bs *bs) {
    const bool gc_on_save = bs->gc_on;
    bs->gc_on = false;

#ifdef BS_GC_DEBUG_LOG
    Bs_Writer *w = &bs->config.log;
    bs_fmt(w, "\n-------- GC Begin --------\n");
    const size_t before = bs->gc_bytes;
#endif // BS_GC_DEBUG_LOG

    // Mark
    for (size_t i = 0; i < bs->stack.count; i++) {
        bs_mark_value(bs, bs->stack.data[i]);
    }

    for (size_t i = 0; i < bs->frames.count; i++) {
        bs_mark(bs, (Bs_Object *)bs->frames.data[i].closure);
    }

    bs_mark(bs, (Bs_Object *)bs->config.cwd);

    for (size_t i = 0; i < bs->modules.count; i++) {
        Bs_Module *m = &bs->modules.data[i];
        bs_mark(bs, (Bs_Object *)m->name);
        bs_mark_value(bs, m->result);
    }

    for (Bs_Upvalue *upvalue = bs->upvalues; upvalue; upvalue = upvalue->next) {
        bs_mark(bs, (Bs_Object *)upvalue);
    }

    bs_mark_map(bs, &bs->globals);

    for (size_t i = 0; i < bs->handles.count; i++) {
        bs_mark(bs, bs->handles.data[i]);
    }

    for (size_t i = 0; i < bs_c_array_size(bs->builtin_methods); i++) {
        bs_mark_map(bs, &bs->builtin_methods[i]);
    }

    // Trace
    while (bs->grays.count) {
        Bs_Object *object = bs->grays.data[--bs->grays.count];
        bs_blacken_object(bs, object);
    }

    // Sweep
    for (size_t i = 0; i < bs->strings.capacity; i++) {
        Bs_Entry *entry = &bs->strings.data[i];
        if (entry->key.type != BS_VALUE_NIL && !entry->key.as.object->marked) {
            bs_map_remove(bs, &bs->strings, entry->key);
        }
    }

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
void bs_buffer_write(Bs_Writer *w, Bs_Sv sv) {
    Bs_Buffer *b = w->data;
    bs_da_push_many(b->bs, b, sv.data, sv.size);
}

Bs *bs_new(int argc, char **argv) {
    Bs *bs = calloc(1, sizeof(Bs));
    assert(bs);

    bs->stack.data = malloc(BS_STACK_CAPACITY * sizeof(*bs->stack.data));
    assert(bs->stack.data);

    bs->frames.data = malloc(BS_FRAMES_CAPACITY * sizeof(*bs->frames.data));
    assert(bs->frames.data);

    bs->gc_max = 1024 * 1024;

    bs->paths.bs = bs;
    bs->config.buffer.bs = bs;

    bs->config.log = bs_file_writer(stdout);
    bs->config.error.write = bs_error_write_default;

    bs_update_cwd(bs);
    bs_core_init(bs, argc, argv);

    // The Repl module
    bs_modules_push(bs, &bs->modules, (Bs_Module){0});
    return bs;
}

void bs_free(Bs *bs) {
    free(bs->stack.data);
    memset(&bs->stack, 0, sizeof(bs->stack));

    free(bs->frames.data);
    memset(&bs->frames, 0, sizeof(bs->frames));

    bs_modules_free(bs, &bs->modules);

    bs_map_free(bs, &bs->globals);
    bs_map_free(bs, &bs->strings);

    for (size_t i = 0; i < bs_c_array_size(bs->builtin_methods); i++) {
        bs_map_free(bs, &bs->builtin_methods[i]);
    }

    Bs_Object *object = bs->objects;
    while (object) {
        Bs_Object *next = object->next;
        bs_free_object(bs, object);
        object = next;
    }
    free(bs->grays.data);
    free(bs->handles.data);

    bs_da_free(bs, &bs->paths);
    bs_da_free(bs, &bs->config.buffer);

    bs_pretty_printer_free(&bs->printer);
    free(bs);
}

void bs_mark(Bs *bs, Bs_Object *object) {
    if (!object || object->marked) {
        return;
    }

#ifdef BS_GC_DEBUG_LOG
    Bs_Writer *w = &bs->config.log;
    bs_fmt(w, "[GC] Mark %p ", object);
    bs_value_write(bs, w, bs_value_object(object));
    bs_fmt(w, "\n");
#endif // BS_GC_DEBUG_LOG

    object->marked = true;
    bs_object_list_push(&bs->grays, object);
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

Bs_Object *bs_object_new(Bs *bs, Bs_Object_Type type, size_t size) {
    Bs_Object *object = bs_realloc(bs, NULL, 0, size);
    assert(object);

    object->type = type;
    object->next = bs->objects;
    object->marked = false;
    bs->objects = object;

#ifdef BS_GC_DEBUG_LOG
    bs_fmt(&bs->config.log, "[GC] Allocate %p (%zu bytes); Type: %d\n", object, size, type);
#endif // BS_GC_DEBUG_LOG

    if (bs->handles_on) {
        bs_object_list_push(&bs->handles, object);
    }

    return object;
}

Bs_Str *bs_str_new(Bs *bs, Bs_Sv sv) {
    const uint32_t hash = bs_hash_bytes(sv.data, sv.size);
    Bs_Entry *entry = bs_entries_find_sv(bs->strings.data, bs->strings.capacity, sv, hash);
    if (entry && entry->key.type != BS_VALUE_NIL) {
        return (Bs_Str *)entry->key.as.object;
    }

    Bs_Str *str = (Bs_Str *)bs_object_new(bs, BS_OBJECT_STR, sizeof(Bs_Str) + sv.size + 1);
    str->size = sv.size;
    str->hash = hash;

    memcpy(str->data, sv.data, sv.size);
    str->data[str->size] = '\0';

    const bool gc_on_save = bs->gc_on;
    bs->gc_on = false;

    // Intern it
    bs_map_set(bs, &bs->strings, bs_value_object(str), bs_value_nil);

    bs->gc_on = gc_on_save;
    return str;
}

void bs_global_set(Bs *bs, Bs_Sv name, Bs_Value value) {
    bs_map_set(bs, &bs->globals, bs_value_object(bs_str_new(bs, name)), value);
}

bool bs_update_cwd(Bs *bs) {
    bool result = true;

    const bool gc_on_save = bs->gc_on;
    bs->gc_on = false;

    Bs_Buffer *b = &bs->paths;
    const size_t start = b->count;

#if defined(_WIN32) || defined(_WIN64)
    const DWORD size = GetCurrentDirectory(0, NULL);
    if (!size) {
        bs_return_defer(false);
    }

    bs_da_push_many(bs, b, NULL, size);
    if (!GetCurrentDirectory(size, b->data + start)) {
        bs_return_defer(false);
    }
#else
    bs_da_push_many(bs, b, NULL, BS_DA_INIT_CAP);

    while (!getcwd(b->data + start, b->capacity - start)) {
        if (errno != ERANGE) {
            bs_return_defer(false);
        }

        b->count = b->capacity;
        bs_da_push_many(bs, b, NULL, BS_DA_INIT_CAP);
    }
#endif

    bs->config.cwd = bs_str_new(bs, bs_sv_from_cstr(b->data + start));

defer:
    b->count = start;
    bs->gc_on = gc_on_save;
    return result;
}

static Bs_Pretty_Printer *bs_pretty_printer(Bs *bs, Bs_Writer *w) {
    bs->printer.writer = w;
    bs->printer.depth = 0;
    bs->printer.count = 0;
    return &bs->printer;
}

void bs_value_write(Bs *bs, Bs_Writer *w, Bs_Value value) {
    bs_value_write_impl(bs_pretty_printer(bs, w), value);
}

static Bs_Map *bs_builtin_number_methods_map(Bs *bs) {
    return &bs->builtin_methods[0];
}

static Bs_Map *bs_builtin_object_methods_map(Bs *bs, Bs_Object_Type type) {
    // The rest are internal and/or have methods already
    assert(type > BS_OBJECT_FN && type <= BS_OBJECT_TABLE);
    return &bs->builtin_methods[type];
}

void bs_builtin_number_methods_add(Bs *bs, Bs_Sv name, Bs_C_Fn_Ptr ptr) {
    bs_map_set(
        bs,
        bs_builtin_number_methods_map(bs),
        bs_value_object(bs_str_new(bs, name)),
        bs_value_object(bs_c_fn_new(bs, name, ptr)));
}

void bs_builtin_object_methods_add(Bs *bs, Bs_Object_Type type, Bs_Sv name, Bs_C_Fn_Ptr ptr) {
    bs_map_set(
        bs,
        bs_builtin_object_methods_map(bs, type),
        bs_value_object(bs_str_new(bs, name)),
        bs_value_object(bs_c_fn_new(bs, name, ptr)));
}

// Buffer
Bs_Writer bs_buffer_writer(Bs_Buffer *b) {
    return (Bs_Writer){.data = b, .write = bs_buffer_write};
}

Bs_Sv bs_buffer_reset(Bs_Buffer *b, size_t pos) {
    assert(pos <= b->count);
    const Bs_Sv sv = {b->data + pos, b->count - pos};
    b->count = pos;
    return sv;
}

Bs_Sv bs_buffer_absolute_path(Bs_Buffer *b, Bs_Sv path) {
    const size_t start = b->count;

    if (path.size && !bs_issep(*path.data)) {
        bs_da_push_many(b->bs, b, b->bs->config.cwd->data, b->bs->config.cwd->size);
        if (b->count != start + 1) {
            bs_da_push(b->bs, b, '/');
        }
    }

    bs_da_push_many(b->bs, b, path.data, path.size);
    bs_da_push(b->bs, b, '\0');

    const char *p = b->data + start;
    char *r = b->data + start;

    while (*p) {
        // Skip consecutive slashes
        while (bs_issep(*p) && bs_issep(p[1])) {
            p++;
        }

        // Handle ./
        if (*p == '.' && (bs_issep(p[1]) || p[1] == '\0')) {
            p++;
            if (bs_issep(*p)) {
                p++;
            }
            continue;
        }

        // Handle ../
        if (*p == '.' && p[1] == '.' && (bs_issep(p[2]) || p[2] == '\0')) {
            p += 2;
            if (bs_issep(*p)) {
                p++;
            }

            if (r != b->data + start + 1) {
                r--;
                while (r != b->data + start && !bs_issep(r[-1])) {
                    r--;
                }
            }
            continue;
        }

        // Normal
        while (!bs_issep(*p) && *p != '\0') {
            *r++ = *p++;
        }

        if (bs_issep(*p)) {
            *r++ = '/'; // Assert dominance
            p++;
        }
    }

    b->count = r - b->data;
    return (Bs_Sv){b->data + start, b->count - start};
}

Bs_Sv bs_buffer_relative_path(Bs_Buffer *b, Bs_Sv path) {
    const size_t start = b->count;

    const Bs_Str *cwd = b->bs->config.cwd;
    const size_t max = bs_min(path.size, cwd->size);

    size_t i = 0;
    if (!bs_sv_eq(Bs_Sv(cwd->data, cwd->size), Bs_Sv_Static("/"))) {
        while (i < max) {
            if (path.data[i] != cwd->data[i]) {
                // More video game OS shenanigans
                if (!bs_issep(path.data[i]) || !bs_issep(cwd->data[i])) {
                    break;
                }
            }

            i++;
        }

        if (i != cwd->size || !bs_issep(path.data[i])) {
            bs_da_push_many(b->bs, b, "../", 3);
            for (size_t j = i; j < cwd->size; j++) {
                if (bs_issep(cwd->data[j])) {
                    bs_da_push_many(b->bs, b, "../", 3);
                }
            }

            while (i && !bs_issep(path.data[i - 1])) {
                i--;
            }
        } else {
            i++;
        }
    }

    bs_da_push_many(b->bs, b, path.data + i, path.size - i);
    return Bs_Sv(b->data + start, b->count - start);
}

// Config
Bs_Unwind bs_unwind_save(Bs *bs) {
    Bs_Unwind u = bs->config.unwind;
    u.stack_count = bs->stack.count;
    u.frames_count = bs->frames.count;
    return u;
}

void bs_unwind_restore(Bs *bs, const Bs_Unwind *u) {
    bs->config.unwind = *u;
    bs->stack.count = u->stack_count;
    bs->frames.count = u->frames_count;
    bs->frame = bs->frames.count ? &bs->frames.data[bs->frames.count - 1] : NULL;
}

Bs_Config *bs_config(Bs *bs) {
    return &bs->config;
}

// Modules
size_t bs_modules_count(Bs *bs) {
    assert(bs->modules.count);
    return bs->modules.count - 1;
}

Bs_Sv bs_modules_get_name(Bs *bs, size_t index) {
    index++; // Skip the repl

    assert(index < bs->modules.count);
    const Bs_Module *m = &bs->modules.data[index];
    return Bs_Sv(m->name->data, m->length);
}

void bs_modules_unload(Bs *bs, size_t index) {
    index++; // Skip the repl

    assert(index < bs->modules.count);
    bs_module_free(&bs->modules.data[index]);

    for (size_t i = index; i + 1 < bs->modules.count; i++) {
        bs->modules.data[i] = bs->modules.data[i + 1];
    }
    bs->modules.count--;
}

// Errors
static Bs_Op_Loc *bs_chunk_get_op_loc(const Bs_Chunk *c, size_t op_index) {
    for (size_t i = 0; i < c->locations.count; i++) {
        if (c->locations.data[i].index == op_index) {
            return &c->locations.data[i];
        }
    }

    assert(false && "unreachable");
}

void bs_unwind(Bs *bs, unsigned char exit) {
    bs->config.unwind.exit = exit;
    longjmp(bs->config.unwind.point, 1);
}

static bool bs_value_has_builtin_methods(Bs_Value value) {
    switch (value.type) {
    case BS_VALUE_NIL:
    case BS_VALUE_BOOL:
        return false;

    case BS_VALUE_NUM:
        return true;

    case BS_VALUE_OBJECT:
        switch (value.as.object->type) {
        case BS_OBJECT_STR:
        case BS_OBJECT_ARRAY:
        case BS_OBJECT_TABLE:
            return true;

        default:
            return false;
        }
    }
}

Bs_Error bs_error_begin_at(Bs *bs, size_t location) {
    fflush(stdout); // Flush stdout beforehand *just in case*
    bs->gc_on = false;

    Bs_Error error = {
        .type = BS_ERROR_MAIN,
        .continued = bs->frames.count > 1,
    };

    if (bs->frame->ip) {
        const Bs_Op_Loc *oploc = bs_chunk_get_op_loc(
            &bs->frame->closure->fn->chunk, bs->frame->ip - bs->frame->closure->fn->chunk.data);

        error.loc = oploc[location].loc;
    } else {
        error.native = true;
        if (bs->frames.data[bs->frames.count - 2].ip) {
            const Bs_Frame *prev = &bs->frames.data[bs->frames.count - 2];
            const Bs_Op_Loc *oploc = bs_chunk_get_op_loc(
                &prev->closure->fn->chunk, prev->ip - prev->closure->fn->chunk.data);

            error.native = false;
            error.loc = oploc[location + bs->frame->locations_offset].loc;

            error.continued = bs->frames.count > 2;
        } else if (error.continued) {
            error.continued = false;
        }
    }

    return error;
}

void bs_error_end_at(Bs *bs, size_t location, bool native) {
    const size_t start = bs->config.buffer.count;
    Bs_Writer w = bs_buffer_writer(&bs->config.buffer);

    for (size_t i = bs->frames.count; i > 1; i--) {
        const Bs_Frame *callee = &bs->frames.data[i - 1];
        const Bs_Frame *caller = &bs->frames.data[i - 2];
        if (!callee->ip && caller->ip && i == bs->frames.count) {
            continue;
        }

        if (!caller->ip && callee->ip && callee->closure->fn->source) {
            continue;
        }

        Bs_Error error = {.type = BS_ERROR_TRACE};

        if (caller->ip) {
            const Bs_Fn *fn = caller->closure->fn;
            const Bs_Op_Loc *oploc = bs_chunk_get_op_loc(&fn->chunk, caller->ip - fn->chunk.data);
            if (native) {
                oploc += location;
            }

            oploc += callee->locations_offset;
            error.loc = oploc->loc;
        } else {
            error.native = true;
        }
        native = false;

        if (callee->ip) {
            if (callee->closure->fn->module) {
                const Bs_Str *module = callee->closure->fn->name;

                Bs_Sv sv = Bs_Sv(module->data, module->size);
                if (bs_sv_suffix(sv, Bs_Sv_Static(".bs"))) {
                    sv.size -= 3;
                }

                bs_fmt(&w, "import(\"");
                while (sv.size) {
                    size_t index;
                    if (bs_sv_find(sv, '"', &index)) {
                        w.write(&w, bs_sv_drop(&sv, index));
                        w.write(&w, Bs_Sv_Static("\\\""));
                        bs_sv_drop(&sv, 1);
                    } else {
                        w.write(&w, bs_sv_drop(&sv, sv.size));
                    }
                }
                bs_fmt(&w, "\")");
            } else if (callee->closure->fn->name) {
                const Bs_Value this = *callee->base;
                if (this.type == BS_VALUE_OBJECT && this.as.object->type == BS_OBJECT_INSTANCE) {
                    const Bs_Instance *instance = (const Bs_Instance *)this.as.object;
                    if (callee->closure != instance->class->init) {
                        bs_fmt(&w, Bs_Sv_Fmt ".", Bs_Sv_Arg(*instance->class->name));
                    }
                } else if (bs_value_has_builtin_methods(this)) {
                    const Bs_Sv sv = bs_value_type_name_full(this);
                    bs_fmt(&w, Bs_Sv_Fmt ".", Bs_Sv_Arg(sv));
                }

                bs_fmt(&w, Bs_Sv_Fmt "()", Bs_Sv_Arg(*callee->closure->fn->name));
            } else {
                bs_fmt(&w, "<anonymous>()");
            }
        } else {
            const Bs_C_Fn *fn = callee->native;
            const Bs_Value this = callee->base[-1];
            if (this.type == BS_VALUE_OBJECT && this.as.object->type == BS_OBJECT_C_INSTANCE) {
                const Bs_C_Instance *instance = (const Bs_C_Instance *)this.as.object;
                if (fn != instance->class->init) {
                    bs_fmt(&w, Bs_Sv_Fmt ".", Bs_Sv_Arg(instance->class->name));
                }
            } else if (bs_value_has_builtin_methods(this)) {
                const Bs_Sv sv = bs_value_type_name_full(this);
                bs_fmt(&w, Bs_Sv_Fmt ".", Bs_Sv_Arg(sv));
            }

            bs_fmt(&w, Bs_Sv_Fmt "()", Bs_Sv_Arg(fn->name));
        }

        error.message = bs_buffer_reset(&bs->config.buffer, start);
        error.continued = i > 2;
        if (error.continued && error.native && !caller->ip) {
            error.continued = true;
        }

        bs->config.error.write(&bs->config.error, error);
    }

    bs->config.unwind.ok = false;
    bs_unwind(bs, 1);
}

void bs_error_full_at(
    Bs *bs, size_t location, Bs_Sv explanation, Bs_Sv example, const char *fmt, ...) {
    const size_t start = bs->config.buffer.count;
    Bs_Writer w = bs_buffer_writer(&bs->config.buffer);

    va_list args;
    va_start(args, fmt);
    bs_vfmt(&w, fmt, args);
    va_end(args);

    Bs_Error error = bs_error_begin_at(bs, location);
    error.message = bs_buffer_reset(&bs->config.buffer, start);
    error.explanation = explanation;
    error.example = example;

    bs->config.error.write(&bs->config.error, error);
    bs_error_end_at(bs, location, error.native);
}

void bs_error_standalone(Bs *bs, const char *fmt, ...) {
    const size_t start = bs->config.buffer.count;
    Bs_Writer w = bs_buffer_writer(&bs->config.buffer);

    va_list args;
    va_start(args, fmt);
    bs_vfmt(&w, fmt, args);
    va_end(args);

    const Bs_Error error = {
        .type = BS_ERROR_STANDALONE,
        .message = bs_buffer_reset(&bs->config.buffer, start),
    };
    bs->config.error.write(&bs->config.error, error);
}

// Checks
void bs_check_arity_at(Bs *bs, size_t location, size_t actual, size_t expected) {
    if (actual != expected) {
        bs_error_at(
            bs,
            location,
            "expected %zu argument%s, got %zu",
            expected,
            expected == 1 ? "" : "s",
            actual);
    }
}

void bs_check_multi_at(
    Bs *bs,
    size_t location,
    Bs_Value value,
    const Bs_Check *checks,
    size_t count,
    const char *label) {

    if (!count) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        const Bs_Check c = checks[i];
        switch (c.type) {
        case BS_CHECK_VALUE:
            if (value.type == c.as.value) {
                return;
            }
            break;

        case BS_CHECK_OBJECT:
            if (value.type == BS_VALUE_OBJECT && value.as.object->type == c.as.object) {
                return;
            }
            break;

        case BS_CHECK_C_INSTANCE:
            if (value.type == BS_VALUE_OBJECT && value.as.object->type == BS_OBJECT_C_INSTANCE) {
                if (((Bs_C_Instance *)value.as.object)->class == c.as.c_instance) {
                    return;
                }
            }
            break;

        case BS_CHECK_FN:
            if (value.type == BS_VALUE_OBJECT) {
                switch (value.as.object->type) {
                case BS_OBJECT_FN:
                case BS_OBJECT_CLOSURE:
                case BS_OBJECT_BOUND_METHOD:
                case BS_OBJECT_C_FN:
                case BS_OBJECT_CLASS:
                case BS_OBJECT_C_CLASS:
                    return;

                default:
                    break;
                }
            }
            break;

        case BS_CHECK_INT:
            if (value.type == BS_VALUE_NUM && value.as.number == (long)value.as.number) {
                return;
            }
            break;

        case BS_CHECK_WHOLE:
            if (value.type == BS_VALUE_NUM && value.as.number >= 0 &&
                value.as.number == (long)value.as.number) {
                return;
            }
            break;

        case BS_CHECK_ASCII:
            if (value.type == BS_VALUE_NUM && value.as.number == (long)value.as.number &&
                value.as.number >= 0 && value.as.number <= 127) {
                return;
            }
            break;

        default:
            assert(false && "unreachable");
        }
    }

    const size_t start = bs->config.buffer.count;
    Bs_Writer w = bs_buffer_writer(&bs->config.buffer);

    if (label) {
        bs_fmt(&w, "expected %s to be ", label);
    } else {
        bs_fmt(&w, "expected argument #%zu to be ", location);
    }

    for (size_t i = 0; i < count; i++) {
        if (i) {
            if (i + 1 == count) {
                bs_fmt(&w, " or ");
            } else {
                bs_fmt(&w, ", ");
            }
        }

        const Bs_Check c = checks[i];
        switch (c.type) {
        case BS_CHECK_VALUE:
            bs_fmt(&w, "%s", bs_value_type_name(c.as.value));
            break;

        case BS_CHECK_OBJECT:
            bs_fmt(&w, "%s", bs_object_type_name(c.as.object));
            break;

        case BS_CHECK_C_INSTANCE:
            bs_fmt(&w, Bs_Sv_Fmt, Bs_Sv_Arg(c.as.c_instance->name));
            break;

        case BS_CHECK_FN:
            bs_fmt(&w, "function");
            break;

        case BS_CHECK_INT:
            bs_fmt(&w, "integer");
            break;

        case BS_CHECK_WHOLE:
            bs_fmt(&w, "positive integer");
            break;

        case BS_CHECK_ASCII:
            bs_fmt(&w, "ASCII code (0 to 127)");
            break;

        default:
            assert(false && "unreachable");
        }
    }

    const Bs_Sv sv = bs_value_type_name_full(value);
    bs_fmt(&w, ", got " Bs_Sv_Fmt, Bs_Sv_Arg(sv));

    Bs_Error error = bs_error_begin_at(bs, location);
    error.message = bs_buffer_reset(&bs->config.buffer, start);

    bs->config.error.write(&bs->config.error, error);
    bs_error_end_at(bs, location, error.native);
}

void bs_check_callable_at(Bs *bs, size_t location, Bs_Value value, const char *label) {
    const Bs_Check check = bs_check_fn;
    bs_check_multi_at(bs, location, value, &check, 1, label);
}

void bs_check_value_type_at(
    Bs *bs, size_t location, Bs_Value value, Bs_Value_Type expected, const char *label) {
    const Bs_Check check = bs_check_value(expected);
    bs_check_multi_at(bs, location, value, &check, 1, label);
}

void bs_check_object_type_at(
    Bs *bs, size_t location, Bs_Value value, Bs_Object_Type expected, const char *label) {
    const Bs_Check check = bs_check_object(expected);
    bs_check_multi_at(bs, location, value, &check, 1, label);
}

void bs_check_integer_at(Bs *bs, size_t location, Bs_Value value, const char *label) {
    const Bs_Check check = bs_check_int;
    bs_check_multi_at(bs, location, value, &check, 1, label);
}

void bs_check_whole_number_at(Bs *bs, size_t location, Bs_Value value, const char *label) {
    const Bs_Check check = bs_check_whole;
    bs_check_multi_at(bs, location, value, &check, 1, label);
}

void bs_check_ascii_code_at(Bs *bs, size_t location, Bs_Value value, const char *label) {
    const Bs_Check check = bs_check_ascii;
    bs_check_multi_at(bs, location, value, &check, 1, label);
}

// Interpreter
static void bs_stack_push(Bs *bs, Bs_Value value) {
    if (bs->stack.count >= BS_STACK_CAPACITY) {
        bs_error(bs, "stack overflow");
    }

    bs->stack.data[bs->stack.count++] = value;
}

static void bs_frames_push(Bs *bs, Bs_Frame frame) {
    if (bs->frames.count >= BS_FRAMES_CAPACITY) {
        bs_error(bs, "call stack overflow");
    }

    bs->frames.data[bs->frames.count++] = frame;
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
        const Bs_Sv s1 = bs_value_type_name_full(*a);
        const Bs_Sv s2 = bs_value_type_name_full(*b);

        bs_error(
            bs,
            "invalid operands to binary (%s): " Bs_Sv_Fmt ", " Bs_Sv_Fmt,
            op,
            Bs_Sv_Arg(s1),
            Bs_Sv_Arg(s2));
    }
}

static void bs_call_c_fn(Bs *bs, size_t offset, const Bs_C_Fn *native, size_t arity) {
    const Bs_Frame frame = {
        .base = &bs->stack.data[bs->stack.count - arity],
        .native = native,
        .locations_offset = offset,
    };

    bs_frames_push(bs, frame);
    bs->frame = &bs->frames.data[bs->frames.count - 1];

    const Bs_Value value = native->ptr(bs, &bs->stack.data[bs->stack.count - arity], arity);
    bs->frames.count--;
    bs->frame = &bs->frames.data[bs->frames.count - 1];

    bs->stack.count -= arity;
    bs->stack.data[bs->stack.count - 1] = value;
}

static void bs_call_closure(Bs *bs, size_t offset, Bs_Closure *closure, size_t arity) {
    bs_check_arity_at(bs, offset, arity, closure->fn->arity);

    const Bs_Frame frame = {
        .base = &bs->stack.data[bs->stack.count - arity - 1],
        .ip = closure->fn->chunk.data,
        .closure = closure,
        .locations_offset = offset,
    };

    bs_frames_push(bs, frame);
    bs->frame = &bs->frames.data[bs->frames.count - 1];
}

static_assert(BS_COUNT_OBJECTS == 13, "Update bs_call_value()");
static void bs_call_value(Bs *bs, size_t offset, Bs_Value value, size_t arity) {
    if (value.type != BS_VALUE_OBJECT) {
        const Bs_Sv sv = bs_value_type_name_full(value);
        bs_error_at(bs, offset, "cannot call " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
    }

    switch (value.as.object->type) {
    case BS_OBJECT_FN:
        bs_error_at(bs, offset, "cannot call raw function directly, wrap it in a closure first");
        break;

    case BS_OBJECT_CLOSURE:
        bs_call_closure(bs, offset, (Bs_Closure *)value.as.object, arity);
        break;

    case BS_OBJECT_CLASS: {
        Bs_Class *class = (Bs_Class *)value.as.object;
        bs->stack.data[bs->stack.count - arity - 1] = bs_value_object(bs_instance_new(bs, class));

        if (class->init) {
            bs_call_closure(bs, offset, class->init, arity);
        } else if (arity) {
            bs_check_arity_at(bs, offset, arity, 0);
        }
    } break;

    case BS_OBJECT_C_CLASS: {
        Bs_C_Class *class = (Bs_C_Class *)value.as.object;
        Bs_Value instance = bs_value_object(bs_c_instance_new(bs, class));
        bs->stack.data[bs->stack.count - arity - 1] = instance;

        if (class->init) {
            const bool handles_on_save = bs->handles_on;
            const size_t handles_count_save = bs->handles.count;
            bs->handles_on = true;

            bs_call_c_fn(bs, offset, class->init, arity);

            // Failable classes can return either nil or the instance
            if (!class->can_fail || bs_stack_peek(bs, 0).type != BS_VALUE_NIL) {
                bs_stack_set(bs, 0, instance);
            }

            bs->handles_on = handles_on_save;
            bs->handles.count = handles_count_save;
        } else {
            bs_check_arity_at(bs, offset, arity, 0);
        }
    } break;

    case BS_OBJECT_BOUND_METHOD: {
        Bs_Bound_Method *method = (Bs_Bound_Method *)value.as.object;
        bs->stack.data[bs->stack.count - arity - 1] = method->this;
        bs_call_value(bs, offset, method->fn, arity);
    } break;

    case BS_OBJECT_C_FN: {
        const Bs_C_Fn *native = (Bs_C_Fn *)value.as.object;

        const bool handles_on_save = bs->handles_on;
        const size_t handles_count_save = bs->handles.count;
        bs->handles_on = true;

        bs_call_c_fn(bs, offset, native, arity);

        bs->handles_on = handles_on_save;
        bs->handles.count = handles_count_save;
    } break;

    default: {
        const Bs_Sv sv = bs_value_type_name_full(value);
        bs_error_at(bs, offset, "cannot call " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
    } break;
    }
}

static void bs_call_stack_top(Bs *bs, size_t arity) {
    bs_call_value(bs, 0, bs_stack_peek(bs, arity), arity);
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

const Bs_Closure *bs_compile_module(Bs *bs, Bs_Sv path, Bs_Sv input, bool is_main, bool is_repl) {
    Bs_Module module = {
        .name = bs_str_new(bs, path),
        .length = path.size,
    };

    // The main and repl contents are owned outside of BS. Don't take ownership over those
    if (!is_main && !is_repl) {
        module.source = input.data;
    }

    if (bs_sv_suffix(path, Bs_Sv_Static(".bs"))) {
        module.length -= 3;
    }

    // Do not use the paramater 'path' directly as the same buffer is being written into
    const Bs_Sv relative =
        bs_buffer_relative_path(&bs->paths, Bs_Sv(module.name->data, module.name->size));

    Bs_Closure *closure = bs_compile(bs, relative, input, is_main, is_repl, false);
    if (!closure) {
        return NULL;
    }

    if (is_repl) {
        bs->modules.data[0] = module;
        closure->fn->module = 1;
    } else {
        bs_modules_push(bs, &bs->modules, module);
        closure->fn->module = bs->modules.count;
    }
    return closure;
}

static bool bs_import_language(Bs *bs, Bs_Sv path) {
    size_t size = 0;
    char *contents = bs_read_file(path.data, &size, false);
    if (!contents) {
        return false;
    }

    path.size--;
    const Bs_Closure *fn = bs_compile_module(bs, path, Bs_Sv(contents, size), false, false);

    if (!fn) {
        bs_unwind(bs, 1);
    }

#ifdef BS_STEP_DEBUG
    bs_debug_chunk(bs_pretty_printer(bs, &bs->config.log), &fn->fn->chunk);
#endif // BS_STEP_DEBUG

    bs_stack_push(bs, bs_value_object(fn));
    bs_call_stack_top(bs, 0);
    return true;
}

static void bs_import(Bs *bs) {
    const bool gc_on_save = bs->gc_on;
    bs->gc_on = false;

    const bool handles_on_save = bs->handles_on;
    bs->handles_on = false;

    const Bs_Value a = bs_stack_pop(bs);
    bs_check_object_type(bs, a, BS_OBJECT_STR, "module name");

    const Bs_Str *path = (const Bs_Str *)a.as.object;
    if (!path->size) {
        bs_error(bs, "module name cannot be empty");
    }

    Bs_Buffer *b = &bs->paths;

    const size_t start = b->count;
    const Bs_Sv resolved = bs_buffer_absolute_path(b, Bs_Sv(path->data, path->size));

    size_t index;
    if (bs_modules_find(&bs->modules, resolved, &index)) {
        const Bs_Module *m = &bs->modules.data[index];
        if (!m->done) {
            bs_error(bs, "import loop detected");
        }

        bs_stack_push(bs, m->result);
        goto defer;
    }

    // BS
    {
        bs_da_push_many(bs, b, ".bs", 4);
        if (bs_import_language(bs, bs_buffer_reset(b, start))) {
            goto defer;
        }
        b->count = start + resolved.size;
    }

    // Native
    {
#if defined(_WIN32) || defined(_WIN64)
        bs_da_push_many(bs, b, ".dll", 5);

        Bs_C_Lib_Handle handle = LoadLibrary(b->data + start);
        if (!handle) {
            bs_error(bs, "could not import module '" Bs_Sv_Fmt "'", Bs_Sv_Arg(*path));
        }
#elif defined(__APPLE__) || defined(__MACH__)
        bs_da_push_many(bs, b, ".dylib", 7);

        Bs_C_Lib_Handle handle = dlopen(b->data + start, RTLD_LAZY);
        if (!handle) {
            bs_error(bs, "could not import module '" Bs_Sv_Fmt "'", Bs_Sv_Arg(*path));
        }
#else
        bs_da_push_many(bs, b, ".so", 4);

        Bs_C_Lib_Handle handle = dlopen(b->data + start, RTLD_LAZY);
        if (!handle) {
            bs_error(bs, "could not import module '" Bs_Sv_Fmt "'", Bs_Sv_Arg(*path));
        }
#endif

        Bs_C_Lib *library = bs_c_lib_new(bs, handle);

#if defined(_WIN32) || defined(_WIN64)
        void (*init)(Bs *, Bs_C_Lib *) = (void *)GetProcAddress(handle, "bs_library_init");
#else
        void (*init)(Bs *, Bs_C_Lib *) = dlsym(handle, "bs_library_init");
#endif

        if (!init) {
            bs_error_full(
                bs,

                Bs_Sv_Static("A BS native library must define 'bs_library_init'"),
                Bs_Sv_Static("BS_LIBRARY_INIT void bs_library_init(Bs *bs, Bs_C_Lib *library) {\n"
                             "    // Perform any initialization you wish to do\n"
                             "    // lolcat_init();\n"
                             "\n"
                             "    // Add BS values to the library\n"
                             "    // bs_c_lib_set(bs, library, Bs_Sv_Static(\"foo\"), "
                             "bs_value_object(lolcat_foo));\n"
                             "\n"
                             "    // Add multiple BS native functions to the library at once\n"
                             "    // static const Bs_FFI ffi[] = {\n"
                             "    //     {\"hello\", lolcat_hello},\n"
                             "    //     {\"urmom\", lolcat_urmom},\n"
                             "    //     /* ... */\n"
                             "    // };\n"
                             "    // bs_c_lib_ffi(bs, library, ffi, bs_c_array_size(ffi));\n"
                             "}"),

                "invalid native library '" Bs_Sv_Fmt "'",
                Bs_Sv_Arg(*path));
        }
        init(bs, library);

        const Bs_Module module = {
            .done = true,
            .result = bs_value_object(library),

            .name = bs_str_new(bs, bs_buffer_reset(b, start)),
            .length = resolved.size,
        };
        bs_modules_push(bs, &bs->modules, module);

        bs_stack_push(bs, module.result);
    }

defer:
    bs->gc_on = gc_on_save;
    bs->handles_on = handles_on_save;
}

static Bs_Value
bs_check_map_get_at(Bs *bs, size_t location, Bs_Map *map, Bs_Value index, const char *label) {
    if (index.type == BS_VALUE_NIL) {
        const Bs_Sv sv = bs_value_type_name_full(index);
        bs_error_at(bs, location, "cannot use '" Bs_Sv_Fmt "' as %s", Bs_Sv_Arg(sv), label);
    }

    Bs_Value value;
    if (!bs_map_get(bs, map, index, &value)) {
        Bs_Buffer *b = &bs->config.buffer;
        const size_t start = b->count;

        Bs_Writer w = bs_buffer_writer(b);
        if (index.type == BS_VALUE_NUM || index.type == BS_VALUE_BOOL) {
            bs_value_write(bs, &w, index);
        } else if (index.type == BS_VALUE_OBJECT) {
            if (index.as.object->type == BS_OBJECT_STR) {
                bs_value_write(bs, &w, index);
            } else {
                bs_fmt(&w, "<%s %p>", bs_object_type_name(index.as.object->type), index.as.object);
            }
        } else {
            assert(false && "unreachable");
        }

        const Bs_Sv sv = bs_buffer_reset(b, start);
        bs_error_at(bs, location, "undefined %s: " Bs_Sv_Fmt, label, Bs_Sv_Arg(sv));
    }

    return value;
}

static Bs_Value bs_container_get(Bs *bs, Bs_Value container, Bs_Value index) {
    if (container.type == BS_VALUE_NIL || container.type == BS_VALUE_BOOL) {
        const Bs_Sv sv = bs_value_type_name_full(container);
        bs_error(bs, "cannot invoke or index into " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
    }

    if (container.type == BS_VALUE_NUM) {
        bs_check_object_type_at(bs, 1, index, BS_OBJECT_STR, "method name");
        return bs_value_object(bs_bound_method_new(
            bs,
            container,
            bs_check_map_get_at(bs, 1, bs_builtin_number_methods_map(bs), index, "method")));
    }

    Bs_Map *map = NULL;
    const char *label = NULL;

    switch (container.as.object->type) {
    case BS_OBJECT_ARRAY: {
        const Bs_Check checks[] = {
            bs_check_whole,
            bs_check_object(BS_OBJECT_STR),
        };

        bs_check_multi_at(
            bs, 1, index, checks, bs_c_array_size(checks), "array index or method name");

        if (index.type == BS_VALUE_NUM) {
            Bs_Array *array = (Bs_Array *)container.as.object;

            Bs_Value value;
            if (!bs_array_get(bs, array, index.as.number, &value)) {
                bs_error(
                    bs,
                    "cannot get value at index %zu in array of length %zu",
                    (size_t)index.as.number,
                    array->count);
            }
            return value;
        }

        return bs_value_object(bs_bound_method_new(
            bs,
            container,
            bs_check_map_get_at(
                bs,
                1,
                bs_builtin_object_methods_map(bs, container.as.object->type),
                index,
                "method")));
    } break;

    case BS_OBJECT_STR: {
        const Bs_Check checks[] = {
            bs_check_whole,
            bs_check_object(BS_OBJECT_STR),
        };

        bs_check_multi_at(
            bs, 1, index, checks, bs_c_array_size(checks), "string index or method name");

        if (index.type == BS_VALUE_NUM) {
            Bs_Str *str = (Bs_Str *)container.as.object;
            const size_t at = index.as.number;

            if (at >= str->size) {
                bs_error(
                    bs, "cannot get substring at index %zu in string of length %zu", at, str->size);
            }

            return bs_value_object(bs_str_new(bs, Bs_Sv(&str->data[at], 1)));
        }

        return bs_value_object(bs_bound_method_new(
            bs,
            container,
            bs_check_map_get_at(
                bs,
                1,
                bs_builtin_object_methods_map(bs, container.as.object->type),
                index,
                "method")));
    } break;

    case BS_OBJECT_TABLE: {
        Bs_Value value;
        if (bs_map_get(
                bs, bs_builtin_object_methods_map(bs, container.as.object->type), index, &value)) {
            return bs_value_object(bs_bound_method_new(bs, container, value));
        }

        map = &((Bs_Table *)container.as.object)->map;
        label = "table key";
    } break;

    case BS_OBJECT_INSTANCE: {
        Bs_Instance *instance = (Bs_Instance *)container.as.object;

        Bs_Value value;
        if (bs_map_get(bs, &instance->class->methods, index, &value)) {
            return bs_value_object(bs_bound_method_new(bs, container, value));
        }

        map = &instance->properties;
        label = "instance property or method";
    } break;

    case BS_OBJECT_C_INSTANCE: {
        Bs_C_Instance *instance = (Bs_C_Instance *)container.as.object;
        return bs_value_object(bs_bound_method_new(
            bs,
            container,
            bs_check_map_get_at(
                bs, 1, &instance->class->methods, index, "instance property or method")));
    } break;

    case BS_OBJECT_C_LIB:
        map = &((Bs_C_Lib *)container.as.object)->map;
        label = "library symbol";
        break;

    default: {
        const Bs_Sv sv = bs_value_type_name_full(container);
        bs_error(bs, "cannot invoke or index into " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
    } break;
    }

    assert(map);
    assert(label);
    return bs_check_map_get_at(bs, 1, map, index, label);
}

static void bs_container_set(Bs *bs, Bs_Value container, Bs_Value index, Bs_Value value) {
    if (container.type != BS_VALUE_OBJECT) {
        const Bs_Sv sv = bs_value_type_name_full(container);
        bs_error(bs, "cannot take mutable index into " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
    }

    if (container.as.object->type == BS_OBJECT_ARRAY) {
        bs_check_whole_number_at(bs, 1, index, "array index");
        bs_array_set(bs, (Bs_Array *)container.as.object, index.as.number, value);
    } else if (container.as.object->type == BS_OBJECT_TABLE) {
        if (index.type == BS_VALUE_NIL) {
            const Bs_Sv sv = bs_value_type_name_full(index);
            bs_error_at(bs, 1, "cannot use '" Bs_Sv_Fmt "' as table key", Bs_Sv_Arg(sv));
        }

        bs_table_set(bs, (Bs_Table *)container.as.object, index, value);
    } else if (container.as.object->type == BS_OBJECT_INSTANCE) {
        if (index.type == BS_VALUE_NIL) {
            const Bs_Sv sv = bs_value_type_name_full(index);
            bs_error_at(bs, 1, "cannot use '" Bs_Sv_Fmt "' as instance property", Bs_Sv_Arg(sv));
        }

        bs_map_set(bs, &((Bs_Instance *)container.as.object)->properties, index, value);
    } else {
        const Bs_Sv sv = bs_value_type_name_full(container);
        bs_error(bs, "cannot take mutable index into " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
    }
}

static void bs_iter_map(Bs *bs, size_t offset, const Bs_Map *map, Bs_Value iterator) {
    size_t index;
    if (iterator.type == BS_VALUE_NIL) {
        index = 0;
    } else {
        index = iterator.as.number + 1;
    }

    while (index < map->capacity && map->data[index].key.type == BS_VALUE_NIL) {
        index++;
    }

    if (index >= map->capacity) {
        bs->frame->ip += offset;
    } else {
        const Bs_Entry entry = map->data[index];
        bs_stack_set(bs, 0, bs_value_num(index)); // Iterator
        bs_stack_push(bs, entry.key);             // Key
        bs_stack_push(bs, entry.value);           // Value
    }
}

static_assert(BS_COUNT_OPS == 69, "Update bs_interpret()");
static void bs_interpret(Bs *bs, Bs_Value *output) {
    const bool gc_on_save = bs->gc_on;
    const bool handles_on_save = bs->handles_on;
    const size_t frames_count_save = bs->frames.count;

    bs->gc_on = true;
    bs->handles_on = false;
    while (true) {
#ifdef BS_STEP_DEBUG
        Bs_Writer *w = &bs->config.log;
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
#endif // BS_STEP_DEBUG

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
                bs->handles_on = handles_on_save;
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
            bs_call_stack_top(bs, *bs->frame->ip++);
            break;

        case BS_OP_CLOSURE: {
            Bs_Closure *closure = bs_closure_new(bs, (Bs_Fn *)bs_chunk_read_const(bs).as.object);
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

        case BS_OP_DUP:
            bs_stack_push(bs, bs_stack_peek(bs, *bs->frame->ip++));
            break;

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

        case BS_OP_CLASS: {
            const Bs_Value name = bs_chunk_read_const(bs);
            assert(name.type == BS_VALUE_OBJECT && name.as.object->type == BS_OBJECT_STR);
            bs_stack_push(bs, bs_value_object(bs_class_new(bs, (Bs_Str *)name.as.object)));
        } break;

        case BS_OP_INVOKE: {
            const Bs_Value name = bs_chunk_read_const(bs);
            const size_t arity = *bs->frame->ip++;
            const Bs_Value this = bs_stack_peek(bs, arity);

            Bs_Value method;
            if (this.type == BS_VALUE_NUM) {
                method =
                    bs_check_map_get_at(bs, 1, bs_builtin_number_methods_map(bs), name, "method");
            } else if (this.type == BS_VALUE_OBJECT) {
                switch (this.as.object->type) {
                case BS_OBJECT_STR:
                case BS_OBJECT_ARRAY:
                    method = bs_check_map_get_at(
                        bs,
                        1,
                        bs_builtin_object_methods_map(bs, this.as.object->type),
                        name,
                        "method");
                    break;

                case BS_OBJECT_TABLE: {
                    Bs_Table *table = (Bs_Table *)this.as.object;
                    if (bs_map_get(bs, &table->map, name, &method)) {
                        bs_stack_set(bs, arity, method);
                    } else {
                        method = bs_check_map_get_at(
                            bs,
                            1,
                            bs_builtin_object_methods_map(bs, this.as.object->type),
                            name,
                            "table key");
                    }
                } break;

                case BS_OBJECT_INSTANCE: {
                    Bs_Instance *instance = (Bs_Instance *)this.as.object;
                    if (bs_map_get(bs, &instance->properties, name, &method)) {
                        bs_stack_set(bs, arity, method);
                    } else {
                        method = bs_check_map_get_at(
                            bs, 1, &instance->class->methods, name, "instance property or method");
                    }
                } break;

                case BS_OBJECT_C_INSTANCE: {
                    Bs_C_Instance *instance = (Bs_C_Instance *)this.as.object;
                    method = bs_check_map_get_at(
                        bs, 1, &instance->class->methods, name, "instance property or method");
                } break;

                case BS_OBJECT_C_LIB: {
                    Bs_C_Lib *library = (Bs_C_Lib *)this.as.object;
                    method = bs_check_map_get_at(bs, 1, &library->map, name, "library symbol");
                    bs_stack_set(bs, arity, method);
                } break;

                default: {
                    const Bs_Sv sv = bs_value_type_name_full(this);
                    bs_error_at(bs, 0, "cannot invoke or index into " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
                } break;
                }
            } else {
                const Bs_Sv sv = bs_value_type_name_full(this);
                bs_error_at(bs, 0, "cannot invoke or index into " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
            }

            bs_call_value(bs, 2, method, arity);
        } break;

        case BS_OP_METHOD: {
            const Bs_Value name = bs_chunk_read_const(bs);
            const Bs_Value method = bs_stack_peek(bs, 0);
            const Bs_Value class = bs_stack_peek(bs, 1);

            assert(class.type == BS_VALUE_OBJECT && class.as.object->type == BS_OBJECT_CLASS);
            bs_map_set(bs, &((Bs_Class *)class.as.object)->methods, name, method);

            bs->stack.count--;
        } break;

        case BS_OP_INIT_METHOD: {
            const bool can_fail = *bs->frame->ip++;

            const Bs_Value method = bs_stack_peek(bs, 0);
            assert(method.type == BS_VALUE_OBJECT && method.as.object->type == BS_OBJECT_CLOSURE);

            const Bs_Value class0 = bs_stack_peek(bs, 1);
            assert(class0.type == BS_VALUE_OBJECT && class0.as.object->type == BS_OBJECT_CLASS);

            Bs_Class *class = (Bs_Class *)class0.as.object;
            class->init = (Bs_Closure *)method.as.object;
            class->can_fail = can_fail;
            bs->stack.count--;
        } break;

        case BS_OP_INHERIT: {
            const Bs_Value super = bs_stack_peek(bs, 1);
            if (super.type != BS_VALUE_OBJECT || super.as.object->type != BS_OBJECT_CLASS) {
                const Bs_Sv sv = bs_value_type_name_full(super);
                bs_error(bs, "cannot inherit from " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
            }

            const Bs_Value class = bs_stack_peek(bs, 0);
            bs_map_copy(
                bs,
                &((Bs_Class *)class.as.object)->methods,
                &((Bs_Class *)super.as.object)->methods);

            bs->stack.count--;
        } break;

        case BS_OP_SUPER_GET: {
            const Bs_Value name = bs_chunk_read_const(bs);
            const Bs_Value super = bs_stack_pop(bs);
            assert(super.type == BS_VALUE_OBJECT && super.as.object->type == BS_OBJECT_CLASS);

            const Bs_Value this = bs_stack_peek(bs, 0);
            assert(this.type == BS_VALUE_OBJECT && this.as.object->type == BS_OBJECT_INSTANCE);

            Bs_Class *superclass = (Bs_Class *)super.as.object;
            Bs_Value value;

            if (name.type == BS_VALUE_NIL) {
                // Requested init()
                if (superclass->init) {
                    value = bs_value_object(
                        bs_bound_method_new(bs, this, bs_value_object(superclass->init)));
                } else {
                    bs_error(bs, "undefined super method: init");
                }
            } else {
                value = bs_value_object(bs_bound_method_new(
                    bs,
                    this,
                    bs_check_map_get_at(bs, 0, &superclass->methods, name, "super method")));
            }
            bs_stack_set(bs, 0, value);
        } break;

        case BS_OP_SUPER_INVOKE: {
            const Bs_Value name = bs_chunk_read_const(bs);
            const size_t arity = *bs->frame->ip++;

            const Bs_Value super = bs_stack_pop(bs);
            assert(super.type == BS_VALUE_OBJECT && super.as.object->type == BS_OBJECT_CLASS);

            const Bs_Value this = bs_stack_peek(bs, arity);
            assert(this.type == BS_VALUE_OBJECT && this.as.object->type == BS_OBJECT_INSTANCE);

            Bs_Class *superclass = (Bs_Class *)super.as.object;
            Bs_Value method;

            if (name.type == BS_VALUE_NIL) {
                // Requested init()
                if (superclass->init) {
                    method = bs_value_object(superclass->init);
                } else {
                    bs_error(bs, "undefined super method: init");
                }
            } else {
                method = bs_check_map_get_at(bs, 0, &superclass->methods, name, "super method");
            }
            bs_call_value(bs, 1, method, arity);
        } break;

        case BS_OP_ADD: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);

            if (a.type != BS_VALUE_NUM || b.type != BS_VALUE_NUM) {
                const Bs_Sv sa = bs_value_type_name_full(a);
                const Bs_Sv sb = bs_value_type_name_full(b);
                if ((a.type == BS_VALUE_OBJECT && a.as.object->type == BS_OBJECT_STR) ||
                    (b.type == BS_VALUE_OBJECT && b.as.object->type == BS_OBJECT_STR)) {
                    bs_error_full(
                        bs,

                        Bs_Sv_Static("Use ($) for string concatenation, or use string "
                                     "interpolation instead"),

                        Bs_Sv_Static("\"Hello, \" $ \"world!\"\n"
                                     "\"Hello, \" $ 69\n"
                                     "\"Hello, \\(34 + 35) nice!\""),

                        "invalid operands to binary (+): " Bs_Sv_Fmt ", " Bs_Sv_Fmt,
                        Bs_Sv_Arg(sa),
                        Bs_Sv_Arg(sb));
                } else {
                    bs_error(
                        bs,
                        "invalid operands to binary (+): " Bs_Sv_Fmt ", " Bs_Sv_Fmt,
                        Bs_Sv_Arg(sa),
                        Bs_Sv_Arg(sb));
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

        case BS_OP_MOD: {
            Bs_Value a, b;
            bs_binary_op(bs, &a, &b, "%");

            double result = fmod(a.as.number, b.as.number);
            if (result < 0) {
                result += fabs(b.as.number);
            }
            bs_stack_push(bs, bs_value_num(result));
        } break;

        case BS_OP_NEG: {
            const Bs_Value a = bs_stack_pop(bs);
            if (a.type != BS_VALUE_NUM) {
                const Bs_Sv sv = bs_value_type_name_full(a);
                bs_error(bs, "invalid operand to unary (-): " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
            }

            bs_stack_push(bs, bs_value_num(-a.as.number));
        } break;

        case BS_OP_BOR: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_check_integer(bs, a, "operand #1 to binary (|)");
            bs_check_integer(bs, b, "operand #2 to binary (|)");

            const long a1 = a.as.number;
            const long b1 = b.as.number;
            bs_stack_push(bs, bs_value_num(a1 | b1));
        } break;

        case BS_OP_BAND: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_check_integer(bs, a, "operand #1 to binary (&)");
            bs_check_integer(bs, b, "operand #2 to binary (&)");

            const long a1 = a.as.number;
            const long b1 = b.as.number;
            bs_stack_push(bs, bs_value_num(a1 & b1));
        } break;

        case BS_OP_BXOR: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_check_integer(bs, a, "operand #1 to binary (^)");
            bs_check_integer(bs, b, "operand #2 to binary (^)");

            const long a1 = a.as.number;
            const long b1 = b.as.number;
            bs_stack_push(bs, bs_value_num(a1 ^ b1));
        } break;

        case BS_OP_BNOT: {
            const Bs_Value a = bs_stack_pop(bs);
            bs_check_integer(bs, a, "operand to unary (~)");

            const long a1 = a.as.number;
            bs_stack_push(bs, bs_value_num(~a1));
        } break;

        case BS_OP_LNOT:
            bs_stack_push(bs, bs_value_bool(bs_value_is_falsey(bs_stack_pop(bs))));
            break;

        case BS_OP_SHL: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_check_integer(bs, a, "operand #1 to binary (<<)");
            bs_check_integer(bs, b, "operand #2 to binary (<<)");

            const long a1 = a.as.number;
            const long b1 = b.as.number;
            bs_stack_push(bs, bs_value_num(a1 << b1));
        } break;

        case BS_OP_SHR: {
            const Bs_Value b = bs_stack_pop(bs);
            const Bs_Value a = bs_stack_pop(bs);
            bs_check_integer(bs, a, "operand #1 to binary (>>)");
            bs_check_integer(bs, b, "operand #2 to binary (>>)");

            const long a1 = a.as.number;
            const long b1 = b.as.number;
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

        case BS_OP_IN: {
            const Bs_Value container = bs_stack_pop(bs);
            const Bs_Value key = bs_stack_pop(bs);

            if (container.type != BS_VALUE_OBJECT) {
                const Bs_Sv sv = bs_value_type_name_full(container);
                bs_error(bs, "cannot index into " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
            }

            Bs_Map *map = NULL;
            const char *label = NULL;

            switch (container.as.object->type) {
            case BS_OBJECT_TABLE:
                map = &((Bs_Table *)container.as.object)->map;
                label = "table key";
                break;

            case BS_OBJECT_CLASS:
                map = &((Bs_Class *)container.as.object)->methods;
                label = "class method";
                break;

            case BS_OBJECT_INSTANCE:
                map = &((Bs_Instance *)container.as.object)->properties;
                label = "instance property";
                break;

            case BS_OBJECT_C_CLASS:
                map = &((Bs_C_Class *)container.as.object)->methods;
                label = "class method";
                break;

            case BS_OBJECT_C_LIB:
                map = &((Bs_C_Lib *)container.as.object)->map;
                label = "library symbol";
                break;

            default: {
                const Bs_Sv sv = bs_value_type_name_full(container);
                bs_error(bs, "cannot index into " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
            } break;
            }

            if (key.type == BS_VALUE_NIL) {
                assert(label);
                const Bs_Sv sv = bs_value_type_name_full(key);
                bs_error(bs, "cannot use '" Bs_Sv_Fmt "' as %s", Bs_Sv_Arg(sv), label);
            }

            assert(map);
            bs_stack_push(bs, bs_value_bool(bs_map_get(bs, map, key, NULL)));
        } break;

        case BS_OP_IS: {
            const Bs_Value type = bs_stack_pop(bs);
            bs_check_object_type(bs, type, BS_OBJECT_STR, "type name");

            const Bs_Str *str = (const Bs_Str *)type.as.object;
            const Bs_Value value = bs_stack_pop(bs);
            bs_stack_push(
                bs,
                bs_value_bool(
                    bs_sv_eq(bs_value_type_name_full(value), Bs_Sv(str->data, str->size))));
        } break;

        case BS_OP_LEN: {
            const Bs_Value a = bs_stack_peek(bs, 0);
            if (a.type != BS_VALUE_OBJECT) {
                const Bs_Sv sv = bs_value_type_name_full(a);
                bs_error(bs, "cannot get length of " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
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
                size = ((Bs_Table *)a.as.object)->map.length;
                break;

            default: {
                const Bs_Sv sv = bs_value_type_name_full(a);
                bs_error(bs, "cannot get length of " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
            } break;
            }

            bs_stack_pop(bs);
            bs_stack_push(bs, bs_value_num(size));
        } break;

        case BS_OP_JOIN: {
            Bs_Buffer *buffer = &bs->config.buffer;
            const size_t start = buffer->count;

            const Bs_Value b = bs_stack_peek(bs, 0);
            const Bs_Value a = bs_stack_peek(bs, 1);

            Bs_Writer w = bs_buffer_writer(buffer);
            bs_value_write(bs, &w, a);
            bs_value_write(bs, &w, b);

            const Bs_Sv sv = bs_buffer_reset(buffer, start);
            bs_stack_set(bs, 1, bs_value_object(bs_str_new(bs, sv)));
            bs->stack.count--;
        } break;

        case BS_OP_TOSTR: {
            Bs_Buffer *buffer = &bs->config.buffer;
            const size_t start = buffer->count;

            Bs_Writer w = bs_buffer_writer(buffer);
            bs_value_write(bs, &w, bs_stack_peek(bs, 0));

            const Bs_Sv sv = bs_buffer_reset(buffer, start);
            bs_stack_set(bs, 0, bs_value_object(bs_str_new(bs, sv)));
        } break;

        case BS_OP_PANIC: {
            const size_t start = bs->config.buffer.count;
            Bs_Writer w = bs_buffer_writer(&bs->config.buffer);

            const Bs_Value value = bs_stack_peek(bs, 0);
            bs_value_write(bs, &w, value);
            bs->stack.count--;

            Bs_Error error = bs_error_begin_at(bs, 0);
            error.type = BS_ERROR_PANIC;
            error.message = bs_buffer_reset(&bs->config.buffer, start);

            bs->config.error.write(&bs->config.error, error);
            bs_error_end_at(bs, 0, error.native);
        } break;

        case BS_OP_ASSERT: {
            if (bs_value_is_falsey(bs_stack_peek(bs, 1))) {
                const size_t start = bs->config.buffer.count;
                Bs_Writer w = bs_buffer_writer(&bs->config.buffer);

                const Bs_Value message = bs_stack_peek(bs, 0);
                bs_value_write(bs, &w, message);
                bs->stack.count -= 2;

                Bs_Error error = bs_error_begin_at(bs, 0);
                error.type = BS_ERROR_PANIC;
                error.message = bs_buffer_reset(&bs->config.buffer, start);

                bs->config.error.write(&bs->config.error, error);
                bs_error_end_at(bs, 0, error.native);
            } else {
                bs->stack.count--;
            }
        } break;

        case BS_OP_IMPORT:
            bs_import(bs);
            break;

        case BS_OP_TYPEOF: {
            const Bs_Sv name = bs_value_type_name_full(bs_stack_peek(bs, 0));
            bs_stack_set(bs, 0, bs_value_object(bs_str_new(bs, name)));
        } break;

        case BS_OP_DELETE: {
            const Bs_Value index = bs_stack_pop(bs);
            const Bs_Value container = bs_stack_pop(bs);

            if (container.type != BS_VALUE_OBJECT) {
                const Bs_Sv sv = bs_value_type_name_full(container);
                bs_error(bs, "cannot delete from " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
            }

            Bs_Map *map = NULL;
            const char *label = NULL;

            switch (container.as.object->type) {
            case BS_OBJECT_TABLE:
                map = &((Bs_Table *)container.as.object)->map;
                label = "table key";
                break;

            case BS_OBJECT_INSTANCE:
                map = &((Bs_Instance *)container.as.object)->properties;
                label = "instance property";
                break;

            default: {
                const Bs_Sv sv = bs_value_type_name_full(container);
                bs_error(bs, "cannot delete from " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
            } break;
            }

            assert(map);
            assert(label);

            if (index.type == BS_VALUE_NIL) {
                const Bs_Sv sv = bs_value_type_name_full(index);
                bs_error(bs, "cannot use '" Bs_Sv_Fmt "' as %s", Bs_Sv_Arg(sv), label);
            }

            bs_stack_push(bs, bs_value_bool(bs_map_remove(bs, map, index)));
        } break;

        case BS_OP_DELETE_CONST: {
            const Bs_Value index = bs_chunk_read_const(bs);
            const Bs_Value container = bs_stack_pop(bs);

            if (container.type != BS_VALUE_OBJECT) {
                const Bs_Sv sv = bs_value_type_name_full(container);
                bs_error(bs, "cannot delete from " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
            }

            Bs_Map *map = NULL;
            const char *label = NULL;

            switch (container.as.object->type) {
            case BS_OBJECT_TABLE:
                map = &((Bs_Table *)container.as.object)->map;
                label = "table key";
                break;

            case BS_OBJECT_INSTANCE:
                map = &((Bs_Instance *)container.as.object)->properties;
                label = "instance property";
                break;

            default: {
                const Bs_Sv sv = bs_value_type_name_full(container);
                bs_error(bs, "cannot delete from " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
            } break;
            }

            assert(map);
            assert(label);

            if (index.type == BS_VALUE_NIL) {
                const Bs_Sv sv = bs_value_type_name_full(index);
                bs_error(bs, "cannot use '" Bs_Sv_Fmt "' as %s", Bs_Sv_Arg(sv), label);
            }

            bs_stack_push(bs, bs_value_bool(bs_map_remove(bs, map, index)));
        } break;

        case BS_OP_GDEF:
            bs_map_set(bs, &bs->globals, bs_chunk_read_const(bs), bs_stack_peek(bs, 0));
            bs_stack_pop(bs);
            break;

        case BS_OP_GGET: {
            const Bs_Value name = bs_chunk_read_const(bs);

            Bs_Value value;
            if (!bs_map_get(bs, &bs->globals, name, &value)) {
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

            if (bs_map_set(bs, &bs->globals, name, value)) {
                bs_map_remove(bs, &bs->globals, name);

                bs_error(
                    bs,
                    "undefined identifier '" Bs_Sv_Fmt "'",
                    Bs_Sv_Arg(*(const Bs_Str *)name.as.object));
            }
        } break;

        case BS_OP_LGET:
        case BS_OP_LRECEIVER:
            bs_stack_push(bs, bs->frame->base[bs_chunk_read_int(bs)]);
            break;

        case BS_OP_LSET:
            bs->frame->base[bs_chunk_read_int(bs)] = bs_stack_peek(bs, 0);
            break;

        case BS_OP_UGET:
        case BS_OP_URECEIVER: {
            Bs_Upvalue *upvalue = bs->frame->closure->data[bs_chunk_read_int(bs)];
            bs_stack_push(bs, *upvalue->value);
        } break;

        case BS_OP_USET: {
            const Bs_Value value = bs_stack_peek(bs, 0);
            Bs_Upvalue *upvalue = bs->frame->closure->data[bs_chunk_read_int(bs)];
            *upvalue->value = value;
        } break;

        case BS_OP_IGET: {
            const Bs_Value value = bs_container_get(bs, bs_stack_peek(bs, 1), bs_stack_peek(bs, 0));
            bs->stack.count -= 2;
            bs_stack_push(bs, value);
        } break;

        case BS_OP_IGET_CONST: {
            const Bs_Value value =
                bs_container_get(bs, bs_stack_peek(bs, 0), bs_chunk_read_const(bs));

            bs->stack.count--;
            bs_stack_push(bs, value);
        } break;

        case BS_OP_ISET:
        case BS_OP_ISET_CHAIN:
            bs_container_set(bs, bs_stack_peek(bs, 2), bs_stack_peek(bs, 1), bs_stack_peek(bs, 0));
            bs->stack.count -= 2;
            break;

        case BS_OP_ISET_CONST:
            bs_container_set(
                bs, bs_stack_peek(bs, 1), bs_chunk_read_const(bs), bs_stack_peek(bs, 0));

            bs->stack.count--;
            break;

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

        case BS_OP_MATCH: {
            const size_t offset = bs_chunk_read_int(bs);
            const Bs_Value pred = bs_stack_pop(bs);
            if (bs_value_equal(bs_stack_peek(bs, 0), pred)) {
                bs->frame->ip += offset;
            }
        } break;

        case BS_OP_ITER: {
            const size_t offset = bs_chunk_read_int(bs);

            const Bs_Value iterator = bs_stack_peek(bs, 0);
            const Bs_Value container = bs_stack_peek(bs, 1);

            if (container.type != BS_VALUE_OBJECT) {
                const Bs_Sv sv = bs_value_type_name_full(container);
                bs_error(bs, "cannot iterate over " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
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
            } else if (container.as.object->type == BS_OBJECT_STR) {
                const Bs_Str *str = (const Bs_Str *)container.as.object;

                size_t index;
                if (iterator.type == BS_VALUE_NIL) {
                    index = 0;
                } else {
                    index = iterator.as.number + 1;
                }

                if (index >= str->size) {
                    bs->frame->ip += offset;
                } else {
                    bs_stack_set(bs, 0, bs_value_num(index)); // Iterator
                    bs_stack_push(bs, bs_value_num(index));   // Key
                    bs_stack_push(
                        bs, bs_value_object(bs_str_new(bs, Bs_Sv(&str->data[index], 1)))); // Value
                }
            } else if (container.as.object->type == BS_OBJECT_TABLE) {
                const Bs_Table *table = (const Bs_Table *)container.as.object;
                bs_iter_map(bs, offset, &table->map, iterator);
            } else if (container.as.object->type == BS_OBJECT_INSTANCE) {
                const Bs_Instance *instance = (const Bs_Instance *)container.as.object;
                bs_iter_map(bs, offset, &instance->properties, iterator);
            } else {
                const Bs_Sv sv = bs_value_type_name_full(container);
                bs_error(bs, "cannot iterate over " Bs_Sv_Fmt, Bs_Sv_Arg(sv));
            }
        } break;

        case BS_OP_RANGE: {
            const size_t offset = bs_chunk_read_int(bs);

            const Bs_Value start = bs_stack_peek(bs, 2);
            bs_check_value_type_at(bs, 0, start, BS_VALUE_NUM, "range start");

            const Bs_Value end = bs_stack_peek(bs, 1);
            bs_check_value_type_at(bs, 1, end, BS_VALUE_NUM, "range end");

            Bs_Value step = bs_stack_peek(bs, 0);
            if (step.type == BS_VALUE_NIL) {
                step = bs_value_num((end.as.number > start.as.number) ? 1 : -1);
                bs_stack_set(bs, 0, step);
            }
            bs_check_value_type_at(bs, 2, step, BS_VALUE_NUM, "range step");

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
            bs_error(
                bs,
                "invalid op %d at offset %zu",
                op,
                bs->frame->ip - bs->frame->closure->fn->chunk.data - 1);
        }
    }
}

Bs_Result bs_run(Bs *bs, Bs_Sv path, Bs_Sv input, bool is_repl) {
    bs->config.unwind.ok = true;
    bs->running = true;
    bs->config.unwind.exit = -1;

    if (setjmp(bs->config.unwind.point)) {
        goto end;
    }

    Bs_Result result = {0};

    Bs_Buffer *b = &bs->paths;
    const size_t start = b->count;

    bs_buffer_absolute_path(b, path);

    const Bs_Closure *fn;
    if (is_repl) {
        Bs_Str *source = bs_str_new(bs, input);

        fn = bs_compile_module(
            bs, bs_buffer_reset(b, start), Bs_Sv(source->data, source->size), true, is_repl);

        if (fn) {
            fn->fn->source = source;
        }
    } else {
        fn = bs_compile_module(bs, bs_buffer_reset(b, start), input, true, is_repl);
    }

    if (!fn) {
        return (Bs_Result){.exit = 1};
    }

#ifdef BS_STEP_DEBUG
    bs_debug_chunk(bs_pretty_printer(bs, &bs->config.log), &fn->fn->chunk);
#endif // BS_STEP_DEBUG

    result.value = bs_call(bs, bs_value_object(fn), NULL, 0);

end:
    bs->stack.count = 0;

    bs->frame = NULL;
    bs->frames.count = 0;

    bs->upvalues = NULL;
    bs->gc_on = false;
    bs->handles_on = false;

    bs->running = false;

#ifdef BS_STEP_DEBUG
    bs_fmt(
        &bs->config.log,
        "Stopping BS with exit code %d\n",
        (bs->config.unwind.exit == -1) ? 0 : bs->config.unwind.exit);
#endif // BS_STEP_DEBUG

    result.ok = bs->config.unwind.ok;
    result.exit = bs->config.unwind.exit;
    return result;
}

Bs_Value bs_call(Bs *bs, Bs_Value fn, const Bs_Value *args, size_t arity) {
    if (!bs->running) {
        bs_error_standalone(
            bs, "cannot call bs_call() while BS is not running; call bs_run() first");
        return bs_value_nil;
    }

    const size_t stack_count_save = bs->stack.count;
    const size_t frames_count_save = bs->frames.count;

    bs_stack_push(bs, fn);
    for (size_t i = 0; i < arity; i++) {
        bs_stack_push(bs, args[i]);
    }
    bs_call_stack_top(bs, arity);

    Bs_Value result;
    if (fn.as.object->type == BS_OBJECT_CLOSURE) {
        bs_interpret(bs, &result);
    } else if (fn.as.object->type == BS_OBJECT_CLASS) {
        Bs_Class *class = (Bs_Class *)fn.as.object;
        if (class->init) {
            bs_interpret(bs, &result);
        } else {
            result = bs_stack_peek(bs, 0);
        }
    } else {
        result = bs_stack_peek(bs, 0);
    }

    bs->stack.count = stack_count_save;
    bs->frames.count = frames_count_save;
    bs->frame = bs->frames.count ? &bs->frames.data[bs->frames.count - 1] : NULL;
    return result;
}
