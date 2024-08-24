#include "bs.h"

// #define GC_DEBUG_LOG
// #define GC_DEBUG_STRESS
#define GC_GROW_FACTOR 2
#define TABLE_MAX_LOAD 0.75

bool object_str_eq(ObjectStr *a, ObjectStr *b) {
    if (!a || !b) {
        return false;
    }
    return a->size == b->size && !memcmp(a->data, b->data, b->size);
}

static_assert(COUNT_OBJECTS == 5, "Update bs_free_object()");
static void bs_free_object(Bs *bs, Object *object) {

#ifdef GC_DEBUG_LOG
    printf("[GC] Free %p; Type: %d\n", object, object->type);
#endif // GC_DEBUG_LOG

    switch (object->type) {
    case OBJECT_FN: {
        ObjectFn *fn = (ObjectFn *)object;
        chunk_free(&fn->chunk);
        bs_realloc(bs, fn, sizeof(*fn), 0);
    } break;

    case OBJECT_STR: {
        ObjectStr *str = (ObjectStr *)object;
        bs_realloc(bs, str, sizeof(*str) + str->size, 0);
    } break;

    case OBJECT_ARRAY: {
        ObjectArray *array = (ObjectArray *)object;
        bs_realloc(bs, array->data, sizeof(*array->data) * array->capacity, 0);
        bs_realloc(bs, array, sizeof(*array), 0);
    } break;

    case OBJECT_UPVALUE:
        bs_realloc(bs, object, sizeof(ObjectUpvalue), 0);
        break;

    case OBJECT_CLOSURE: {
        ObjectClosure *closure = (ObjectClosure *)object;
        bs_realloc(bs, closure, sizeof(*closure) + sizeof(ObjectUpvalue *) * closure->upvalues, 0);
    } break;

    default:
        assert(false && "unreachable");
    }
}

static_assert(COUNT_OBJECTS == 5, "Update object_mark()");
static void object_mark(Object *o, Memory *m) {
    if (!o || o->marked) {
        return;
    }
    o->marked = true;

#ifdef GC_DEBUG_LOG
    printf("[GC] Mark %p ", o);
    value_print(value_object(o), stdout);
    printf("\n");
#endif // GC_DEBUG_LOG

    switch (o->type) {
    case OBJECT_FN: {
        ObjectFn *fn = (ObjectFn *)o;
        object_mark((Object *)fn->name, m);

        for (size_t i = 0; i < fn->chunk.constants.count; i++) {
            const Value value = fn->chunk.constants.data[i];
            if (value.type == VALUE_OBJECT) {
                object_mark(value.as.object, m);
            }
        }
    } break;

    case OBJECT_STR:
        break;

    case OBJECT_ARRAY: {
        ObjectArray *array = (ObjectArray *)o;
        for (size_t i = 0; i < array->count; i++) {
            const Value value = array->data[i];
            if (value.type == VALUE_OBJECT) {
                object_mark(value.as.object, m);
            }
        }
    } break;

    case OBJECT_CLOSURE: {
        ObjectClosure *closure = (ObjectClosure *)o;
        object_mark((Object *)closure->fn, m);

        for (size_t i = 0; i < closure->upvalues; i++) {
            object_mark((Object *)closure->data[i], m);
        }
    } break;

    case OBJECT_UPVALUE: {
        ObjectUpvalue *upvalue = (ObjectUpvalue *)o;
        if (upvalue->closed) {
            if (upvalue->value.type == VALUE_OBJECT) {
                object_mark(upvalue->value.as.object, m);
            }
        }
    } break;

    default:
        assert(false && "unreachable");
    }
}

static void table_mark(Table *t, Memory *m) {
    for (size_t i = 0; i < t->capacity; i++) {
        Entry *entry = &t->data[i];
        object_mark((Object *)entry->key, m);

        if (entry->value.type == VALUE_OBJECT) {
            object_mark(entry->value.as.object, m);
        }
    }
}

