#ifndef BS_VALUE_H
#define BS_VALUE_H

#include <stdint.h>

#include "basic.h"

#define BS_PRETTY_PRINT_INDENT 4

typedef enum {
    BS_VALUE_NIL,
    BS_VALUE_NUM,
    BS_VALUE_BOOL,
    BS_VALUE_OBJECT,
} Bs_Value_Type;

typedef enum {
    BS_OBJECT_FN,
    BS_OBJECT_STR,
    BS_OBJECT_ARRAY,
    BS_OBJECT_TABLE,
    BS_OBJECT_CLOSURE,
    BS_OBJECT_UPVALUE,

    BS_OBJECT_CLASS,
    BS_OBJECT_INSTANCE,
    BS_OBJECT_BOUND_METHOD,

    BS_OBJECT_C_FN,
    BS_OBJECT_C_LIB,
    BS_OBJECT_C_DATA,
    BS_COUNT_OBJECTS
} Bs_Object_Type;

const char *bs_object_type_name(Bs_Object_Type type);

typedef struct Bs_Object Bs_Object;

typedef struct Bs_Fn Bs_Fn;
typedef struct Bs_Str Bs_Str;
typedef struct Bs_Array Bs_Array;
typedef struct Bs_Table Bs_Table;
typedef struct Bs_Closure Bs_Closure;
typedef struct Bs_Upvalue Bs_Upvalue;

typedef struct Bs_Class Bs_Class;
typedef struct Bs_Instance Bs_Instance;
typedef struct Bs_Bound_Method Bs_Bound_Method;

typedef struct Bs_C_Fn Bs_C_Fn;
typedef struct Bs_C_Lib Bs_C_Lib;
typedef struct Bs_C_Data Bs_C_Data;

typedef struct Bs_Map Bs_Map;

typedef struct {
    Bs_Value_Type type;
    union {
        bool boolean;
        double number;
        Bs_Object *object;
    } as;
} Bs_Value;

#define bs_value_nil ((Bs_Value){BS_VALUE_NIL})
#define bs_value_num(v) ((Bs_Value){BS_VALUE_NUM, .as.number = (v)})
#define bs_value_bool(v) ((Bs_Value){BS_VALUE_BOOL, .as.boolean = (v)})
#define bs_value_object(v) ((Bs_Value){BS_VALUE_OBJECT, .as.object = (Bs_Object *)(v)})

bool bs_value_is_falsey(Bs_Value value);
const char *bs_value_type_name(Bs_Value value, bool extended);

typedef struct {
    Bs_Writer *writer;

    bool extended;
    size_t depth;

    const Bs_Object **data;
    size_t count;
    size_t capacity;
} Bs_Pretty_Printer;

void bs_pretty_printer_free(Bs_Pretty_Printer *p);
void bs_pretty_printer_push(Bs_Pretty_Printer *p, const Bs_Object *object);
bool bs_pretty_printer_has(Bs_Pretty_Printer *p, const Bs_Object *object);

void bs_pretty_printer_map(Bs_Pretty_Printer *p, const Bs_Map *map);
void bs_pretty_printer_quote(Bs_Pretty_Printer *p, Bs_Sv sv);

void bs_value_write_impl(Bs_Pretty_Printer *printer, Bs_Value value);
bool bs_value_equal(Bs_Value a, Bs_Value b);

#endif // BS_VALUE_H
