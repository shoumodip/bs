#include <ctype.h>

#include "bs/object.h"

bool bs_value_is_falsey(Bs_Value v) {
    return v.type == BS_VALUE_NIL || (v.type == BS_VALUE_BOOL && !v.as.boolean);
}

const char *bs_value_type_name(Bs_Value_Type type) {
    switch (type) {
    case BS_VALUE_NIL:
        return "nil";

    case BS_VALUE_NUM:
        return "number";

    case BS_VALUE_BOOL:
        return "boolean";

    case BS_VALUE_OBJECT:
        return "object";
    }
}

static_assert(BS_COUNT_OBJECTS == 13, "Update bs_object_type_name()");
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
    case BS_OBJECT_C_CLASS:
        return "class";

    case BS_OBJECT_INSTANCE:
        return "instance";

    case BS_OBJECT_C_INSTANCE:
        return "native instance";

    case BS_OBJECT_C_LIB:
        return "native library";

    default:
        assert(false && "unreachable");
    }
}

Bs_Sv bs_value_type_name_full(Bs_Value v) {
    if (v.type != BS_VALUE_OBJECT) {
        return bs_sv_from_cstr(bs_value_type_name(v.type));
    }

    switch (v.as.object->type) {
    case BS_OBJECT_INSTANCE: {
        const Bs_Str *str = ((const Bs_Instance *)v.as.object)->class->name;
        return Bs_Sv(str->data, str->size);
    } break;

    case BS_OBJECT_C_INSTANCE:
        return ((const Bs_C_Instance *)v.as.object)->class->name;

    default:
        return bs_sv_from_cstr(bs_object_type_name(v.as.object->type));
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
                switch (v.as.object->type) {
                case BS_OBJECT_ARRAY:
                case BS_OBJECT_TABLE:
                case BS_OBJECT_CLASS:
                case BS_OBJECT_INSTANCE:
                case BS_OBJECT_C_CLASS:
                    return true;

                default:
                    break;
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

static void
bs_pretty_printer_map_extra(Bs_Pretty_Printer *p, const Bs_Map *map, const char *extra) {
    bs_fmt(p->writer, "{");
    if (map->length || extra) {
        p->depth++;

        if (extra) {
            bs_fmt(p->writer, "\n%*s", (int)p->depth * BS_PRETTY_PRINT_INDENT, "");
            bs_fmt(p->writer, "%s", extra);
        }

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

void bs_pretty_printer_map(Bs_Pretty_Printer *p, const Bs_Map *map) {
    bs_pretty_printer_map_extra(p, map, NULL);
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

static_assert(BS_COUNT_OBJECTS == 13, "Update bs_object_write()");
static void bs_object_write_impl(Bs_Pretty_Printer *p, const Bs_Object *object) {
    switch (object->type) {
    case BS_OBJECT_FN:
    case BS_OBJECT_CLOSURE:
    case BS_OBJECT_BOUND_METHOD:
    case BS_OBJECT_C_FN:
        bs_fmt(p->writer, "<fn>");
        break;

    case BS_OBJECT_STR: {
        const Bs_Str *str = (const Bs_Str *)object;
        const Bs_Sv sv = Bs_Sv(str->data, str->size);
        if (p->depth) {
            bs_pretty_printer_quote(p, sv);
        } else {
            p->writer->write(p->writer, sv);
        }
    } break;

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
        const Bs_Class *class = (const Bs_Class *)object;
        const Bs_Str *name = class->name;
        bs_fmt(p->writer, "class " Bs_Sv_Fmt " ", Bs_Sv_Arg(*name));
        bs_pretty_printer_map_extra(p, &class->methods, class->can_fail ? "# Can fail" : NULL);
    } break;

    case BS_OBJECT_INSTANCE: {
        const Bs_Instance *instance = (const Bs_Instance *)object;
        if (bs_pretty_printer_has(p, object)) {
            bs_fmt(p->writer, "<" Bs_Sv_Fmt ">", Bs_Sv_Arg(*instance->class->name));
        } else {
            bs_pretty_printer_push(p, object);
            bs_fmt(p->writer, Bs_Sv_Fmt " ", Bs_Sv_Arg(*instance->class->name));
            bs_pretty_printer_map(p, &instance->properties);
        }
    } break;

    case BS_OBJECT_C_CLASS: {
        const Bs_C_Class *class = (const Bs_C_Class *)object;
        bs_fmt(p->writer, "class " Bs_Sv_Fmt " ", Bs_Sv_Arg(class->name));
        bs_pretty_printer_map_extra(p, &class->methods, class->can_fail ? "# Can fail" : NULL);
    } break;

    case BS_OBJECT_C_INSTANCE: {
        const Bs_C_Instance *instance = (const Bs_C_Instance *)object;
        bs_fmt(p->writer, "<" Bs_Sv_Fmt ">", Bs_Sv_Arg(instance->class->name));
    } break;

    case BS_OBJECT_C_LIB:
        bs_pretty_printer_map(p, &((const Bs_C_Lib *)object)->map);
        break;

    default:
        assert(false && "unreachable");
    }
}

void bs_value_write_impl(Bs_Pretty_Printer *p, Bs_Value value) {
    switch (value.type) {
    case BS_VALUE_NIL:
        bs_fmt(p->writer, "nil");
        break;

    case BS_VALUE_NUM:
        bs_fmt(p->writer, "%.15g", value.as.number);
        break;

    case BS_VALUE_BOOL:
        bs_fmt(p->writer, "%s", value.as.boolean ? "true" : "false");
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