void bs_collect(Bs *bs) {
    Memory *m = &bs->memory;

#ifdef GC_DEBUG_LOG
    printf("\n-------- GC Begin --------\n");
    const size_t before = m->allocated;
#endif // GC_DEBUG_LOG

    // Mark
    for (size_t i = 0; i < m->stack.count; i++) {
        const Value value = m->stack.data[i];
        if (value.type == VALUE_OBJECT) {
            object_mark(value.as.object, m);
        }
    }

    for (size_t i = 0; i < m->frames.count; i++) {
        object_mark((Object *)m->frames.data[i].closure, m);
    }

    for (ObjectUpvalue *upvalue = m->upvalues; upvalue; upvalue = upvalue->next) {
        object_mark((Object *)upvalue, m);
    }

    for (Scope *scope = bs->compiler.scope; scope; scope = scope->outer) {
        object_mark((Object *)scope->fn, m);
    }

    table_mark(&m->globals, m);

    // Sweep
    Object *previous = NULL;
    Object *object = m->objects;

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
                m->objects = object;
            }

            bs_free_object(bs, lost);
        }
    }

    m->gc = max(m->gc, m->allocated * GC_GROW_FACTOR);

#ifdef GC_DEBUG_LOG
    if (before != m->allocated) {
        printf(
            "\n[GC] Collected %zu bytes (%zu -> %zu); Next run at %zu bytes\n",
            before - m->allocated,
            before,
            m->allocated,
            m->gc);
    }

    printf("-------- GC End ----------\n\n");
#endif // GC_DEBUG_LOG
}

void bs_free(Bs *bs) {
    Memory *m = &bs->memory;

    Object *object = m->objects;
    while (object) {
        Object *next = object->next;
        bs_free_object(bs, object);
        object = next;
    }

    values_free(&m->stack);
    frames_free(&m->frames);
    table_free(&m->globals, bs);
}

void *bs_realloc(Bs *bs, void *ptr, size_t old_size, size_t new_size) {
    bs->memory.allocated += new_size - old_size;

    if (new_size > old_size) {
#ifdef GC_DEBUG_STRESS
        bs_collect(bs);
#endif // GC_DEBUG_STRESS

        if (bs->memory.allocated > bs->memory.gc) {
            bs_collect(bs);
        }
    }

    if (!new_size) {
        free(ptr);
        return NULL;
    }

    return realloc(ptr, new_size);
}

bool array_get(ObjectArray *array, size_t index, Value *value) {
    if (index >= array->count) {
        return false;
    }

    *value = array->data[index];
    return true;
}

void array_set(ObjectArray *a, Bs *bs, size_t index, Value value) {
    if (index >= a->count) {
        if (index >= a->capacity) {
            const size_t old = a->capacity;

            if (!a->capacity) {
                a->capacity = DA_INIT_CAP;
            }

            while (index >= a->capacity) {
                a->capacity *= 2;
            }

            a->data =
                bs_realloc(bs, a->data, old * sizeof(*a->data), a->capacity * sizeof(*a->data));
        }

        memset(&a->data[a->count], 0, (index - a->count) * sizeof(*a->data));
    }

    a->data[index] = value;
    a->count = max(a->count, index + 1);
}

// TODO: Lazy hash strings
static uint32_t hash_string(const char *data, size_t size) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; i++) {
        hash ^= (uint8_t)data[i];
        hash *= 16777619;
    }
    return hash;
}

static Entry *entries_find(Entry *entries, size_t capacity, ObjectStr *key) {
    uint32_t index = hash_string(key->data, key->size) % capacity;
    Entry *tombstone = NULL;

    while (true) {
        Entry *entry = &entries[index];
        if (object_str_eq(entry->key, key)) {
            return entry;
        }

        if (!entry->key) {
            if (entry->value.type == VALUE_NIL) {
                return tombstone ? tombstone : entry;
            }

            if (!tombstone) {
                tombstone = entry;
            }
        }

        index = (index + 1) % capacity;
    }
}

void table_free(Table *t, Bs *bs) {
    bs_realloc(bs, t->data, sizeof(*t->data) * t->capacity, 0);
    memset(t, 0, sizeof(*t));
}

