#include <stdio.h>

#include "bs/core.h"
#include "bs/object.h"

static void file_data_free(Bs *bs, void *data) {
    if (data) {
        fclose(data);
    }
}

static const Bs_C_Data_Spec file_spec = {
    .name = Bs_Sv_Static("file"),
    .free = file_data_free,
};

static bool io_open(Bs *bs, Bs_Value *args, size_t arity, Bs_Value *result) {
    if (!bs_check_arity(bs, arity, 2)) {
        return false;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1")) {
        return false;
    }

    if (!bs_check_value_type(bs, args[1], BS_VALUE_BOOL, "argument #2")) {
        return false;
    }

    size_t start;
    Bs_Writer *w = bs_str_writer_init(bs, &start);
    bs_fmt(w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[0].as.object));

    const bool write = args[1].as.boolean;
    FILE *file = fopen(bs_str_writer_end(bs, start).data, write ? "w" : "r");
    if (file) {
        *result = bs_value_object(bs_c_data_new(bs, file, &file_spec));
    }

    return true;
}

static bool io_read(Bs *bs, Bs_Value *args, size_t arity, Bs_Value *result) {
    if (!bs_check_arity(bs, arity, 2)) {
        return false;
    }

    if (!bs_check_object_c_type(bs, args[0], &file_spec, "argument #1")) {
        return false;
    }

    if (!bs_check_whole_number(bs, args[1], "argument #2")) {
        return false;
    }

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (!c->data) {
        bs_error(bs, "cannot read from closed file");
        return false;
    }

    size_t count = args[1].as.number;
    if (!count) {
        const long start = ftell(c->data);
        if (start == -1) {
            return true;
        }

        if (fseek(c->data, 0, SEEK_END) == -1) {
            return true;
        }

        const long offset = ftell(c->data);
        if (offset == -1) {
            return true;
        }

        if (fseek(c->data, start, SEEK_SET) == -1) {
            return true;
        }

        count = offset;
    }

    char *data = malloc(count);
    assert(data);

    count = fread(data, sizeof(char), count, c->data);
    if (!ferror(c->data)) {
        *result = bs_value_object(bs_str_new(bs, (Bs_Sv){data, count}));
    }

    free(data);
    return true;
}

static bool io_write(Bs *bs, Bs_Value *args, size_t arity, Bs_Value *result) {
    if (!bs_check_arity(bs, arity, 2)) {
        return false;
    }

    if (!bs_check_object_c_type(bs, args[0], &file_spec, "argument #1")) {
        return false;
    }

    if (!bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2")) {
        return false;
    }

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (!c->data) {
        bs_error(bs, "cannot write into closed file");
        return false;
    }

    Bs_Str *bytes = (Bs_Str *)args[1].as.object;
    fwrite(bytes->data, bytes->size, 1, c->data);
    *result = bs_value_bool(!ferror(c->data));

    return true;
}

static bool io_flush(Bs *bs, Bs_Value *args, size_t arity, Bs_Value *result) {
    if (!bs_check_arity(bs, arity, 1)) {
        return false;
    }

    if (!bs_check_object_c_type(bs, args[0], &file_spec, "argument #1")) {
        return false;
    }

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (!c->data) {
        bs_error(bs, "cannot flush closed file");
        return false;
    }

    fflush(c->data);
    return true;
}

static bool io_close(Bs *bs, Bs_Value *args, size_t arity, Bs_Value *result) {
    if (!bs_check_arity(bs, arity, 1)) {
        return false;
    }

    if (!bs_check_object_c_type(bs, args[0], &file_spec, "argument #1")) {
        return false;
    }

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (c->data) {
        file_data_free(bs, c->data);
        c->data = NULL;
    }

    return true;
}

static bool os_exit(Bs *bs, Bs_Value *args, size_t arity, Bs_Value *result) {
    if (!bs_check_arity(bs, arity, 1)) {
        return false;
    }

    if (!bs_check_whole_number(bs, args[0], "argument #1")) {
        return false;
    }

    bs_exit_set(bs, args[0].as.number);
    return true;
}

static bool os_getenv(Bs *bs, Bs_Value *args, size_t arity, Bs_Value *result) {
    if (!bs_check_arity(bs, arity, 1)) {
        return false;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1")) {
        return false;
    }

    {
        size_t start;
        Bs_Writer *w = bs_str_writer_init(bs, &start);
        bs_fmt(w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[0].as.object));

        const char *key = bs_str_writer_end(bs, start).data;
        const char *value = getenv(key);

        if (value) {
            *result = bs_value_object(bs_str_new(bs, bs_sv_from_cstr(value)));
        } else {
            *result = bs_value_nil;
        }
    }

    return true;
}

