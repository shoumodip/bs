#include "bs/object.h"

void bs_chunk_free(Bs *bs, Bs_Chunk *c) {
    bs_values_free(bs, &c->constants);
    bs_op_locs_free(bs, &c->locations);
    bs_da_free(bs, c);
}

void bs_chunk_push_op(Bs *bs, Bs_Chunk *c, Bs_Op op) {
    c->last = c->count;
    bs_da_push(bs, c, op);
}

void bs_chunk_push_op_loc(Bs *bs, Bs_Chunk *c, Bs_Loc loc) {
    bs_op_locs_push(bs, &c->locations, ((Bs_Op_Loc){.loc = loc, .index = c->count}));
}

void bs_chunk_push_op_int(Bs *bs, Bs_Chunk *c, Bs_Op op, size_t value) {
    const size_t bytes = sizeof(value);
    bs_chunk_push_op(bs, c, op);
    bs_da_push_many(bs, c, &value, bytes);
}

void bs_chunk_push_op_value(Bs *bs, Bs_Chunk *c, Bs_Op op, Bs_Value value) {
    const size_t index = c->constants.count;
    const size_t bytes = sizeof(index);
    bs_values_push(bs, &c->constants, value);

    bs_chunk_push_op(bs, c, op);
    bs_da_push_many(bs, c, &index, bytes);
}

Bs_Fn *bs_fn_new(Bs *bs) {
    Bs_Fn *fn = (Bs_Fn *)bs_object_new(bs, BS_OBJECT_FN, sizeof(Bs_Fn));
    fn->chunk = (Bs_Chunk){0};
    fn->module = 0;
    fn->name = NULL;

    fn->arity = 0;
    fn->upvalues = 0;

    fn->extended = false;
    return fn;
}

Bs_Str *bs_str_new(Bs *bs, Bs_Sv sv) {
    Bs_Str *str = (Bs_Str *)bs_object_new(bs, BS_OBJECT_STR, sizeof(Bs_Str) + sv.size);
    str->hashed = false;

    if (sv.data) {
        memcpy(str->data, sv.data, sv.size);
    }
    str->size = sv.size;
    return str;
}

bool bs_str_eq(const Bs_Str *a, const Bs_Str *b) {
    if (!a || !b) {
        return false;
    }

    if (a == b) {
        return true;
    }

    return a->size == b->size && !memcmp(a->data, b->data, b->size);
}

Bs_Array *bs_array_new(Bs *bs) {
    Bs_Array *array = (Bs_Array *)bs_object_new(bs, BS_OBJECT_ARRAY, sizeof(Bs_Array));
    array->data = NULL;
    array->count = 0;
    array->capacity = 0;
    return array;
}

bool bs_array_get(Bs *bs, Bs_Array *a, size_t index, Bs_Value *value) {
    if (index >= a->count) {
        return false;
    }

    *value = a->data[index];
    return true;
}

void bs_array_set(Bs *bs, Bs_Array *a, size_t index, Bs_Value value) {
    if (index >= a->count) {
        if (index >= a->capacity) {
            const size_t old = a->capacity;

            if (!a->capacity) {
                a->capacity = BS_DA_INIT_CAP;
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
    a->count = bs_max(a->count, index + 1);
}

Bs_Table *bs_table_new(Bs *bs) {
    Bs_Table *table = (Bs_Table *)bs_object_new(bs, BS_OBJECT_TABLE, sizeof(Bs_Table));
    memset(&table->map, 0, sizeof(table->map));
    return table;
}

void bs_table_free(Bs *bs, Bs_Table *t) {
    bs_map_free(bs, &t->map);
}

bool bs_table_remove(Bs *bs, Bs_Table *t, Bs_Value key) {
    return bs_map_remove(bs, &t->map, key);
}

bool bs_table_get(Bs *bs, Bs_Table *t, Bs_Value key, Bs_Value *value) {
    return bs_map_get(bs, &t->map, key, value);
}

bool bs_table_set(Bs *bs, Bs_Table *t, Bs_Value key, Bs_Value value) {
    return bs_map_set(bs, &t->map, key, value);
}

Bs_Closure *bs_closure_new(Bs *bs, const Bs_Fn *fn) {
    const size_t upvalues = sizeof(Bs_Upvalue *) * fn->upvalues;
    Bs_Closure *closure =
        (Bs_Closure *)bs_object_new(bs, BS_OBJECT_CLOSURE, sizeof(Bs_Closure) + upvalues);

    closure->fn = fn;
    closure->upvalues = fn->upvalues;
    memset(closure->data, 0, upvalues);
    return closure;
}

Bs_Upvalue *bs_upvalue_new(Bs *bs, Bs_Value *value) {
    Bs_Upvalue *upvalue = (Bs_Upvalue *)bs_object_new(bs, BS_OBJECT_UPVALUE, sizeof(Bs_Upvalue));
    upvalue->value = value;
    upvalue->next = NULL;
    return upvalue;
}

Bs_C_Fn *bs_c_fn_new(Bs *bs, const char *name, Bs_C_Fn_Ptr fn) {
    Bs_C_Fn *c = (Bs_C_Fn *)bs_object_new(bs, BS_OBJECT_C_FN, sizeof(Bs_C_Fn));
    c->fn = fn;
    c->name = name;
    c->library = NULL;
    return c;
}

Bs_C_Lib *bs_c_lib_new(Bs *bs, void *data, const Bs_Str *path) {
    Bs_C_Lib *c = (Bs_C_Lib *)bs_object_new(bs, BS_OBJECT_C_LIB, sizeof(Bs_C_Lib));
    c->data = data;
    c->path = path;

    memset(&c->functions, 0, sizeof(c->functions));
    c->functions.meta.type = BS_OBJECT_TABLE;
    return c;
}

Bs_C_Data *bs_c_data_new(Bs *bs, void *data, const Bs_C_Data_Spec *spec) {
    Bs_C_Data *c = (Bs_C_Data *)bs_object_new(bs, BS_OBJECT_C_DATA, sizeof(Bs_C_Data));
    c->data = data;
    c->spec = spec;
    return c;
}
