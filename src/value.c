#include <stdint.h>
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

    default:
        assert(false && "unreachable");
    }
}

bool value_is_falsey(Value v) {
    return v.type == VALUE_NIL || (v.type == VALUE_BOOL && !v.as.boolean);
}

static_assert(COUNT_OBJECTS == 5, "Update object_print()");
static void object_print(const Object *o, FILE *f) {
    switch (o->type) {
    case OBJECT_FN: {
        const ObjectFn *fn = (const ObjectFn *)o;
        if (fn->name) {
            fprintf(f, "fn " SVFmt "()", SVArg(*fn->name));
        } else {
            fprintf(f, "fn ()");
        }
    } break;

    case OBJECT_STR:
        fprintf(f, SVFmt, SVArg(*(const ObjectStr *)o));
        break;

    case OBJECT_ARRAY: {
        const ObjectArray *array = (const ObjectArray *)o;

        fprintf(f, "[");
        for (size_t i = 0; i < array->count; i++) {
            if (i) {
                fprintf(f, ", ");
            }
            value_print(array->data[i], f);
        }
        fprintf(f, "]");
    } break;

    case OBJECT_CLOSURE: {
        const ObjectFn *fn = ((const ObjectClosure *)o)->fn;
        if (fn->name) {
            fprintf(f, "fn " SVFmt "()", SVArg(*fn->name));
        } else {
            fprintf(f, "fn ()");
        }
    } break;

    case OBJECT_UPVALUE:
        fprintf(f, "<upvalue>");
        break;

    default:
        assert(false && "unreachable");
    }
}

void value_print(Value v, FILE *f) {
    switch (v.type) {
    case VALUE_NIL:
        fprintf(f, "nil");
        break;

    case VALUE_NUM:
        fprintf(f, "%g", v.as.number);
        break;

    case VALUE_BOOL:
        fprintf(f, "%s", v.as.boolean ? "true" : "false");
        break;

    case VALUE_OBJECT:
        object_print(v.as.object, f);
        break;

    default:
        assert(false && "unreachable");
    }
}

static_assert(COUNT_OBJECTS == 5, "Update object_equal()");
static bool object_equal(const Object *a, const Object *b) {
    if (a->type != b->type) {
        return false;
    }

    switch (a->type) {
    case OBJECT_STR: {
        const ObjectStr *a1 = (const ObjectStr *)a;
        const ObjectStr *b1 = (const ObjectStr *)b;
        return a1->size == b1->size && !memcmp(a1->data, b1->data, b1->size);
    };

    case OBJECT_ARRAY: {
        const ObjectArray *a1 = (const ObjectArray *)a;
        const ObjectArray *b1 = (const ObjectArray *)b;

        if (a1->count != b1->count) {
            return false;
        }

        for (size_t i = 0; i < a1->count; i++) {
            if (!value_equal(a1->data[i], b1->data[i])) {
                return false;
            }
        }

        return true;
    } break;

    case OBJECT_CLOSURE:
        return a == b;

    default:
        assert(false && "unreachable");
    }
}

bool value_equal(Value a, Value b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
    case VALUE_NIL:
        return true;

    case VALUE_NUM:
        return a.as.number == b.as.number;

    case VALUE_BOOL:
        return a.as.boolean == b.as.boolean;

    case VALUE_OBJECT:
        return object_equal(a.as.object, b.as.object);

    default:
        assert(false && "unreachable");
    }
};

void chunk_free(Chunk *c) {
    values_free(&c->constants);
    da_free(c);
}

void chunk_push_op(Chunk *c, Op op) {
    c->last = c->count;
    da_push(c, op);
}

void chunk_push_op_int(Chunk *c, Op op, size_t value) {
    const size_t bytes = sizeof(value);
    chunk_push_op(c, op);
    da_push_many(c, &value, bytes);
}

void chunk_push_op_value(Chunk *c, Op op, Value value) {
    const size_t index = c->constants.count;
    const size_t bytes = sizeof(index);
    values_push(&c->constants, value);

    chunk_push_op(c, op);
    da_push_many(c, &index, bytes);
}
