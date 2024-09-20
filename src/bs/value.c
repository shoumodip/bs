#include <ctype.h>

#include "bs/object.h"

bool bs_value_is_falsey(Bs_Value v) {
    return v.type == BS_VALUE_NIL || (v.type == BS_VALUE_BOOL && !v.as.boolean);
}

static_assert(BS_COUNT_OBJECTS == 12, "Update bs_object_type_name()");
const char *bs_object_type_name(Bs_Object_Type type) {
    switch (type) {
    case BS_OBJECT_FN:
    case BS_OBJECT_CLOSURE:
    case BS_OBJECT_BOUND_METHOD:
    case BS_OBJECT_C_FN:
        return "function";

    case BS_OBJECT_STR:
        return "string";

    case BS_OBJECT_ARRAY:
        return "array";

    case BS_OBJECT_TABLE:
        return "table";

    case BS_OBJECT_UPVALUE:
        return "upvalue";

    case BS_OBJECT_CLASS:
        return "class";

    case BS_OBJECT_INSTANCE:
        return "instance";

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

static bool bs_should_print_multi(const Bs_Object *object) {
    if (object->type == BS_OBJECT_TABLE) {
        return ((const Bs_Table *)object)->map.length != 0;
    }

    if (object->type == BS_OBJECT_ARRAY) {
        const Bs_Array *array = (const Bs_Array *)object;
        if (array->count > 5) {
            return true;
        }

        for (size_t i = 0; i < array->count; i++) {
            const Bs_Value v = array->data[i];
            if (v.type == BS_VALUE_OBJECT) {
                if (v.as.object->type == BS_OBJECT_ARRAY || v.as.object->type == BS_OBJECT_TABLE) {
                    return true;
                }
            }
        }
    }

    return false;
}

void bs_pretty_printer_free(Bs_Pretty_Printer *p) {
    free(p->data);
    memset(p, '\0', sizeof(*p));
}

void bs_pretty_printer_push(Bs_Pretty_Printer *p, const Bs_Object *object) {
    if (p->count >= p->capacity) {
        p->capacity = p->capacity ? p->capacity * 2 : BS_DA_INIT_CAP;
        p->data = realloc(p->data, p->capacity * sizeof(const Bs_Object *));
        assert(p->data);
    }

    p->data[p->count++] = object;
}

bool bs_pretty_printer_has(Bs_Pretty_Printer *p, const Bs_Object *object) {
    for (size_t i = 0; i < p->count; i++) {
        if (p->data[i] == object) {
            return true;
        }
    }

    return false;
}

void bs_pretty_printer_map(Bs_Pretty_Printer *p, const Bs_Map *map) {
    bs_fmt(p->writer, "{");
    if (map->length) {
        p->depth++;

        for (size_t i = 0, count = 0; i < map->capacity; i++) {
            Bs_Entry *entry = &map->data[i];
            if (entry->key.type == BS_VALUE_NIL) {
                continue;
            }

            if (count) {
                bs_fmt(p->writer, ",");
            }
            bs_fmt(p->writer, "\n%*s", (int)p->depth * BS_PRETTY_PRINT_INDENT, "");

            bool symbol =
                entry->key.type == BS_VALUE_OBJECT && entry->key.as.object->type == BS_OBJECT_STR;

            if (symbol) {
                const Bs_Str *str = (const Bs_Str *)entry->key.as.object;
                if (str->size && (isalpha(*str->data) || *str->data == '_')) {
                    for (size_t i = 0; i < str->size; i++) {
                        if (!isalnum(str->data[i]) && str->data[i] != '_') {
                            symbol = false;
                            break;
                        }
                    }
                } else {
                    symbol = false;
                }
            }

            if (symbol) {
                bs_fmt(p->writer, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)entry->key.as.object));
            } else {
                p->writer->write(p->writer, Bs_Sv_Static("["));
                bs_value_write_impl(p, entry->key);
                p->writer->write(p->writer, Bs_Sv_Static("]"));
            }
            bs_fmt(p->writer, " = ");
            bs_value_write_impl(p, entry->value);
            count++;
        }

        p->depth--;
        bs_fmt(p->writer, "\n%*s", (int)p->depth * BS_PRETTY_PRINT_INDENT, "");
    }
    bs_fmt(p->writer, "}");
}

void bs_pretty_printer_quote(Bs_Pretty_Printer *p, Bs_Sv sv) {
    bs_fmt(p->writer, "\"");
    while (sv.size) {
        size_t index;
        if (bs_sv_find(sv, '"', &index)) {
            p->writer->write(p->writer, bs_sv_drop(&sv, index));
            p->writer->write(p->writer, Bs_Sv_Static("\\\""));
            bs_sv_drop(&sv, 1);
        } else {
            p->writer->write(p->writer, bs_sv_drop(&sv, sv.size));
        }
    }
    bs_fmt(p->writer, "\"");
}

