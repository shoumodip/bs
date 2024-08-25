#include "bs.h"
#include "hash.h"

#define TABLE_MAX_LOAD 0.75

void chunk_free(Vm *vm, Chunk *c) {
    values_free(vm, &c->constants);
    da_free(vm, c);
}

void chunk_push_op(Vm *vm, Chunk *c, Op op) {
    c->last = c->count;
    da_push(vm, c, op);
}

void chunk_push_op_int(Vm *vm, Chunk *c, Op op, size_t value) {
    const size_t bytes = sizeof(value);
    chunk_push_op(vm, c, op);
    da_push_many(vm, c, &value, bytes);
}

void chunk_push_op_value(Vm *vm, Chunk *c, Op op, Value value) {
    const size_t index = c->constants.count;
    const size_t bytes = sizeof(index);
    values_push(vm, &c->constants, value);

    chunk_push_op(vm, c, op);
    da_push_many(vm, c, &index, bytes);
}

ObjectFn *object_fn_new(Vm *vm) {
    ObjectFn *fn = (ObjectFn *)object_new(vm, OBJECT_FN, sizeof(ObjectFn));
    fn->name = NULL;
    fn->chunk = (Chunk){0};
    fn->arity = 0;
    fn->upvalues = 0;
    return fn;
}

ObjectStr *object_str_new(Vm *vm, const char *data, size_t size) {
    ObjectStr *str = (ObjectStr *)object_new(vm, OBJECT_STR, sizeof(ObjectStr) + size);
    str->hashed = false;

    memcpy(str->data, data, size);
    str->size = size;
    return str;
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

bool object_str_eq(const ObjectStr *a, const ObjectStr *b) {
    if (!a || !b) {
        return false;
    }
    return a->size == b->size && !memcmp(a->data, b->data, b->size);
}

ObjectArray *object_array_new(Vm *vm) {
    ObjectArray *array = (ObjectArray *)object_new(vm, OBJECT_ARRAY, sizeof(ObjectArray));
    array->data = NULL;
    array->count = 0;
    array->capacity = 0;
    return array;
}

bool object_array_get(Vm *vm, ObjectArray *a, size_t index, Value *value) {
    if (index >= a->count) {
        return false;
    }

    *value = a->data[index];
    return true;
}

void object_array_set(Vm *vm, ObjectArray *a, size_t index, Value value) {
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
                vm_realloc(vm, a->data, old * sizeof(*a->data), a->capacity * sizeof(*a->data));
        }

        memset(&a->data[a->count], 0, (index - a->count) * sizeof(*a->data));
    }

    a->data[index] = value;
    a->count = max(a->count, index + 1);
}

ObjectTable *object_table_new(Vm *vm) {
    ObjectTable *table = (ObjectTable *)object_new(vm, OBJECT_TABLE, sizeof(ObjectTable));
    table->data = NULL;
    table->count = 0;
    table->capacity = 0;
    table->real_count = 0;
    return table;
}

void object_table_free(Vm *vm, ObjectTable *t) {
    vm_realloc(vm, t->data, sizeof(*t->data) * t->capacity, 0);
    t->data = NULL;
    t->count = 0;
    t->capacity = 0;
}

bool object_table_remove(Vm *vm, ObjectTable *t, ObjectStr *key) {
    if (!t->count) {
        return false;
    }

    Entry *entry = entries_find_str(t->data, t->capacity, key);
    if (!entry) {
        return false;
    }

    if (!entry->key && entry->value.type == VALUE_BOOL && entry->value.as.boolean == true) {
        return false;
    }

    t->real_count--;
    entry->key = NULL;
    entry->value = value_bool(true);
    return true;
}

bool object_table_get(Vm *vm, ObjectTable *t, ObjectStr *key, Value *value) {
    if (!t->count) {
        return false;
    }

    Entry *entry = entries_find_str(t->data, t->capacity, key);
    if (!entry->key) {
        return false;
    }

    *value = entry->value;
    return true;
}

static void object_table_grow(Vm *vm, ObjectTable *t, size_t capacity) {
    const size_t size = sizeof(Entry) * capacity;

    Entry *entries = vm_realloc(vm, NULL, 0, size);
    memset(entries, 0, size);

    size_t count = 0;
    for (size_t i = 0; i < t->capacity; i++) {
        Entry *src = &t->data[i];
        if (!src->key) {
            continue;
        }

        Entry *dst = entries_find_str(entries, capacity, src->key);
        dst->key = src->key;
        dst->value = src->value;
        count++;
    }

    object_table_free(vm, t);
    t->data = entries;
    t->count = count;
    t->capacity = capacity;
}

bool object_table_set(Vm *vm, ObjectTable *t, ObjectStr *key, Value value) {
    if (t->count >= t->capacity * TABLE_MAX_LOAD) {
        object_table_grow(vm, t, t->capacity ? t->capacity * 2 : DA_INIT_CAP);
    }
    Entry *entry = entries_find_str(t->data, t->capacity, key);

    bool is_new = !entry->key;
    if (is_new && entry->value.type == VALUE_NIL) {
        t->count++;
        t->real_count++;
    }

    entry->key = key;
    entry->value = value;
    return is_new;
}

ObjectClosure *object_closure_new(Vm *vm, const ObjectFn *fn) {
    const size_t upvalues = sizeof(ObjectUpvalue *) * fn->upvalues;
    ObjectClosure *closure =
        (ObjectClosure *)object_new(vm, OBJECT_CLOSURE, sizeof(ObjectClosure) + upvalues);

    closure->fn = fn;
    closure->upvalues = fn->upvalues;
    memset(closure->data, 0, upvalues);
    return closure;
}

ObjectUpvalue *object_upvalue_new(Vm *vm, size_t index) {
    ObjectUpvalue *upvalue = (ObjectUpvalue *)object_new(vm, OBJECT_UPVALUE, sizeof(ObjectUpvalue));

    upvalue->closed = false;
    upvalue->index = index;
    upvalue->next = NULL;
    return upvalue;
}
