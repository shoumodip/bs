#ifndef VALUE_H
#define VALUE_H

#include "basic.h"

typedef enum {
    VALUE_NIL,
    VALUE_NUM,
    VALUE_BOOL,
    COUNT_VALUES
} ValueType;

const char *value_type_name(ValueType type);

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;
} Value;

#define value_nil ((Value){VALUE_NIL})
#define value_num(v) ((Value){VALUE_NUM, .as.number = (v)})
#define value_bool(v) ((Value){VALUE_BOOL, .as.boolean = (v)})

void value_print(Value value);
bool value_is_falsey(Value value);

typedef struct {
    Value *data;
    size_t count;
    size_t capacity;
} Values;

#define values_push da_push
#define values_free da_free

#endif // VALUE_H
