#ifndef BS_OBJECT_H
#define BS_OBJECT_H

#include <stdint.h>

#include "map.h"
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

    // In case the function was compiled at runtime using meta.compile() or meta.eval()
    Bs_Str *source;
};

Bs_Fn *bs_fn_new(Bs *bs);

struct Bs_Str {
    Bs_Object meta;
    uint32_t hash;
    size_t size;
    char data[];
};

struct Bs_Array {
    Bs_Object meta;
    Bs_Value *data;
    size_t count;
    size_t capacity;
};

Bs_Array *bs_array_new(Bs *bs);
bool bs_array_get(Bs *bs, Bs_Array *array, size_t index, Bs_Value *value);
void bs_array_set(Bs *bs, Bs_Array *array, size_t index, Bs_Value value);

struct Bs_Table {
    Bs_Object meta;
    Bs_Map map;
};

Bs_Table *bs_table_new(Bs *bs);

void bs_table_free(Bs *bs, Bs_Table *table);
bool bs_table_remove(Bs *bs, Bs_Table *table, Bs_Value key);

bool bs_table_get(Bs *bs, Bs_Table *table, Bs_Value key, Bs_Value *value);
bool bs_table_set(Bs *bs, Bs_Table *table, Bs_Value key, Bs_Value value);

struct Bs_Closure {
    Bs_Object meta;
    Bs_Fn *fn;

    size_t upvalues;
    Bs_Upvalue *data[];
};

Bs_Closure *bs_closure_new(Bs *bs, Bs_Fn *fn);

struct Bs_Upvalue {
    Bs_Object meta;
    Bs_Value closed;
    Bs_Value *value;
    Bs_Upvalue *next;
};

Bs_Upvalue *bs_upvalue_new(Bs *bs, Bs_Value *value);

struct Bs_Class {
    Bs_Object meta;
    Bs_Str *name;
    Bs_Map methods;
    Bs_Closure *init;

    bool can_fail;
};

Bs_Class *bs_class_new(Bs *bs, Bs_Str *name);

struct Bs_Instance {
    Bs_Object meta;
    Bs_Class *class;
    Bs_Map properties;
};

Bs_Instance *bs_instance_new(Bs *bs, Bs_Class *class);

typedef void (*Bs_C_Class_Free)(void *userdata, void *instance_data);
typedef void (*Bs_C_Class_Mark)(Bs *bs, void *instance_data);
typedef void (*Bs_C_Class_Show)(Bs_Pretty_Printer *printer, const void *instance_data);

struct Bs_C_Class {
    Bs_Object meta;
    Bs_Sv name;
    size_t size;
    Bs_Map methods;

    Bs_C_Fn *init;
    Bs_C_Class_Free free;
    Bs_C_Class_Mark mark;
    Bs_C_Class_Show show;

    bool can_fail;
};

Bs_C_Class *bs_c_class_new(Bs *bs, Bs_Sv name, size_t size, Bs_C_Fn_Ptr init);
void bs_c_class_add(Bs *bs, Bs_C_Class *class, Bs_Sv name, Bs_C_Fn_Ptr ptr);

struct Bs_C_Instance {
    Bs_Object meta;
    Bs_C_Class *class;
    char data[];
};

#define bs_flex_member_as(data, T) (*(T *)(data))

Bs_C_Instance *bs_c_instance_new(Bs *bs, Bs_C_Class *class);

struct Bs_Bound_Method {
    Bs_Object meta;
    Bs_Value this;
    Bs_Value fn;
};

Bs_Bound_Method *bs_bound_method_new(Bs *bs, Bs_Value this, Bs_Value fn);

struct Bs_C_Fn {
    Bs_Object meta;
    Bs_Sv name;
    Bs_C_Fn_Ptr ptr;
};

Bs_C_Fn *bs_c_fn_new(Bs *bs, Bs_Sv name, Bs_C_Fn_Ptr ptr);

#if defined(_WIN32) || defined(_WIN64)
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>

#    define BS_LIBRARY_INIT __declspec(dllexport)

typedef HMODULE Bs_C_Lib_Handle;
#else
#    define BS_LIBRARY_INIT

typedef void *Bs_C_Lib_Handle;
#endif // _WIN32

struct Bs_C_Lib {
    Bs_Object meta;
    Bs_C_Lib_Handle handle;
    Bs_Map map;
};

Bs_C_Lib *bs_c_lib_new(Bs *bs, Bs_C_Lib_Handle handle);
void bs_c_lib_set(Bs *bs, Bs_C_Lib *library, Bs_Sv name, Bs_Value value);

typedef struct {
    const char *name;
    Bs_C_Fn_Ptr ptr;
} Bs_FFI;

void bs_c_lib_ffi(Bs *bs, Bs_C_Lib *library, const Bs_FFI *ffi, size_t count);

#endif // BS_OBJECT_H
