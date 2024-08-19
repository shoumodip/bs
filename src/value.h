#ifndef VALUE_H
#define VALUE_H

#include "basic.h"

typedef enum {
    VALUE_NIL,
    VALUE_NUM,
    VALUE_BOOL,
    VALUE_OBJECT
} ValueType;

const char *value_type_name(ValueType type);

typedef enum {
    OBJECT_STR,
} ObjectType;

typedef struct Object Object;

struct Object {
    ObjectType type;
    Object *next;
};

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Object *object;
    } as;
} Value;

#define value_nil ((Value){VALUE_NIL})
#define value_num(v) ((Value){VALUE_NUM, .as.number = (v)})
#define value_bool(v) ((Value){VALUE_BOOL, .as.boolean = (v)})
#define value_object(v) ((Value){VALUE_OBJECT, .as.object = (Object *)(v)})

bool value_is_falsey(Value value);

typedef struct {
    Value *data;
    size_t count;
    size_t capacity;
} Values;

#define values_push da_push
#define values_free da_free

typedef struct {
    Object meta;
    size_t size;
    char data[];
} ObjectStr;

void value_print(Value value);

typedef struct {
    Object *objects;
} GC;

void *gc_realloc(GC *gc, void *previous, size_t old_size, size_t new_size);

ObjectStr *object_str_new(GC *gc, const char *data, size_t size);

#endif // VALUE_H
