#include <stdint.h>
#include <stdio.h>

#include "value.h"

const char *value_type_name(ValueType type) {
    switch (type) {
    case VALUE_NIL:
        return "nil";

    case VALUE_NUM:
        return "number";

    case VALUE_BOOL:
        return "boolean";

    case VALUE_OBJECT:
        return "object";

    default:
        assert(false && "unreachable");
    }
}

bool value_is_falsey(Value v) {
    return v.type == VALUE_NIL || (v.type == VALUE_BOOL && !v.as.boolean);
}

static_assert(COUNT_OBJECTS == 4, "Update object_print()");
static void object_print(const Object *o, FILE *f) {
    switch (o->type) {
    case OBJECT_FN: {
        const ObjectFn *fn = (const ObjectFn *)o;
        if (fn->name) {
            fprintf(f, "fn " SVFmt "()", SVArg(*fn->name));
        } else {
            fprintf(f, "fn ()");
        }
    } break;

    case OBJECT_STR:
        fprintf(f, SVFmt, SVArg(*(const ObjectStr *)o));
        break;

    case OBJECT_CLOSURE: {
        const ObjectFn *fn = ((const ObjectClosure *)o)->fn;
        if (fn->name) {
            fprintf(f, "fn " SVFmt "()", SVArg(*fn->name));
        } else {
            fprintf(f, "fn ()");
        }
    } break;

    default:
        assert(false && "unreachable");
    }
}

void value_print(Value v, FILE *f) {
    switch (v.type) {
    case VALUE_NIL:
        fprintf(f, "nil");
        break;

    case VALUE_NUM:
        fprintf(f, "%g", v.as.number);
        break;

    case VALUE_BOOL:
        fprintf(f, "%s", v.as.boolean ? "true" : "false");
        break;

    case VALUE_OBJECT:
        object_print(v.as.object, f);
        break;

    default:
        assert(false && "unreachable");
    }
}

static_assert(COUNT_OBJECTS == 4, "Update object_equal()");
static bool object_equal(const Object *a, const Object *b) {
    if (a->type != b->type) {
        return false;
    }

    switch (a->type) {
    case OBJECT_STR: {
        const ObjectStr *as = (const ObjectStr *)a;
        const ObjectStr *bs = (const ObjectStr *)b;
        return as->size == bs->size && !memcmp(as->data, bs->data, bs->size);
    };

    case OBJECT_CLOSURE:
        return a == b;

    default:
        assert(false && "unreachable");
    }
}

bool value_equal(Value a, Value b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
    case VALUE_NIL:
        return true;

    case VALUE_NUM:
        return a.as.number == b.as.number;

    case VALUE_BOOL:
        return a.as.boolean == b.as.boolean;

    case VALUE_OBJECT:
        return object_equal(a.as.object, b.as.object);

    default:
        assert(false && "unreachable");
    }
};

void chunk_free(Chunk *c) {
    values_free(&c->constants);
    da_free(c);
}

void chunk_push_op(Chunk *c, Op op) {
    c->last = c->count;
    da_push(c, op);
}

void chunk_push_op_int(Chunk *c, Op op, size_t value) {
    const size_t bytes = sizeof(value);
    chunk_push_op(c, op);
    da_push_many(c, &value, bytes);
}

void chunk_push_op_value(Chunk *c, Op op, Value value) {
    const size_t index = c->constants.count;
    const size_t bytes = sizeof(index);
    values_push(&c->constants, value);

    chunk_push_op(c, op);
    da_push_many(c, &index, bytes);
}

bool object_str_eq(ObjectStr *a, ObjectStr *b) {
    if (!a || !b) {
        return false;
    }
    return a->size == b->size && !memcmp(a->data, b->data, b->size);
}

void *gc_realloc(GC *gc, void *ptr, size_t old_size, size_t new_size) {
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

void table_free(Table *t, GC *gc) {
    gc_realloc(gc, t->data, sizeof(*t->data) * t->capacity, 0);
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

static void table_grow(Table *t, GC *gc, size_t capacity) {
    const size_t size = sizeof(Entry) * capacity;

    Entry *entries = gc_realloc(gc, NULL, 0, size);
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

    table_free(t, gc);
    t->data = entries;
    t->count = count;
    t->capacity = capacity;
}

bool table_set(Table *t, GC *gc, ObjectStr *key, Value value) {
    if (t->count >= t->capacity * TABLE_MAX_LOAD) {
        table_grow(t, gc, t->capacity ? t->capacity * 2 : DA_INIT_CAP);
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

static Object *gc_new_object(GC *gc, ObjectType type, size_t size) {
    Object *object = gc_realloc(gc, NULL, 0, size);
    object->type = type;
    object->next = gc->objects;
    gc->objects = object;
    return object;
}

ObjectFn *gc_new_object_fn(GC *gc) {
    ObjectFn *fn = (ObjectFn *)gc_new_object(gc, OBJECT_FN, sizeof(ObjectFn));
    fn->name = NULL;
    fn->chunk = (Chunk){0};
    fn->arity = 0;
    fn->upvalues = 0;
    return fn;
}

ObjectStr *gc_new_object_str(GC *gc, const char *data, size_t size) {
    ObjectStr *str = (ObjectStr *)gc_new_object(gc, OBJECT_STR, sizeof(ObjectStr) + size);
    memcpy(str->data, data, size);
    str->size = size;
    return str;
}

ObjectUpvalue *gc_new_object_upvalue(GC *gc, size_t index) {
    ObjectUpvalue *upvalue =
        (ObjectUpvalue *)gc_new_object(gc, OBJECT_UPVALUE, sizeof(ObjectUpvalue));

    upvalue->closed = false;
    upvalue->index = index;
    upvalue->next = NULL;
    return upvalue;
}

ObjectClosure *gc_new_object_closure(GC *gc, const ObjectFn *fn) {
    const size_t upvalues = sizeof(ObjectUpvalue *) * fn->upvalues;
    ObjectClosure *closure =
        (ObjectClosure *)gc_new_object(gc, OBJECT_CLOSURE, sizeof(ObjectClosure) + upvalues);

    closure->fn = fn;
    closure->upvalues = fn->upvalues;
    memset(closure->data, 0, upvalues);
    return closure;
}
