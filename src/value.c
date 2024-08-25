#include "object.h"

bool value_is_falsey(Value v) {
    return v.type == VALUE_NIL || (v.type == VALUE_BOOL && !v.as.boolean);
}

static_assert(COUNT_OBJECTS == 5, "Update value_type_name()");
const char *value_type_name(Value v) {
    switch (v.type) {
    case VALUE_NIL:
        return "nil";

    case VALUE_NUM:
        return "number";

    case VALUE_BOOL:
        return "boolean";

    case VALUE_OBJECT:
        switch (v.as.object->type) {
        case OBJECT_FN:
            return "function";

        case OBJECT_STR:
            return "string";

        case OBJECT_ARRAY:
            return "array";

        case OBJECT_CLOSURE:
            return "closure";

        case OBJECT_UPVALUE:
            return "upvalue";

        default:
            assert(false && "unreachable");
        }

    default:
        assert(false && "unreachable");
    }
}

static_assert(COUNT_OBJECTS == 5, "Update object_print()");
static void object_print(const Object *o, FILE *file) {
    switch (o->type) {
    case OBJECT_FN: {
        const ObjectFn *fn = (const ObjectFn *)o;
        if (fn->name) {
            fprintf(file, "fn " SVFmt "()", SVArg(*fn->name));
        } else {
            fprintf(file, "fn ()");
        }
    } break;

    case OBJECT_STR:
        fprintf(file, SVFmt, SVArg(*(const ObjectStr *)o));
        break;

    case OBJECT_ARRAY: {
        const ObjectArray *array = (const ObjectArray *)o;

        fprintf(file, "[");
        for (size_t i = 0; i < array->count; i++) {
            if (i) {
                fprintf(file, ", ");
            }
            value_print(array->data[i], file);
        }
        fprintf(file, "]");
    } break;

    case OBJECT_CLOSURE: {
        const ObjectFn *fn = ((const ObjectClosure *)o)->fn;
        if (fn->name) {
            fprintf(file, "fn " SVFmt "()", SVArg(*fn->name));
        } else {
            fprintf(file, "fn ()");
        }
    } break;

    case OBJECT_UPVALUE:
        fprintf(file, "<upvalue>");
        break;

    default:
        assert(false && "unreachable");
    }
}

void value_print(Value v, FILE *file) {
    switch (v.type) {
    case VALUE_NIL:
        fprintf(file, "nil");
        break;

    case VALUE_NUM:
        fprintf(file, "%g", v.as.number);
        break;

    case VALUE_BOOL:
        fprintf(file, "%s", v.as.boolean ? "true" : "false");
        break;

    case VALUE_OBJECT:
        object_print(v.as.object, file);
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
