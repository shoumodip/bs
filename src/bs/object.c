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
    return fn;
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
    upvalue->next = NULL;
    upvalue->value = value;
    upvalue->closed = bs_value_nil;
    return upvalue;
}

Bs_Class *bs_class_new(Bs *bs, Bs_Str *name) {
    Bs_Class *class = (Bs_Class *)bs_object_new(bs, BS_OBJECT_CLASS, sizeof(Bs_Class));
    class->name = name;
    class->init = NULL;
    class->can_fail = false;
    memset(&class->methods, '\0', sizeof(class->methods));
    return class;
}

Bs_Instance *bs_instance_new(Bs *bs, Bs_Class *class) {
    Bs_Instance *instance =
        (Bs_Instance *)bs_object_new(bs, BS_OBJECT_INSTANCE, sizeof(Bs_Instance));

    instance->class = class;
    memset(&instance->properties, '\0', sizeof(instance->properties));
    return instance;
}

Bs_C_Class *bs_c_class_new(Bs *bs, Bs_Sv name, size_t size, Bs_C_Fn_Ptr init) {
    Bs_C_Class *class = (Bs_C_Class *)bs_object_new(bs, BS_OBJECT_C_CLASS, sizeof(Bs_C_Class));
    class->name = name;
    class->size = size;
    class->init = init ? bs_c_fn_new(bs, name, init) : NULL;
    class->free = NULL;
    class->mark = NULL;
    class->can_fail = false;
    memset(&class->methods, '\0', sizeof(class->methods));
    return class;
}

void bs_c_class_add(Bs *bs, Bs_C_Class *class, Bs_Sv name, Bs_C_Fn_Ptr ptr) {
    bs_map_set(
        bs,
        &class->methods,
        bs_value_object(bs_str_new(bs, name)),
        bs_value_object(bs_c_fn_new(bs, name, ptr)));
}

Bs_C_Instance *bs_c_instance_new(Bs *bs, Bs_C_Class *class) {
    Bs_C_Instance *instance = (Bs_C_Instance *)bs_object_new(
        bs, BS_OBJECT_C_INSTANCE, sizeof(Bs_C_Instance) + class->size);

    instance->class = class;
    memset(instance->data, '\0', class->size);
    return instance;
}

Bs_Bound_Method *bs_bound_method_new(Bs *bs, Bs_Value this, Bs_Value fn) {
    Bs_Bound_Method *method =
        (Bs_Bound_Method *)bs_object_new(bs, BS_OBJECT_BOUND_METHOD, sizeof(Bs_Bound_Method));
    method->this = this;
    method->fn = fn;
    return method;
}

Bs_C_Fn *bs_c_fn_new(Bs *bs, Bs_Sv name, Bs_C_Fn_Ptr ptr) {
    Bs_C_Fn *fn = (Bs_C_Fn *)bs_object_new(bs, BS_OBJECT_C_FN, sizeof(Bs_C_Fn));
    fn->ptr = ptr;
    fn->name = name;
    return fn;
}

Bs_C_Lib *bs_c_lib_new(Bs *bs, Bs_C_Lib_Handle handle) {
    Bs_C_Lib *library = (Bs_C_Lib *)bs_object_new(bs, BS_OBJECT_C_LIB, sizeof(Bs_C_Lib));
    library->handle = handle;
    memset(&library->map, 0, sizeof(library->map));
    return library;
}

void bs_c_lib_set(Bs *bs, Bs_C_Lib *library, Bs_Sv name, Bs_Value value) {
    bs_map_set(bs, &library->map, bs_value_object(bs_str_new(bs, name)), value);
}

void bs_c_lib_ffi(Bs *bs, Bs_C_Lib *library, const Bs_FFI *ffi, size_t count) {
    for (size_t i = 0; i < count; i++) {
        const Bs_Sv name = bs_sv_from_cstr(ffi[i].name);
        bs_map_set(
            bs,
            &library->map,
            bs_value_object(bs_str_new(bs, name)),
            bs_value_object(bs_c_fn_new(bs, name, ffi[i].ptr)));
    }
}
