#include <stdio.h>

#include "bs/compiler.h"
#include "bs/core.h"

// IO
static void file_data_free(Bs *bs, void *data) {
    if (data && fileno(data) > 2) {
        fclose(data);
    }
}

static const Bs_C_Data_Spec file_spec = {
    .name = Bs_Sv_Static("file"),
    .free = file_data_free,
};

static Bs_Value io_open(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 2)) {
        return bs_value_error;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1")) {
        return bs_value_error;
    }

    if (!bs_check_value_type(bs, args[1], BS_VALUE_BOOL, "argument #2")) {
        return bs_value_error;
    }

    Bs_Buffer *b = bs_buffer_get(bs);
    const size_t start = b->count;

    Bs_Writer w = bs_buffer_writer(b);
    bs_fmt(&w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[0].as.object));
    bs_buffer_push(b->bs, b, '\0');

    const bool write = args[1].as.boolean;
    FILE *file = fopen(bs_buffer_reset(b, start).data, write ? "w" : "r");
    if (file) {
        return bs_value_object(bs_c_data_new(bs, file, &file_spec));
    }

    return bs_value_nil;
}

static Bs_Value io_close(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 1)) {
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &file_spec, "argument #1")) {
        return bs_value_error;
    }

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (c->data) {
        file_data_free(bs, c->data);
        c->data = NULL;
    }

    return bs_value_nil;
}

static Bs_Value io_read(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 2)) {
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &file_spec, "argument #1")) {
        return bs_value_error;
    }

    if (!bs_check_whole_number(bs, args[1], "argument #2")) {
        return bs_value_error;
    }

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (!c->data) {
        bs_error(bs, "cannot read from closed file");
        return bs_value_error;
    }

    size_t count = args[1].as.number;
    if (!count) {
        const long start = ftell(c->data);
        if (start == -1) {
            return bs_value_nil;
        }

        if (fseek(c->data, 0, SEEK_END) == -1) {
            return bs_value_nil;
        }

        const long offset = ftell(c->data);
        if (offset == -1) {
            return bs_value_nil;
        }

        if (fseek(c->data, start, SEEK_SET) == -1) {
            return bs_value_nil;
        }

        count = offset;
    }

    char *data = malloc(count);
    assert(data);

    count = fread(data, sizeof(char), count, c->data);

    Bs_Value result = bs_value_nil;
    if (!ferror(c->data)) {
        result = bs_value_object(bs_str_new(bs, (Bs_Sv){data, count}));
    }

    free(data);
    return result;
}

static Bs_Value io_flush(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 1)) {
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &file_spec, "argument #1")) {
        return bs_value_error;
    }

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (!c->data) {
        bs_error(bs, "cannot flush closed file");
        return bs_value_error;
    }

    fflush(c->data);
    return bs_value_nil;
}

static Bs_Value io_write(Bs *bs, Bs_Value *args, size_t arity) {
    if (!arity) {
        bs_error(bs, "expected at least 1 argument, got 0");
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &file_spec, "argument #1")) {
        return bs_value_error;
    }

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (!c->data) {
        bs_error(bs, "cannot write into closed file");
        return bs_value_error;
    }

    Bs_Writer w = bs_file_writer(c->data);
    for (size_t i = 1; i < arity; i++) {
        if (i > 1) {
            bs_fmt(&w, " ");
        }
        bs_value_write(&w, args[i]);
    }

    return bs_value_bool(!ferror(c->data));
}

static Bs_Value io_print(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer *w = bs_stdout_get(bs);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(w, " ");
        }
        bs_value_write(w, args[i]);
    }
    return bs_value_nil;
}

static Bs_Value io_eprint(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer *w = bs_stderr_get(bs);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(w, " ");
        }
        bs_value_write(w, args[i]);
    }
    return bs_value_nil;
}

static Bs_Value io_writeln(Bs *bs, Bs_Value *args, size_t arity) {
    if (!arity) {
        bs_error(bs, "expected at least 1 argument, got 0");
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &file_spec, "argument #1")) {
        return bs_value_error;
    }

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (!c->data) {
        bs_error(bs, "cannot write into closed file");
        return bs_value_error;
    }

    Bs_Writer w = bs_file_writer(c->data);
    for (size_t i = 1; i < arity; i++) {
        if (i > 1) {
            bs_fmt(&w, " ");
        }
        bs_value_write(&w, args[i]);
    }
    bs_fmt(&w, "\n");

    return bs_value_bool(!ferror(c->data));
}

static Bs_Value io_println(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer *w = bs_stdout_get(bs);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(w, " ");
        }
        bs_value_write(w, args[i]);
    }
    bs_fmt(w, "\n");
    return bs_value_nil;
}

static Bs_Value io_eprintln(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer *w = bs_stderr_get(bs);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(w, " ");
        }
        bs_value_write(w, args[i]);
    }
    bs_fmt(w, "\n");
    return bs_value_nil;
}

