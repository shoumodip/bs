#include <stdint.h>
#include <stdio.h>

#include "value.h"

// TODO: Lazy hash strings
static uint32_t hash_string(const char *data, size_t size) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < size; i++) {
        hash ^= (uint8_t)data[i];
        hash *= 16777619;
    }
    return hash;
}

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
    }
}

bool value_is_falsey(Value value) {
    return value.type == VALUE_NIL || (value.type == VALUE_BOOL && !value.as.boolean);
}

void chunk_free(Chunk *chunk) {
    values_free(&chunk->constants);
    da_free(chunk);
}

void chunk_push_op(Chunk *chunk, Op op) {
    chunk->last = chunk->count;
    da_push(chunk, op);
}

void chunk_push_op_int(Chunk *chunk, Op op, size_t value) {
    const size_t bytes = sizeof(value);
    chunk_push_op(chunk, op);
    da_push_many(chunk, &value, bytes);
}

void chunk_push_op_value(Chunk *chunk, Op op, Value value) {
    const size_t index = chunk->constants.count;
    const size_t bytes = sizeof(index);
    values_push(&chunk->constants, value);

    chunk_push_op(chunk, op);
    da_push_many(chunk, &index, bytes);
}

bool object_str_eq(ObjectStr *a, ObjectStr *b) {
    if (!a || !b) {
        return false;
    }
    return a->size == b->size && !memcmp(a->data, b->data, b->size);
}

static void object_print(Object *object) {
    switch (object->type) {
    case OBJECT_STR:
        printf(SVFmt, SVArg(*(ObjectStr *)object));
        break;
    }
}

void value_print(Value value) {
    switch (value.type) {
    case VALUE_NIL:
        printf("nil");
        break;

    case VALUE_NUM:
        printf("%g", value.as.number);
        break;

    case VALUE_BOOL:
        printf("%s", value.as.boolean ? "true" : "false");
        break;

    case VALUE_OBJECT:
        object_print(value.as.object);
        break;
    }
}

void *gc_realloc(GC *gc, void *previous, size_t old_size, size_t new_size) {
    if (!new_size) {
        free(previous);
        return NULL;
    }

    return realloc(previous, new_size);
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

void table_free(Table *table, GC *gc) {
    gc_realloc(gc, table->data, sizeof(*table->data) * table->capacity, 0);
    memset(table, 0, sizeof(*table));
}

static void table_grow(Table *table, GC *gc, size_t capacity) {
    const size_t size = sizeof(Entry) * capacity;

    Entry *entries = gc_realloc(gc, NULL, 0, size);
    memset(entries, 0, size);

    size_t count = 0;
    for (size_t i = 0; i < table->capacity; i++) {
        Entry *src = &table->data[i];
        if (!src->key) {
            continue;
        }

        Entry *dst = entries_find(entries, capacity, src->key);
        dst->key = src->key;
        dst->value = src->value;
        count++;
    }

    table_free(table, gc);
    table->data = entries;
    table->count = count;
    table->capacity = capacity;
}

bool table_remove(Table *table, ObjectStr *key) {
    if (!table->count) {
        return false;
    }

    Entry *entry = entries_find(table->data, table->capacity, key);
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

bool table_get(Table *table, ObjectStr *key, Value *value) {
    if (!table->count) {
        return false;
    }

    Entry *entry = entries_find(table->data, table->capacity, key);
    if (!entry->key) {
        return false;
    }

    *value = entry->value;
    return true;
}

bool table_set(Table *table, GC *gc, ObjectStr *key, Value value) {
    if (table->count >= table->capacity * TABLE_MAX_LOAD) {
        table_grow(table, gc, table->capacity ? table->capacity * 2 : DA_INIT_CAP);
    }
    Entry *entry = entries_find(table->data, table->capacity, key);

    bool is_new = !entry->key;
    if (is_new && entry->value.type == VALUE_NIL) {
        table->count++;
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

ObjectStr *gc_new_object_str(GC *gc, const char *data, size_t size) {
    ObjectStr *str = (ObjectStr *)gc_new_object(gc, OBJECT_STR, sizeof(ObjectStr) + size);
    memcpy(str->data, data, size);
    str->size = size;
    return str;
}
