#include "basic.h"
#include "hash.h"

bool value_is_falsey(Value v) {
    return v.type == VALUE_NIL || (v.type == VALUE_BOOL && !v.as.boolean);
}

static_assert(COUNT_OBJECTS == 7, "Update object_type_name()");
const char *object_type_name(ObjectType type) {
    switch (type) {
    case OBJECT_FN:
        return "function";

    case OBJECT_STR:
        return "string";

    case OBJECT_ARRAY:
        return "array";

    case OBJECT_TABLE:
        return "table";

    case OBJECT_CLOSURE:
        return "closure";

    case OBJECT_UPVALUE:
        return "upvalue";

    case OBJECT_NATIVE_FN:
        return "native function";

    default:
        assert(false && "unreachable");
    }
}

const char *value_get_type_name(Value v) {
    switch (v.type) {
    case VALUE_NIL:
        return "nil";

    case VALUE_NUM:
        return "number";

    case VALUE_BOOL:
        return "boolean";

    case VALUE_OBJECT:
        return object_type_name(v.as.object->type);

    default:
        assert(false && "unreachable");
    }
}

static_assert(COUNT_OBJECTS == 7, "Update object_write()");
static void object_write(const Object *o, Writer *w) {
    switch (o->type) {
    case OBJECT_FN: {
        const ObjectFn *fn = (const ObjectFn *)o;
        if (fn->module) {
            w->fmt(w, "file '" SVFmt "'", SVArg(*fn->name));
        } else if (fn->name) {
            w->fmt(w, "fn " SVFmt "()", SVArg(*fn->name));
        } else {
            w->fmt(w, "fn ()");
        }
    } break;

    case OBJECT_STR:
        w->fmt(w, SVFmt, SVArg(*(const ObjectStr *)o));
        break;

    case OBJECT_ARRAY: {
        const ObjectArray *array = (const ObjectArray *)o;

        w->fmt(w, "[");
        for (size_t i = 0; i < array->count; i++) {
            if (i) {
                w->fmt(w, ", ");
            }
            value_write(array->data[i], w);
        }
        w->fmt(w, "]");
    } break;

    case OBJECT_TABLE: {
        ObjectTable *table = (ObjectTable *)o;

        w->fmt(w, "{");
        for (size_t i = 0, count = 0; i < table->capacity; i++) {
            Entry *entry = &table->data[i];
            if (!entry->key) {
                continue;
            }

            if (count) {
                w->fmt(w, ", ");
            }

            object_write((const Object *)entry->key, w);
            w->fmt(w, " = ");
            value_write(entry->value, w);
            count++;
        }
        w->fmt(w, "}");
    } break;

    case OBJECT_CLOSURE:
        object_write((Object *)((const ObjectClosure *)o)->fn, w);
        break;

    case OBJECT_NATIVE_FN:
        w->fmt(w, "native fn ()");
        break;

    case OBJECT_UPVALUE:
        w->fmt(w, "<upvalue>");
        break;

    default:
        assert(false && "unreachable");
    }
}

void value_write(Value v, Writer *w) {
    switch (v.type) {
    case VALUE_NIL:
        w->fmt(w, "nil");
        break;

    case VALUE_NUM:
        w->fmt(w, "%g", v.as.number);
        break;

    case VALUE_BOOL:
        w->fmt(w, "%s", v.as.boolean ? "true" : "false");
        break;

    case VALUE_OBJECT:
        object_write(v.as.object, w);
        break;

    default:
        assert(false && "unreachable");
    }
}

static_assert(COUNT_OBJECTS == 7, "Update object_equal()");
static bool object_equal(const Object *a, const Object *b) {
    if (a->type != b->type) {
        return false;
    }

    switch (a->type) {
    case OBJECT_STR: {
        const ObjectStr *a1 = (const ObjectStr *)a;
        const ObjectStr *b1 = (const ObjectStr *)b;
        return object_str_eq(a1, b1);
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

    case OBJECT_TABLE: {
        const ObjectTable *a1 = (const ObjectTable *)a;
        const ObjectTable *b1 = (const ObjectTable *)b;

        if (a1->real_count != b1->real_count) {
            return false;
        }

        for (size_t i = 0; i < a1->capacity; i++) {
            const Entry *a2 = &a1->data[i];
            if (!a2->key) {
                continue;
            }

            const Entry *b2 = entries_find_str(b1->data, b1->capacity, a2->key);
            if (!b2->key) {
                return false;
            }
        }

        return true;
    } break;

    case OBJECT_CLOSURE:
        return a == b;

    case OBJECT_NATIVE_FN:
        return ((ObjectNativeFn *)a)->fn == ((ObjectNativeFn *)b)->fn;

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
}