// OS
static Bs_Value os_exit(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 1)) {
        return bs_value_error;
    }

    if (!bs_check_whole_number(bs, args[0], "argument #1")) {
        return bs_value_error;
    }

    return bs_value_halt(args[0].as.number);
}

static Bs_Value os_getenv(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 1)) {
        return bs_value_error;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1")) {
        return bs_value_error;
    }

    {
        Bs_Buffer *b = bs_buffer_get(bs);
        const size_t start = b->count;

        Bs_Writer w = bs_buffer_writer(b);
        bs_fmt(&w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[0].as.object));
        bs_buffer_push(b->bs, b, '\0');

        const char *key = bs_buffer_reset(b, start).data;
        const char *value = getenv(key);

        if (value) {
            return bs_value_object(bs_str_new(bs, bs_sv_from_cstr(value)));
        } else {
            return bs_value_nil;
        }
    }
}

static Bs_Value os_setenv(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 2)) {
        return bs_value_error;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1")) {
        return bs_value_error;
    }

    if (!bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2")) {
        return bs_value_error;
    }

    const char *key;
    const char *value;
    {
        Bs_Buffer *b = bs_buffer_get(bs);
        const size_t start = b->count;

        Bs_Writer w = bs_buffer_writer(b);

        const size_t key_pos = b->count;
        bs_fmt(&w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[0].as.object));
        bs_buffer_push(b->bs, b, '\0');

        const size_t value_pos = b->count;
        bs_fmt(&w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[1].as.object));
        bs_buffer_push(b->bs, b, '\0');

        key = b->data + key_pos;
        value = b->data + value_pos;

        bs_buffer_reset(b, start);
    }

    return bs_value_bool(setenv(key, value, true) == 0);
}

static Bs_Value os_execute(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 1)) {
        return bs_value_error;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1")) {
        return bs_value_error;
    }

    Bs_Buffer *b = bs_buffer_get(bs);
    const size_t start = b->count;

    Bs_Writer w = bs_buffer_writer(b);
    bs_fmt(&w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[0].as.object));
    bs_buffer_push(b->bs, b, '\0');

    const char *command = bs_buffer_reset(b, start).data;

    return bs_value_num(WEXITSTATUS(system(command)));
}

// Str
static Bs_Value str_slice(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 3)) {
        return bs_value_error;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1")) {
        return bs_value_error;
    }

    if (!bs_check_whole_number(bs, args[1], "argument #2")) {
        return bs_value_error;
    }

    if (!bs_check_whole_number(bs, args[2], "argument #3")) {
        return bs_value_error;
    }

    Bs_Str *str = (Bs_Str *)args[0].as.object;
    const size_t begin = bs_min(args[1].as.number, args[2].as.number);
    const size_t end = bs_max(args[1].as.number, args[2].as.number);

    if (begin >= str->size || end > str->size) {
        bs_error(bs, "cannot slice string of length %zu from %zu to %zu", str->size, begin, end);
        return bs_value_error;
    }

    return bs_value_object(bs_str_new(bs, bs_sv_from_parts(str->data + begin, end - begin)));
}

static Bs_Value str_format(Bs *bs, Bs_Value *args, size_t arity) {
    if (!arity) {
        bs_error(bs, "expected at least 1 argument, got 0");
        return bs_value_error;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1")) {
        return bs_value_error;
    }

    const Bs_Str *fmt = (const Bs_Str *)args[0].as.object;

    size_t count = 0;
    for (size_t i = 0; i < fmt->size; i++) {
        if (fmt->data[i] == '$') {
            if (i + 1 < fmt->size && fmt->data[i + 1] == '$') {
                i++;
            } else {
                count++;
            }
        }
    }

    if (!bs_check_arity(bs, arity, count + 1)) {
        return bs_value_error;
    }

    Bs_Buffer *b = bs_buffer_get(bs);
    const size_t start = b->count;

    Bs_Writer w = bs_buffer_writer(b);

    count = 0;
    for (size_t i = 0; i < fmt->size; i++) {
        if (fmt->data[i] == '$') {
            if (i + 1 < fmt->size && fmt->data[i + 1] == '$') {
                bs_buffer_push(b->bs, b, '$');
                i++;
            } else {
                bs_value_write(&w, args[++count]);
            }
        } else {
            bs_buffer_push(b->bs, b, fmt->data[i]);
        }
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

static Bs_Value str_reverse(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 1)) {
        return bs_value_error;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1")) {
        return bs_value_error;
    }

    Bs_Str *src = (Bs_Str *)args[0].as.object;
    Bs_Str *dst = bs_str_new(bs, bs_sv_from_parts(NULL, src->size));
    for (size_t i = 0; i < src->size; i++) {
        dst->data[dst->size - i - 1] = src->data[i];
    }

    return bs_value_object(dst);
}

