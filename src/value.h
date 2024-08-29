#ifndef VALUE_H
#define VALUE_H

#include <stdbool.h>
#include <stdio.h>

#include "basic.h"

typedef enum {
    VALUE_NIL,
    VALUE_NUM,
    VALUE_BOOL,
    VALUE_OBJECT
} ValueType;

typedef enum {
    OBJECT_FN,
    OBJECT_STR,
    OBJECT_ARRAY,
    OBJECT_TABLE,
    OBJECT_CLOSURE,
    OBJECT_UPVALUE,
    OBJECT_NATIVE_FN,
    OBJECT_NATIVE_DATA,
    OBJECT_NATIVE_LIBRARY,
    COUNT_OBJECTS
} ObjectType;

const char *object_type_name(ObjectType type);

typedef struct Object Object;
typedef struct ObjectFn ObjectFn;
typedef struct ObjectStr ObjectStr;
typedef struct ObjectArray ObjectArray;
typedef struct ObjectTable ObjectTable;
typedef struct ObjectClosure ObjectClosure;
typedef struct ObjectUpvalue ObjectUpvalue;
typedef struct ObjectNativeFn ObjectNativeFn;
typedef struct ObjectNativeData ObjectNativeData;
typedef struct ObjectNativeLibrary ObjectNativeLibrary;

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
const char *value_get_type_name(Value value);

void value_write(Value value, Writer *writer);
bool value_equal(Value a, Value b);

#endif // VALUE_H
