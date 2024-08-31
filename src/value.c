#include "hash.h"

bool bs_value_is_falsey(Bs_Value v) {
    return v.type == BS_VALUE_NIL || (v.type == BS_VALUE_BOOL && !v.as.boolean);
}

static_assert(BS_COUNT_OBJECTS == 9, "Update bs_object_type_name()");
const char *bs_object_type_name(Bs_Object_Type type) {
    switch (type) {
    case BS_OBJECT_FN:
        return "function";

    case BS_OBJECT_STR:
        return "string";

    case BS_OBJECT_ARRAY:
        return "array";

    case BS_OBJECT_TABLE:
        return "table";

    case BS_OBJECT_CLOSURE:
        return "closure";

    case BS_OBJECT_UPVALUE:
        return "upvalue";

    case BS_OBJECT_C_FN:
        return "native function";

    case BS_OBJECT_C_LIB:
        return "native library";

    case BS_OBJECT_C_DATA:
        return "native object";

    default:
        assert(false && "unreachable");
    }
}

const char *bs_value_type_name(Bs_Value v) {
    switch (v.type) {
    case BS_VALUE_NIL:
        return "nil";

    case BS_VALUE_NUM:
        return "number";

    case BS_VALUE_BOOL:
        return "boolean";

    case BS_VALUE_OBJECT:
        return bs_object_type_name(v.as.object->type);

    default:
        assert(false && "unreachable");
    }
}

static_assert(BS_COUNT_OBJECTS == 9, "Update bs_object_write()");
static void bs_object_write(Bs_Writer *w, const Bs_Object *o) {
    switch (o->type) {
    case BS_OBJECT_FN: {
        const Bs_Fn *fn = (const Bs_Fn *)o;
        if (fn->module) {
            bs_write(w, "file '" Bs_Sv_Fmt "'", Bs_Sv_Arg(*fn->name));
        } else if (fn->name) {
            bs_write(w, "fn " Bs_Sv_Fmt "()", Bs_Sv_Arg(*fn->name));
        } else {
            bs_write(w, "fn ()");
        }
    } break;

    case BS_OBJECT_STR:
        bs_write(w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)o));
        break;

    case BS_OBJECT_ARRAY: {
        const Bs_Array *array = (const Bs_Array *)o;

        bs_write(w, "[");
        for (size_t i = 0; i < array->count; i++) {
            if (i) {
                bs_write(w, ", ");
            }
            bs_value_write(w, array->data[i]);
        }
        bs_write(w, "]");
    } break;

    case BS_OBJECT_TABLE: {
        Bs_Table *table = (Bs_Table *)o;

        bs_write(w, "{");
        for (size_t i = 0, count = 0; i < table->capacity; i++) {
            Entry *entry = &table->data[i];
            if (!entry->key) {
                continue;
            }

            if (count) {
                bs_write(w, ", ");
            }

            bs_object_write(w, (const Bs_Object *)entry->key);
            bs_write(w, " = ");
            bs_value_write(w, entry->value);
            count++;
        }
        bs_write(w, "}");
    } break;

    case BS_OBJECT_CLOSURE:
        bs_object_write(w, (Bs_Object *)((const Bs_Closure *)o)->fn);
        break;

    case BS_OBJECT_UPVALUE:
        bs_write(w, "<upvalue>");
        break;

    case BS_OBJECT_C_FN:
        bs_write(w, "native fn ()");
        break;

    case BS_OBJECT_C_LIB:
        bs_write(w, "<native library '" Bs_Sv_Fmt "'>", Bs_Sv_Arg(*((const Bs_C_Lib *)o)->path));
        break;

    case BS_OBJECT_C_DATA: {
        const Bs_C_Data *native = (const Bs_C_Data *)o;
        if (native->spec->write) {
            native->spec->write(w, native->data);
        } else {
            bs_write(w, "<native " Bs_Sv_Fmt " object>", Bs_Sv_Arg(native->spec->name));
        }
    } break;

    default:
        assert(false && "unreachable");
    }
}

void bs_value_write(Bs_Writer *w, Bs_Value v) {
    switch (v.type) {
    case BS_VALUE_NIL:
        bs_write(w, "nil");
        break;

    case BS_VALUE_NUM:
        bs_write(w, "%g", v.as.number);
        break;

    case BS_VALUE_BOOL:
        bs_write(w, "%s", v.as.boolean ? "true" : "false");
        break;

    case BS_VALUE_OBJECT:
        bs_object_write(w, v.as.object);
        break;

    default:
        assert(false && "unreachable");
    }
}

static_assert(BS_COUNT_OBJECTS == 9, "Update bs_object_equal()");
static bool bs_object_equal(const Bs_Object *a, const Bs_Object *b) {
    if (a->type != b->type) {
        return false;
    }

    switch (a->type) {
    case BS_OBJECT_STR: {
        const Bs_Str *a1 = (const Bs_Str *)a;
        const Bs_Str *b1 = (const Bs_Str *)b;
        return bs_str_eq(a1, b1);
    };

    case BS_OBJECT_ARRAY: {
        const Bs_Array *a1 = (const Bs_Array *)a;
        const Bs_Array *b1 = (const Bs_Array *)b;

        if (a1->count != b1->count) {
            return false;
        }

        for (size_t i = 0; i < a1->count; i++) {
            if (!bs_value_equal(a1->data[i], b1->data[i])) {
                return false;
            }
        }

        return true;
    } break;

    case BS_OBJECT_TABLE: {
        const Bs_Table *a1 = (const Bs_Table *)a;
        const Bs_Table *b1 = (const Bs_Table *)b;

        if (a1->real_count != b1->real_count) {
            return false;
        }

        for (size_t i = 0; i < a1->capacity; i++) {
            const Entry *a2 = &a1->data[i];
            if (!a2->key) {
                continue;
            }

            const Entry *b2 = bs_entries_find_str(b1->data, b1->capacity, a2->key);
            if (!b2->key) {
                return false;
            }
        }

        return true;
    } break;

    case BS_OBJECT_CLOSURE:
        return a == b;

    case BS_OBJECT_C_FN:
        return ((const Bs_C_Fn *)a)->fn == ((const Bs_C_Fn *)b)->fn;

    case BS_OBJECT_C_LIB:
    case BS_OBJECT_C_DATA:
        return a == b;

    default:
        assert(false && "unreachable");
    }
}

bool bs_value_equal(Bs_Value a, Bs_Value b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
    case BS_VALUE_NIL:
        return true;

    case BS_VALUE_NUM:
        return a.as.number == b.as.number;

    case BS_VALUE_BOOL:
        return a.as.boolean == b.as.boolean;

    case BS_VALUE_OBJECT:
        return bs_object_equal(a.as.object, b.as.object);

    default:
        assert(false && "unreachable");
    }
}