// Array
static Bs_Value array_join(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 2)) {
        return bs_value_error;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_ARRAY, "argument #1")) {
        return bs_value_error;
    }

    if (!bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2")) {
        return bs_value_error;
    }

    Bs_Buffer *b = bs_buffer_get(bs);
    const size_t start = b->count;
    {
        Bs_Writer w = bs_buffer_writer(b);

        const Bs_Array *array = (const Bs_Array *)args[0].as.object;
        const Bs_Str *str = (const Bs_Str *)args[1].as.object;
        const Bs_Sv separator = bs_sv_from_parts(str->data, str->size);

        for (size_t i = 0; i < array->count; i++) {
            if (i) {
                w.write(&w, separator);
            }
            bs_value_write(&w, array->data[i]);
        }
    }
    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

// Main
static void bs_table_add(Bs *bs, Bs_Table *table, const char *name, Bs_Value value) {
    bs_table_set(bs, table, bs_value_object(bs_str_const(bs, bs_sv_from_cstr(name))), value);
}

int bs_core_init(Bs *bs, int argc, char **argv) {
    {
        Bs_Table *io = bs_table_new(bs);
        bs_table_add(bs, io, "open", bs_value_object(bs_c_fn_new(bs, io_open)));
        bs_table_add(bs, io, "close", bs_value_object(bs_c_fn_new(bs, io_close)));
        bs_table_add(bs, io, "read", bs_value_object(bs_c_fn_new(bs, io_read)));
        bs_table_add(bs, io, "flush", bs_value_object(bs_c_fn_new(bs, io_flush)));

        bs_table_add(bs, io, "write", bs_value_object(bs_c_fn_new(bs, io_write)));
        bs_table_add(bs, io, "print", bs_value_object(bs_c_fn_new(bs, io_print)));
        bs_table_add(bs, io, "eprint", bs_value_object(bs_c_fn_new(bs, io_eprint)));

        bs_table_add(bs, io, "writeln", bs_value_object(bs_c_fn_new(bs, io_writeln)));
        bs_table_add(bs, io, "println", bs_value_object(bs_c_fn_new(bs, io_println)));
        bs_table_add(bs, io, "eprintln", bs_value_object(bs_c_fn_new(bs, io_eprintln)));

        bs_table_add(bs, io, "stdin", bs_value_object(bs_c_data_new(bs, stdin, &file_spec)));
        bs_table_add(bs, io, "stdout", bs_value_object(bs_c_data_new(bs, stdout, &file_spec)));
        bs_table_add(bs, io, "stderr", bs_value_object(bs_c_data_new(bs, stderr, &file_spec)));

        bs_global_set(bs, Bs_Sv_Static("io"), bs_value_object(io));
    }

    {
        Bs_Table *os = bs_table_new(bs);
        bs_table_add(bs, os, "exit", bs_value_object(bs_c_fn_new(bs, os_exit)));
        bs_table_add(bs, os, "getenv", bs_value_object(bs_c_fn_new(bs, os_getenv)));
        bs_table_add(bs, os, "setenv", bs_value_object(bs_c_fn_new(bs, os_setenv)));
        bs_table_add(bs, os, "execute", bs_value_object(bs_c_fn_new(bs, os_execute)));

        Bs_Array *args = bs_array_new(bs);
        for (int i = 0; i < argc; i++) {
            bs_array_set(bs, args, i, bs_value_object(bs_str_const(bs, bs_sv_from_cstr(argv[i]))));
        }
        bs_table_add(bs, os, "args", bs_value_object(args));

        bs_global_set(bs, Bs_Sv_Static("os"), bs_value_object(os));
    }

    {
        Bs_Table *str = bs_table_new(bs);
        bs_table_add(bs, str, "slice", bs_value_object(bs_c_fn_new(bs, str_slice)));
        bs_table_add(bs, str, "format", bs_value_object(bs_c_fn_new(bs, str_format)));
        bs_table_add(bs, str, "reverse", bs_value_object(bs_c_fn_new(bs, str_reverse)));
        bs_global_set(bs, Bs_Sv_Static("str"), bs_value_object(str));
    }

    {
        Bs_Table *array = bs_table_new(bs);
        bs_table_add(bs, array, "join", bs_value_object(bs_c_fn_new(bs, array_join)));
        bs_global_set(bs, Bs_Sv_Static("array"), bs_value_object(array));
    }

    return bs_run(
        bs,
        "",
        Bs_Sv_Static("array.map = fn (a, f) {"
                     "    var b = [];"
                     "    for i, v in a {"
                     "        b[i] = f(v);"
                     "    }"
                     "    return b;"
                     "};"

                     "array.filter = fn (a, f) {"
                     "    var b = [];"
                     "    for i, v in a {"
                     "        if f(v) {"
                     "            b[@b] = v;"
                     "        }"
                     "    }"
                     "    return b;"
                     "};"

                     "array.reduce = fn (a, b, f) {"
                     "    for i, v in a {"
                     "        if b == nil {"
                     "            b = v;"
                     "        } else {"
                     "            b = f(b, v);"
                     "        }"
                     "    }"
                     "    return b;"
                     "};"),
        false);
}