static bool os_setenv(Bs *bs, Bs_Value *args, size_t arity, Bs_Value *result) {
    if (!bs_check_arity(bs, arity, 2)) {
        return false;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1")) {
        return false;
    }

    if (!bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2")) {
        return false;
    }

    const char *key;
    const char *value;
    {
        size_t start;
        Bs_Writer *w = bs_str_writer_init(bs, &start);

        const size_t key_pos = bs_str_writer_pos(bs);
        bs_fmt(w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[0].as.object));
        bs_str_writer_null(bs);

        const size_t value_pos = bs_str_writer_pos(bs);
        bs_fmt(w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[1].as.object));

        key = bs_str_writer_get(bs, key_pos);
        value = bs_str_writer_get(bs, value_pos);

        bs_str_writer_end(bs, start);
    }

    *result = bs_value_bool(setenv(key, value, true) == 0);
    return true;
}

static bool os_execute(Bs *bs, Bs_Value *args, size_t arity, Bs_Value *result) {
    if (!bs_check_arity(bs, arity, 1)) {
        return false;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1")) {
        return false;
    }

    size_t start;
    {
        Bs_Writer *w = bs_str_writer_init(bs, &start);
        bs_fmt(w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[0].as.object));
    }
    const char *command = bs_str_writer_end(bs, start).data;

    *result = bs_value_num(WEXITSTATUS(system(command)));
    return true;
}

static bool string_slice(Bs *bs, Bs_Value *args, size_t arity, Bs_Value *result) {
    if (!bs_check_arity(bs, arity, 3)) {
        return false;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1")) {
        return false;
    }

    if (!bs_check_whole_number(bs, args[1], "argument #2")) {
        return false;
    }

    if (!bs_check_whole_number(bs, args[2], "argument #3")) {
        return false;
    }

    Bs_Str *str = (Bs_Str *)args[0].as.object;
    const size_t begin = bs_min(args[1].as.number, args[2].as.number);
    const size_t end = bs_max(args[1].as.number, args[2].as.number);

    if (begin >= str->size || end > str->size) {
        bs_error(bs, "cannot slice string of length %zu from %zu to %zu", str->size, begin, end);
        return false;
    }

    *result = bs_value_object(bs_str_new(bs, bs_sv_from_parts(str->data + begin, end - begin)));

    return true;
}

static void bs_table_set_fn(Bs *bs, Bs_Table *table, const char *name, Bs_C_Fn_Ptr fn) {
    bs_table_set(
        bs, table, bs_str_const(bs, bs_sv_from_cstr(name)), bs_value_object(bs_c_fn_new(bs, fn)));
}

void bs_core_init(Bs *bs, int argc, char **argv) {
    {
        Bs_Table *io = bs_table_new(bs);
        bs_table_set_fn(bs, io, "open", io_open);
        bs_table_set_fn(bs, io, "read", io_read);
        bs_table_set_fn(bs, io, "write", io_write);
        bs_table_set_fn(bs, io, "flush", io_flush);
        bs_table_set_fn(bs, io, "close", io_close);

        bs_table_set(
            bs,
            io,
            bs_str_const(bs, Bs_Sv_Static("stdin")),
            bs_value_object(bs_c_data_new(bs, stdin, &file_spec)));

        bs_table_set(
            bs,
            io,
            bs_str_const(bs, Bs_Sv_Static("stdout")),
            bs_value_object(bs_c_data_new(bs, stdout, &file_spec)));

        bs_table_set(
            bs,
            io,
            bs_str_const(bs, Bs_Sv_Static("stderr")),
            bs_value_object(bs_c_data_new(bs, stderr, &file_spec)));

        bs_core_set(bs, Bs_Sv_Static("io"), bs_value_object(io));
    }

    {
        Bs_Table *os = bs_table_new(bs);
        bs_table_set_fn(bs, os, "exit", os_exit);
        bs_table_set_fn(bs, os, "getenv", os_getenv);
        bs_table_set_fn(bs, os, "setenv", os_setenv);
        bs_table_set_fn(bs, os, "execute", os_execute);

        Bs_Array *args = bs_array_new(bs);
        for (int i = 0; i < argc; i++) {
            bs_array_set(bs, args, i, bs_value_object(bs_str_const(bs, bs_sv_from_cstr(argv[i]))));
        }
        bs_table_set(bs, os, bs_str_const(bs, Bs_Sv_Static("args")), bs_value_object(args));

        bs_core_set(bs, Bs_Sv_Static("os"), bs_value_object(os));
    }

    {
        Bs_Table *string = bs_table_new(bs);
        bs_table_set_fn(bs, string, "slice", string_slice);
        bs_core_set(bs, Bs_Sv_Static("string"), bs_value_object(string));
    }
}