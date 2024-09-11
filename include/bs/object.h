#ifndef BS_OBJECT_H
#define BS_OBJECT_H

#include <stdint.h>

#include "bs.h"
#include "op.h"
#include "token.h"

typedef struct {
    Bs_Value *data;
    size_t count;
    size_t capacity;
} Bs_Values;

#define bs_values_free bs_da_free
#define bs_values_push bs_da_push

typedef struct {
    Bs_Loc loc;
    size_t index;
} Bs_Op_Loc;

typedef struct {
    Bs_Op_Loc *data;
    size_t count;
    size_t capacity;
} Bs_Op_Locs;

#define bs_op_locs_free bs_da_free
#define bs_op_locs_push bs_da_push

typedef struct {
    uint8_t *data;
    size_t last;
    size_t count;
    size_t capacity;

    Bs_Values constants;
    Bs_Op_Locs locations;
} Bs_Chunk;

void bs_chunk_free(Bs *bs, Bs_Chunk *chunk);
void bs_chunk_push_op(Bs *bs, Bs_Chunk *chunk, Bs_Op op);
void bs_chunk_push_op_loc(Bs *bs, Bs_Chunk *c, Bs_Loc loc);
void bs_chunk_push_op_int(Bs *bs, Bs_Chunk *chunk, Bs_Op op, size_t value);
void bs_chunk_push_op_value(Bs *bs, Bs_Chunk *chunk, Bs_Op op, Bs_Value value);

struct Bs_Object {
    Bs_Object_Type type;
    Bs_Object *next;
    bool marked;
};

struct Bs_Fn {
    Bs_Object meta;

    Bs_Chunk chunk;
    size_t module;
    Bs_Str *name;

    size_t arity;
    size_t upvalues;

    bool extended;
};

Bs_Fn *bs_fn_new(Bs *bs);

struct Bs_Str {
    Bs_Object meta;

    bool hashed;
    uint32_t hash;

    size_t size;
    char data[];
};

Bs_Str *bs_str_new(Bs *bs, Bs_Sv sv);
bool bs_str_eq(const Bs_Str *a, const Bs_Str *b);

struct Bs_Array {
    Bs_Object meta;
    Bs_Value *data;
    size_t count;
    size_t capacity;
};

Bs_Array *bs_array_new(Bs *bs);
bool bs_array_get(Bs *bs, Bs_Array *array, size_t index, Bs_Value *value);
void bs_array_set(Bs *bs, Bs_Array *array, size_t index, Bs_Value value);

typedef struct {
    Bs_Value key;
    Bs_Value value;
} Bs_Entry;

struct Bs_Table {
    Bs_Object meta;
    Bs_Entry *data;
    size_t count;
    size_t length;
    size_t capacity;
};

Bs_Table *bs_table_new(Bs *bs);

void bs_table_free(Bs *bs, Bs_Table *table);
bool bs_table_remove(Bs *bs, Bs_Table *table, Bs_Value key);

bool bs_table_get(Bs *bs, Bs_Table *table, Bs_Value key, Bs_Value *value);
bool bs_table_set(Bs *bs, Bs_Table *table, Bs_Value key, Bs_Value value);

struct Bs_Closure {
    Bs_Object meta;
    const Bs_Fn *fn;

    size_t upvalues;
    Bs_Upvalue *data[];
};

Bs_Closure *bs_closure_new(Bs *bs, const Bs_Fn *fn);

struct Bs_Upvalue {
    Bs_Object meta;
    Bs_Value closed;
    Bs_Value *value;
    Bs_Upvalue *next;
};

Bs_Upvalue *bs_upvalue_new(Bs *bs, Bs_Value *value);

struct Bs_C_Fn {
    Bs_Object meta;
    Bs_C_Fn_Ptr fn;
    Bs_C_Lib *library;
};

Bs_C_Fn *bs_c_fn_new(Bs *bs, Bs_C_Fn_Ptr ptr);

struct Bs_C_Lib {
    Bs_Object meta;
    void *data;
    const Bs_Str *path;

    Bs_Table functions;
};

Bs_C_Lib *bs_c_lib_new(Bs *bs, void *data, const Bs_Str *path);

struct Bs_C_Data {
    Bs_Object meta;
    void *data;
    const Bs_C_Data_Spec *spec;
};

Bs_C_Data *bs_c_data_new(Bs *bs, void *data, const Bs_C_Data_Spec *spec);

#endif // BS_OBJECT_H
