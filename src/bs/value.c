#include "bs/object.h"

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

const char *bs_value_type_name(Bs_Value v, bool extended) {
    switch (v.type) {
    case BS_VALUE_NIL:
        return extended ? "bruh" : "nil";

    case BS_VALUE_NUM:
        return "number";

    case BS_VALUE_BOOL:
        return extended ? "capness" : "boolean";

    case BS_VALUE_OBJECT:
        return bs_object_type_name(v.as.object->type);

    default:
        assert(false && "unreachable");
    }
}

static_assert(BS_COUNT_OBJECTS == 9, "Update bs_object_write()");
static void bs_object_write_impl(Bs_Writer *w, const Bs_Object *o, bool extended) {
    switch (o->type) {
    case BS_OBJECT_FN: {
        const Bs_Fn *fn = (const Bs_Fn *)o;
        if (fn->module) {
            if (fn->name) {
                bs_fmt(w, "module '" Bs_Sv_Fmt "'", Bs_Sv_Arg(*fn->name));
            } else {
                bs_fmt(w, "module");
            }
        } else if (fn->name) {
            bs_fmt(w, "fn " Bs_Sv_Fmt "()", Bs_Sv_Arg(*fn->name));
        } else {
            bs_fmt(w, "fn ()");
        }
    } break;

    case BS_OBJECT_STR:
        bs_fmt(w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)o));
        break;

    case BS_OBJECT_ARRAY: {
        const Bs_Array *array = (const Bs_Array *)o;

        bs_fmt(w, "[");
        for (size_t i = 0; i < array->count; i++) {
            if (i) {
                bs_fmt(w, ", ");
            }
            bs_value_write_impl(w, array->data[i], extended);
        }
        bs_fmt(w, "]");
    } break;

    case BS_OBJECT_TABLE: {
        Bs_Table *table = (Bs_Table *)o;

        bs_fmt(w, "{");
        for (size_t i = 0, count = 0; i < table->capacity; i++) {
            Bs_Entry *entry = &table->data[i];
            if (entry->key.type == BS_VALUE_NIL) {
                continue;
            }

            if (count) {
                bs_fmt(w, ", ");
            }

            bs_value_write_impl(w, entry->key, extended);
            bs_fmt(w, " = ");
            bs_value_write_impl(w, entry->value, extended);
            count++;
        }
        bs_fmt(w, "}");
    } break;

    case BS_OBJECT_CLOSURE:
        bs_object_write_impl(w, (Bs_Object *)((const Bs_Closure *)o)->fn, extended);
        break;

    case BS_OBJECT_UPVALUE:
        bs_fmt(w, "<upvalue>");
        break;

    case BS_OBJECT_C_FN:
        bs_fmt(w, "native fn ()");
        break;

    case BS_OBJECT_C_LIB:
        bs_object_write_impl(w, (const Bs_Object *)&((const Bs_C_Lib *)o)->functions, extended);
        break;

    case BS_OBJECT_C_DATA: {
        const Bs_C_Data *native = (const Bs_C_Data *)o;
        if (native->spec->write) {
            native->spec->write(w, native->data);
        } else {
            bs_fmt(w, "<native " Bs_Sv_Fmt " object>", Bs_Sv_Arg(native->spec->name));
        }
    } break;

    default:
        assert(false && "unreachable");
    }
}

void bs_value_write_impl(Bs_Writer *w, Bs_Value v, bool extended) {
    switch (v.type) {
    case BS_VALUE_NIL:
        if (extended) {
            bs_fmt(w, "bruh");
        } else {
            bs_fmt(w, "nil");
        }
        break;

    case BS_VALUE_NUM: {
        char buffer[32];
        int count = snprintf(buffer, sizeof(buffer), "%lf", v.as.number);
        assert(count >= 0 && count + 1 < sizeof(buffer));

        const char *decimal = memchr(buffer, '.', count);
        if (decimal) {
            for (const size_t point = decimal - buffer; count > point; count--) {
                if (buffer[count - 1] != '0') {
                    break;
                }
            }

            if (buffer[count - 1] == '.') {
                count--;
            }
        }

        w->write(w, Bs_Sv(buffer, count));
    } break;

    case BS_VALUE_BOOL:
        if (extended) {
            bs_fmt(w, "%s", v.as.boolean ? "nocap" : "cap");
        } else {
            bs_fmt(w, "%s", v.as.boolean ? "true" : "false");
        }
        break;

    case BS_VALUE_OBJECT:
        bs_object_write_impl(w, v.as.object, extended);
        break;

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
        if (a.as.object->type != b.as.object->type) {
            return false;
        }

        if (a.as.object->type == BS_OBJECT_STR) {
            return bs_str_eq((const Bs_Str *)a.as.object, (const Bs_Str *)b.as.object);
        }

        return a.as.object == b.as.object;

    default:
        assert(false && "unreachable");
    }
}
