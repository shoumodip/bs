#ifndef VALUE_H
#define VALUE_H

#include <stdio.h>

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
    OBJECT_CLOSURE,
    OBJECT_UPVALUE,
    COUNT_OBJECTS
} ObjectType;

typedef struct Object Object;
typedef struct ObjectFn ObjectFn;
typedef struct ObjectStr ObjectStr;
typedef struct ObjectArray ObjectArray;
typedef struct ObjectClosure ObjectClosure;
typedef struct ObjectUpvalue ObjectUpvalue;

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
const char *value_type_name(Value value);

void value_print(Value value, FILE *file);
bool value_equal(Value a, Value b);

#endif // VALUE_H
