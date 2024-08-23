#include "bs.h"

bool object_str_eq(ObjectStr *a, ObjectStr *b) {
    if (!a || !b) {
        return false;
    }
    return a->size == b->size && !memcmp(a->data, b->data, b->size);
}

static_assert(COUNT_OBJECTS == 4, "Update bs_free_object()");
static void bs_free_object(Bs *bs, Object *object) {
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
    if (!new_size) {
        free(ptr);
        return NULL;
    }

    return realloc(ptr, new_size);
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

#define TABLE_MAX_LOAD 0.75

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
    bs->memory.objects = object;
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