static_assert(BS_COUNT_OBJECTS == 12, "Update bs_object_write()");
static void bs_object_write_impl(Bs_Pretty_Printer *p, const Bs_Object *object) {
    switch (object->type) {
    case BS_OBJECT_FN:
    case BS_OBJECT_CLOSURE:
    case BS_OBJECT_BOUND_METHOD:
    case BS_OBJECT_C_FN:
        bs_fmt(p->writer, "<fn>");
        break;

    case BS_OBJECT_STR:
        if (p->depth) {
            const Bs_Str *str = (const Bs_Str *)object;
            bs_pretty_printer_quote(p, Bs_Sv(str->data, str->size));
        } else {
            bs_fmt(p->writer, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)object));
        }
        break;

    case BS_OBJECT_ARRAY: {
        if (bs_pretty_printer_has(p, object)) {
            bs_fmt(p->writer, "<array>");
        } else {
            bs_pretty_printer_push(p, object);
            const Bs_Array *array = (const Bs_Array *)object;

            const bool multi = bs_should_print_multi(object);
            p->depth++;

            bs_fmt(p->writer, "[");
            for (size_t i = 0; i < array->count; i++) {
                if (i) {
                    bs_fmt(p->writer, ",");
                }

                if (multi) {
                    bs_fmt(p->writer, "\n%*s", (int)p->depth * BS_PRETTY_PRINT_INDENT, "");
                } else if (i) {
                    bs_fmt(p->writer, " ");
                }

                bs_value_write_impl(p, array->data[i]);
            }

            p->depth--;
            if (multi) {
                bs_fmt(p->writer, "\n%*s", (int)p->depth * BS_PRETTY_PRINT_INDENT, "");
            }
            bs_fmt(p->writer, "]");
        }
    } break;

    case BS_OBJECT_TABLE: {
        if (bs_pretty_printer_has(p, object)) {
            bs_fmt(p->writer, "<table>");
        } else {
            bs_pretty_printer_push(p, object);
            bs_pretty_printer_map(p, &((const Bs_Table *)object)->map);
        }
    } break;

    case BS_OBJECT_UPVALUE:
        bs_fmt(p->writer, "<upvalue>");
        break;

    case BS_OBJECT_CLASS: {
        const Bs_Str *name = ((const Bs_Class *)object)->name;
        p->writer->write(p->writer, Bs_Sv(name->data, name->size));
    } break;

    case BS_OBJECT_INSTANCE: {
        const Bs_Instance *instance = (const Bs_Instance *)object;
        if (bs_pretty_printer_has(p, object)) {
            bs_fmt(p->writer, "<" Bs_Sv_Fmt ">", Bs_Sv_Arg(*instance->class->name));
        } else {
            bs_pretty_printer_push(p, object);
            bs_fmt(p->writer, Bs_Sv_Fmt " ", Bs_Sv_Arg(*instance->class->name));
            bs_pretty_printer_map(p, &instance->fields);
        }
    } break;

    case BS_OBJECT_C_LIB:
        bs_pretty_printer_map(p, &((const Bs_C_Lib *)object)->functions);
        break;

    case BS_OBJECT_C_DATA: {
        const Bs_C_Data *native = (const Bs_C_Data *)object;
        if (native->spec->write) {
            native->spec->write(p->writer, native->data);
        } else {
            bs_fmt(p->writer, "<native " Bs_Sv_Fmt " object>", Bs_Sv_Arg(native->spec->name));
        }
    } break;

    default:
        assert(false && "unreachable");
    }
}

void bs_value_write_impl(Bs_Pretty_Printer *p, Bs_Value value) {
    switch (value.type) {
    case BS_VALUE_NIL:
        if (p->extended) {
            bs_fmt(p->writer, "bruh");
        } else {
            bs_fmt(p->writer, "nil");
        }
        break;

    case BS_VALUE_NUM:
        bs_fmt(p->writer, "%.15g", value.as.number);
        break;

    case BS_VALUE_BOOL:
        if (p->extended) {
            bs_fmt(p->writer, "%s", value.as.boolean ? "nocap" : "cap");
        } else {
            bs_fmt(p->writer, "%s", value.as.boolean ? "true" : "false");
        }
        break;

    case BS_VALUE_OBJECT:
        bs_object_write_impl(p, value.as.object);
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
        return a.as.object == b.as.object;

    default:
        assert(false && "unreachable");
    }
}
