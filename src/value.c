#include <stdio.h>

#include "value.h"

const char *value_type_name(ValueType type) {
    switch (type) {
    case VALUE_NIL:
        return "nil";

    case VALUE_NUM:
        return "number";

    case VALUE_BOOL:
        return "boolean";

    case VALUE_OBJECT:
        return "object";
    }
}

bool value_is_falsey(Value value) {
    return value.type == VALUE_NIL || (value.type == VALUE_BOOL && !value.as.boolean);
}

static void object_print(Object *object) {
    switch (object->type) {
    case OBJECT_STR:
        printf(SVFmt, SVArg(*(ObjectStr *)object));
        break;
    }
}

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

    case VALUE_OBJECT:
        object_print(value.as.object);
        break;
    }
}

void *gc_realloc(GC *gc, void *previous, size_t old_size, size_t new_size) {
    if (!new_size) {
        free(previous);
        return NULL;
    }

    return realloc(previous, new_size);
}

static Object *object_new(GC *gc, ObjectType type, size_t size) {
    Object *object = gc_realloc(gc, NULL, 0, size);
    object->type = type;
    object->next = gc->objects;
    gc->objects = object;
    return object;
}

ObjectStr *object_str_new(GC *gc, const char *data, size_t size) {
    ObjectStr *str = (ObjectStr *)object_new(gc, OBJECT_STR, sizeof(ObjectStr) + size);
    memcpy(str->data, data, size);
    str->size = size;
    return str;
}