bool table_remove(Table *t, ObjectStr *key) {
    if (!t->count) {
        return false;
    }

    Entry *entry = entries_find(t->data, t->capacity, key);
    if (!entry) {
        return false;
    }

    if (!entry->key && entry->value.type == VALUE_BOOL && entry->value.as.boolean == true) {
        return false;
    }

    entry->key = NULL;
    entry->value = value_bool(true);
    return true;
}

bool table_get(Table *t, ObjectStr *key, Value *value) {
    if (!t->count) {
        return false;
    }

    Entry *entry = entries_find(t->data, t->capacity, key);
    if (!entry->key) {
        return false;
    }

    *value = entry->value;
    return true;
}

static void table_grow(Table *t, Bs *bs, size_t capacity) {
    const size_t size = sizeof(Entry) * capacity;

    Entry *entries = bs_realloc(bs, NULL, 0, size);
    memset(entries, 0, size);

    size_t count = 0;
    for (size_t i = 0; i < t->capacity; i++) {
        Entry *src = &t->data[i];
        if (!src->key) {
            continue;
        }

        Entry *dst = entries_find(entries, capacity, src->key);
        dst->key = src->key;
        dst->value = src->value;
        count++;
    }

    table_free(t, bs);
    t->data = entries;
    t->count = count;
    t->capacity = capacity;
}

bool table_set(Table *t, Bs *bs, ObjectStr *key, Value value) {
    if (t->count >= t->capacity * TABLE_MAX_LOAD) {
        table_grow(t, bs, t->capacity ? t->capacity * 2 : DA_INIT_CAP);
    }
    Entry *entry = entries_find(t->data, t->capacity, key);

    bool is_new = !entry->key;
    if (is_new && entry->value.type == VALUE_NIL) {
        t->count++;
    }

    entry->key = key;
    entry->value = value;
    return is_new;
}

static Object *bs_new_object(Bs *bs, ObjectType type, size_t size) {
    Object *object = bs_realloc(bs, NULL, 0, size);
    object->type = type;
    object->next = bs->memory.objects;
    object->marked = false;
    bs->memory.objects = object;

#ifdef GC_DEBUG_LOG
    printf("[GC] Allocate %p (%zu bytes); Type: %d\n", object, size, type);
#endif // GC_DEBUG_LOG

    return object;
}

ObjectFn *bs_new_object_fn(Bs *bs) {
    ObjectFn *fn = (ObjectFn *)bs_new_object(bs, OBJECT_FN, sizeof(ObjectFn));
    fn->name = NULL;
    fn->chunk = (Chunk){0};
    fn->arity = 0;
    fn->upvalues = 0;
    return fn;
}

ObjectStr *bs_new_object_str(Bs *bs, const char *data, size_t size) {
    ObjectStr *str = (ObjectStr *)bs_new_object(bs, OBJECT_STR, sizeof(ObjectStr) + size);
    memcpy(str->data, data, size);
    str->size = size;
    return str;
}

ObjectArray *bs_new_object_array(Bs *bs) {
    ObjectArray *array = (ObjectArray *)bs_new_object(bs, OBJECT_ARRAY, sizeof(ObjectArray));
    array->data = NULL;
    array->count = 0;
    array->capacity = 0;
    return array;
}

ObjectUpvalue *bs_new_object_upvalue(Bs *bs, size_t index) {
    ObjectUpvalue *upvalue =
        (ObjectUpvalue *)bs_new_object(bs, OBJECT_UPVALUE, sizeof(ObjectUpvalue));

    upvalue->closed = false;
    upvalue->index = index;
    upvalue->next = NULL;
    return upvalue;
}

ObjectClosure *bs_new_object_closure(Bs *bs, const ObjectFn *fn) {
    const size_t upvalues = sizeof(ObjectUpvalue *) * fn->upvalues;
    ObjectClosure *closure =
        (ObjectClosure *)bs_new_object(bs, OBJECT_CLOSURE, sizeof(ObjectClosure) + upvalues);

    closure->fn = fn;
    closure->upvalues = fn->upvalues;
    memset(closure->data, 0, upvalues);
    return closure;
}
