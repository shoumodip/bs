#include <stdio.h>

#include "bs/object.h"

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

    const Bs_Str *path = (Bs_Str *)args[0].as.object;
    const bool write = args[1].as.boolean;

    size_t start;
    Bs_Writer *w = bs_str_writer_init(bs, &start);
    bs_write(w, Bs_Sv_Fmt, Bs_Sv_Arg(*path));

    Bs_Sv path_native = bs_str_writer_end(bs, start);
    FILE *file = fopen(path_native.data, write ? "w" : "r");
    if (file) {
        *result = bs_value_object(bs_c_data_new(bs, file, &file_spec));
    }

    return true;
}

static bool io_close(Bs *bs, Bs_Value *args, size_t arity, Bs_Value *result) {
    if (!bs_check_arity(bs, arity, 1)) {
        return false;
    }

    if (!bs_check_object_c_type(bs, args[0], &file_spec, "argument #1")) {
        return false;
    }

    Bs_C_Data *native = (Bs_C_Data *)args[0].as.object;
    if (native->data) {
        file_data_free(bs, native->data);
        native->data = NULL;
    }

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

    Bs_C_Data *native = (Bs_C_Data *)args[0].as.object;
    if (!native->data) {
        bs_error(bs, "cannot write into closed file");
        return false;
    }

    Bs_Str *bytes = (Bs_Str *)args[1].as.object;
    fwrite(bytes->data, bytes->size, 1, native->data);
    *result = bs_value_bool(!ferror(native->data));

    return true;
}

int main(int argc, char **argv) {
    int result = 0;

    if (argc < 2) {
        fprintf(stderr, "error: file path not provided\n");
        fprintf(stderr, "usage: %s <path>\n", *argv);
        exit(1);
    }
    const char *path = argv[1];

    size_t size = 0;
    char *contents = bs_read_file(path, &size);
    if (!contents) {
        fprintf(stderr, "error: could not read file '%s'\n", path);
        exit(1);
    }

    Bs *bs = bs_new();

    Bs_Table *string = bs_table_new(bs);
    bs_table_set(
        bs,
        string,
        bs_str_const(bs, Bs_Sv_Static("slice")),
        bs_value_object(bs_c_fn_new(bs, string_slice)));

    bs_core_set(bs, Bs_Sv_Static("string"), bs_value_object(string));

    Bs_Table *io = bs_table_new(bs);
    bs_table_set(
        bs, io, bs_str_const(bs, Bs_Sv_Static("open")), bs_value_object(bs_c_fn_new(bs, io_open)));

    bs_table_set(
        bs,
        io,
        bs_str_const(bs, Bs_Sv_Static("close")),
        bs_value_object(bs_c_fn_new(bs, io_close)));

    bs_table_set(
        bs,
        io,
        bs_str_const(bs, Bs_Sv_Static("write")),
        bs_value_object(bs_c_fn_new(bs, io_write)));

    bs_core_set(bs, Bs_Sv_Static("io"), bs_value_object(io));

    if (!bs_run(bs, path, bs_sv_from_parts(contents, size), false)) {
        bs_return_defer(1);
    }

defer:
    bs_free(bs);
    free(contents);
    return result;
}
