#include <stdio.h>

#include "value.h"

static_assert(COUNT_VALUES == 3, "Update value_type_name()");
const char *value_type_name(ValueType type) {
    switch (type) {
    case VALUE_NIL:
        return "nil";

    case VALUE_NUM:
        return "number";

    case VALUE_BOOL:
        return "boolean";

    default:
        assert(false && "unreachable");
    }
}

static_assert(COUNT_VALUES == 3, "Update value_print()");
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

    default:
        assert(false && "unreachable");
    }
}

bool value_is_falsey(Value value) {
    return value.type == VALUE_NIL || (value.type == VALUE_BOOL && !value.as.boolean);
}
