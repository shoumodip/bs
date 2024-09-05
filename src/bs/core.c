#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "bs/core.h"
#include "bs/object.h"

// IO
static void bs_file_data_free(Bs *bs, void *data) {
    if (data && fileno(data) > 2) {
        fclose(data);
    }
}

static const Bs_C_Data_Spec bs_file_data_spec = {
    .name = Bs_Sv_Static("file"),
    .free = bs_file_data_free,
};

static Bs_Value bs_io_open(Bs *bs, Bs_Value *args, size_t arity) {
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
        return bs_value_object(bs_c_data_new(bs, file, &bs_file_data_spec));
    }

    return bs_value_nil;
}

static Bs_Value bs_io_close(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 1)) {
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &bs_file_data_spec, "argument #1")) {
        return bs_value_error;
    }

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (!c->data) {
        bs_error(bs, "cannot close already closed file");
        return bs_value_error;
    }

    bs_file_data_free(bs, c->data);
    c->data = NULL;

    return bs_value_nil;
}

static Bs_Value bs_io_read(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 2)) {
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &bs_file_data_spec, "argument #1")) {
        return bs_value_error;
    }

    if (!bs_check_whole_number(bs, args[1], "argument #2")) {
        return bs_value_error;
    }

    FILE *f = ((Bs_C_Data *)args[0].as.object)->data;
    if (!f) {
        bs_error(bs, "cannot read from closed file");
        return bs_value_error;
    }

    size_t count = args[1].as.number;
    if (!count) {
        const long start = ftell(f);
        if (start == -1) {
            return bs_value_nil;
        }

        if (fseek(f, 0, SEEK_END) == -1) {
            return bs_value_nil;
        }

        const long offset = ftell(f);
        if (offset == -1) {
            return bs_value_nil;
        }

        if (fseek(f, start, SEEK_SET) == -1) {
            return bs_value_nil;
        }

        count = offset;
    }

    char *data = malloc(count);
    assert(data);

    count = fread(data, sizeof(char), count, f);

    Bs_Value result = bs_value_nil;
    if (!ferror(f)) {
        result = bs_value_object(bs_str_new(bs, (Bs_Sv){data, count}));
    }

    free(data);
    return result;
}

static Bs_Value bs_io_flush(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 1)) {
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &bs_file_data_spec, "argument #1")) {
        return bs_value_error;
    }

    FILE *f = ((Bs_C_Data *)args[0].as.object)->data;
    if (!f) {
        bs_error(bs, "cannot flush closed file");
        return bs_value_error;
    }

    fflush(f);
    return bs_value_nil;
}

static Bs_Value bs_io_write(Bs *bs, Bs_Value *args, size_t arity) {
    if (!arity) {
        bs_error(bs, "expected at least 1 argument, got 0");
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &bs_file_data_spec, "argument #1")) {
        return bs_value_error;
    }

    FILE *f = ((Bs_C_Data *)args[0].as.object)->data;
    if (!f) {
        bs_error(bs, "cannot write into closed file");
        return bs_value_error;
    }

    Bs_Writer w = bs_file_writer(f);
    for (size_t i = 1; i < arity; i++) {
        if (i > 1) {
            bs_fmt(&w, " ");
        }
        bs_value_write(&w, args[i]);
    }

    return bs_value_bool(!ferror(f));
}

static Bs_Value bs_io_print(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer *w = bs_stdout_get(bs);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(w, " ");
        }
        bs_value_write(w, args[i]);
    }
    return bs_value_nil;
}

static Bs_Value bs_io_eprint(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer *w = bs_stderr_get(bs);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(w, " ");
        }
        bs_value_write(w, args[i]);
    }
    return bs_value_nil;
}

static Bs_Value bs_io_writeln(Bs *bs, Bs_Value *args, size_t arity) {
    if (!arity) {
        bs_error(bs, "expected at least 1 argument, got 0");
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &bs_file_data_spec, "argument #1")) {
        return bs_value_error;
    }

    FILE *f = ((Bs_C_Data *)args[0].as.object)->data;
    if (!f) {
        bs_error(bs, "cannot write into closed file");
        return bs_value_error;
    }

    Bs_Writer w = bs_file_writer(f);
    for (size_t i = 1; i < arity; i++) {
        if (i > 1) {
            bs_fmt(&w, " ");
        }
        bs_value_write(&w, args[i]);
    }
    bs_fmt(&w, "\n");

    return bs_value_bool(!ferror(f));
}

static Bs_Value bs_io_println(Bs *bs, Bs_Value *args, size_t arity) {
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

static Bs_Value bs_io_eprintln(Bs *bs, Bs_Value *args, size_t arity) {
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
static Bs_Value bs_os_exit(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 1)) {
        return bs_value_error;
    }

    if (!bs_check_whole_number(bs, args[0], "argument #1")) {
        return bs_value_error;
    }

    return bs_value_halt(args[0].as.number);
}

static Bs_Value bs_os_getenv(Bs *bs, Bs_Value *args, size_t arity) {
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

static Bs_Value bs_os_setenv(Bs *bs, Bs_Value *args, size_t arity) {
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

// Process
static const Bs_C_Data_Spec bs_process_data_spec = {
    .name = Bs_Sv_Static("process"),
};

static Bs_Value bs_process_kill(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 2)) {
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &bs_process_data_spec, "argument #1")) {
        return bs_value_error;
    }

    if (!bs_check_whole_number(bs, args[1], "argument #2")) {
        return bs_value_error;
    }

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (!c->data) {
        bs_error(bs, "cannot kill already terminated process");
        return bs_value_error;
    }

    const pid_t pid = (uintptr_t)c->data;
    c->data = NULL;

    if (kill(pid, args[1].as.number) < 0) {
        bs_error(bs, "could not kill process");
        return bs_value_error;
    }

    return bs_value_nil;
}

static Bs_Value bs_process_wait(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 1)) {
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &bs_process_data_spec, "argument #1")) {
        return bs_value_error;
    }

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (!c->data) {
        bs_error(bs, "cannot wait for already terminated process");
        return bs_value_error;
    }

    const pid_t pid = (uintptr_t)c->data;
    c->data = NULL;

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        bs_error(bs, "could not wait for process");
        return bs_value_error;
    }

    return WIFEXITED(status) ? bs_value_num(WEXITSTATUS(status)) : bs_value_nil;
}

static Bs_Value bs_process_spawn(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 1)) {
        return bs_value_error;
    }

    if (!bs_check_object_type(bs, args[0], BS_OBJECT_ARRAY, "argument #1")) {
        return bs_value_error;
    }

    const Bs_Array *array = (const Bs_Array *)args[0].as.object;
    if (!array->count) {
        bs_error(bs, "cannot execute empty array as process");
        return bs_value_error;
    }

    for (size_t i = 0; i < array->count; i++) {
        if (!bs_check_object_type(bs, array->data[i], BS_OBJECT_STR, "command argument")) {
            return bs_value_error;
        }
    }

    const pid_t pid = fork();
    if (pid < 0) {
        bs_error(bs, "could not fork process");
        return bs_value_error;
    }

    if (pid == 0) {
        Bs_Buffer *b = bs_buffer_get(bs);
        const size_t start = b->count;

        char **cargv = malloc((array->count + 1) * sizeof(char *));
        assert(cargv);

        for (size_t i = 0; i < array->count; i++) {
            // Store the offset instead of the actual pointer, because the buffer might be
            // reallocated
            cargv[i] = (char *)b->count;

            const Bs_Str *str = (const Bs_Str *)array->data[i].as.object;
            bs_da_push_many(bs, b, str->data, str->size);
            bs_da_push(bs, b, '\0');
        }

        // Resolve the pointer offsets
        for (size_t i = 0; i < array->count; i++) {
            cargv[i] = b->data + (size_t)cargv[i];
        }
        cargv[array->count] = NULL;

        execvp(*cargv, (char *const *)cargv);
        exit(127);
    }

    return bs_value_object(bs_c_data_new(bs, (void *)(uintptr_t)pid, &bs_process_data_spec));
}

// Str
static Bs_Value bs_str_slice(Bs *bs, Bs_Value *args, size_t arity) {
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

static Bs_Value bs_str_format(Bs *bs, Bs_Value *args, size_t arity) {
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

static Bs_Value bs_str_reverse(Bs *bs, Bs_Value *args, size_t arity) {
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
static Bs_Value bs_array_join(Bs *bs, Bs_Value *args, size_t arity) {
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

// Bytes
static void bs_bytes_data_free(Bs *bs, void *data) {
    bs_da_free(bs, (Bs_Buffer *)data);
    free(data);
}

static void bs_bytes_data_write(Bs_Writer *w, const void *data) {
    const Bs_Buffer *b = data;
    w->write(w, bs_sv_from_parts(b->data, b->count));
}

static const Bs_C_Data_Spec bs_bytes_data_spec = {
    .name = Bs_Sv_Static("bytes"),
    .free = bs_bytes_data_free,
    .write = bs_bytes_data_write,
};

static Bs_Value bs_bytes_new(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 0)) {
        return bs_value_error;
    }

    Bs_Buffer *buffer = bs_realloc(bs, NULL, 0, sizeof(Bs_Buffer));
    memset(buffer, 0, sizeof(*buffer));
    buffer->bs = bs;

    return bs_value_object(bs_c_data_new(bs, buffer, &bs_bytes_data_spec));
}

static Bs_Value bs_bytes_len(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 1)) {
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &bs_bytes_data_spec, "argument #1")) {
        return bs_value_error;
    }

    const Bs_Buffer *b = ((Bs_C_Data *)args[0].as.object)->data;
    return bs_value_num(b->count);
}

static Bs_Value bs_bytes_reset(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 2)) {
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &bs_bytes_data_spec, "argument #1")) {
        return bs_value_error;
    }

    if (!bs_check_whole_number(bs, args[1], "argument #2")) {
        return bs_value_error;
    }

    Bs_Buffer *b = ((Bs_C_Data *)args[0].as.object)->data;
    const size_t reset = args[1].as.number;

    if (reset > b->count) {
        bs_error(bs, "cannot reset bytes of length %zu to %zu", b->count, reset);
        return bs_value_error;
    }

    b->count = args[1].as.number;
    return bs_value_nil;
}

static Bs_Value bs_bytes_slice(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 3)) {
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &bs_bytes_data_spec, "argument #1")) {
        return bs_value_error;
    }

    if (!bs_check_whole_number(bs, args[1], "argument #2")) {
        return bs_value_error;
    }

    if (!bs_check_whole_number(bs, args[2], "argument #3")) {
        return bs_value_error;
    }

    const Bs_Buffer *b = ((Bs_C_Data *)args[0].as.object)->data;
    const size_t begin = bs_min(args[1].as.number, args[2].as.number);
    const size_t end = bs_max(args[1].as.number, args[2].as.number);

    if (begin >= b->count || end > b->count) {
        bs_error(bs, "cannot slice bytes of length %zu from %zu to %zu", b->count, begin, end);
        return bs_value_error;
    }

    return bs_value_object(bs_str_new(bs, bs_sv_from_parts(b->data + begin, end - begin)));
}

static Bs_Value bs_bytes_push(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 2)) {
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &bs_bytes_data_spec, "argument #1")) {
        return bs_value_error;
    }

    if (!bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2")) {
        return bs_value_error;
    }

    Bs_Buffer *b = ((Bs_C_Data *)args[0].as.object)->data;
    const Bs_Str *src = (const Bs_Str *)args[1].as.object;
    bs_da_push_many(bs, b, src->data, src->size);

    return bs_value_nil;
}

static Bs_Value bs_bytes_insert(Bs *bs, Bs_Value *args, size_t arity) {
    if (!bs_check_arity(bs, arity, 3)) {
        return bs_value_error;
    }

    if (!bs_check_object_c_type(bs, args[0], &bs_bytes_data_spec, "argument #1")) {
        return bs_value_error;
    }

    if (!bs_check_whole_number(bs, args[1], "argument #2")) {
        return bs_value_error;
    }

    if (!bs_check_object_type(bs, args[2], BS_OBJECT_STR, "argument #3")) {
        return bs_value_error;
    }

    Bs_Buffer *b = ((Bs_C_Data *)args[0].as.object)->data;
    const size_t index = args[1].as.number;
    const Bs_Str *src = (const Bs_Str *)args[2].as.object;

    if (index > b->count) {
        bs_error(bs, "cannot insert at %zu into bytes of length %zu", index, b->count);
        return bs_value_error;
    }

    bs_da_push_many(bs, b, NULL, src->size);
    memmove(b->data + index + src->size, b->data + index, b->count - index);
    memcpy(b->data + index, src->data, src->size);
    b->count += src->size;

    return bs_value_nil;
}

// Main
static void bs_table_add(Bs *bs, Bs_Table *table, const char *name, Bs_Value value) {
    bs_table_set(bs, table, bs_value_object(bs_str_const(bs, bs_sv_from_cstr(name))), value);
}

int bs_core_init(Bs *bs, int argc, char **argv) {
    {
        Bs_Table *io = bs_table_new(bs);
        bs_table_add(bs, io, "open", bs_value_object(bs_c_fn_new(bs, bs_io_open)));
        bs_table_add(bs, io, "close", bs_value_object(bs_c_fn_new(bs, bs_io_close)));
        bs_table_add(bs, io, "read", bs_value_object(bs_c_fn_new(bs, bs_io_read)));
        bs_table_add(bs, io, "flush", bs_value_object(bs_c_fn_new(bs, bs_io_flush)));

        bs_table_add(bs, io, "write", bs_value_object(bs_c_fn_new(bs, bs_io_write)));
        bs_table_add(bs, io, "print", bs_value_object(bs_c_fn_new(bs, bs_io_print)));
        bs_table_add(bs, io, "eprint", bs_value_object(bs_c_fn_new(bs, bs_io_eprint)));

        bs_table_add(bs, io, "writeln", bs_value_object(bs_c_fn_new(bs, bs_io_writeln)));
        bs_table_add(bs, io, "println", bs_value_object(bs_c_fn_new(bs, bs_io_println)));
        bs_table_add(bs, io, "eprintln", bs_value_object(bs_c_fn_new(bs, bs_io_eprintln)));

        bs_table_add(
            bs, io, "stdin", bs_value_object(bs_c_data_new(bs, stdin, &bs_file_data_spec)));
        bs_table_add(
            bs, io, "stdout", bs_value_object(bs_c_data_new(bs, stdout, &bs_file_data_spec)));
        bs_table_add(
            bs, io, "stderr", bs_value_object(bs_c_data_new(bs, stderr, &bs_file_data_spec)));

        bs_global_set(bs, Bs_Sv_Static("io"), bs_value_object(io));
    }

    {
        Bs_Table *os = bs_table_new(bs);
        bs_table_add(bs, os, "exit", bs_value_object(bs_c_fn_new(bs, bs_os_exit)));
        bs_table_add(bs, os, "getenv", bs_value_object(bs_c_fn_new(bs, bs_os_getenv)));
        bs_table_add(bs, os, "setenv", bs_value_object(bs_c_fn_new(bs, bs_os_setenv)));

        Bs_Array *args = bs_array_new(bs);
        for (int i = 0; i < argc; i++) {
            bs_array_set(bs, args, i, bs_value_object(bs_str_const(bs, bs_sv_from_cstr(argv[i]))));
        }
        bs_table_add(bs, os, "args", bs_value_object(args));

        bs_global_set(bs, Bs_Sv_Static("os"), bs_value_object(os));
    }

    {
        Bs_Table *process = bs_table_new(bs);
        bs_table_add(bs, process, "kill", bs_value_object(bs_c_fn_new(bs, bs_process_kill)));
        bs_table_add(bs, process, "wait", bs_value_object(bs_c_fn_new(bs, bs_process_wait)));
        bs_table_add(bs, process, "spawn", bs_value_object(bs_c_fn_new(bs, bs_process_spawn)));
        bs_global_set(bs, Bs_Sv_Static("process"), bs_value_object(process));
    }

    {
        Bs_Table *str = bs_table_new(bs);
        bs_table_add(bs, str, "slice", bs_value_object(bs_c_fn_new(bs, bs_str_slice)));
        bs_table_add(bs, str, "format", bs_value_object(bs_c_fn_new(bs, bs_str_format)));
        bs_table_add(bs, str, "reverse", bs_value_object(bs_c_fn_new(bs, bs_str_reverse)));
        bs_global_set(bs, Bs_Sv_Static("str"), bs_value_object(str));
    }

    {
        Bs_Table *array = bs_table_new(bs);
        bs_table_add(bs, array, "join", bs_value_object(bs_c_fn_new(bs, bs_array_join)));
        bs_global_set(bs, Bs_Sv_Static("array"), bs_value_object(array));
    }

    {
        Bs_Table *bytes = bs_table_new(bs);
        bs_table_add(bs, bytes, "new", bs_value_object(bs_c_fn_new(bs, bs_bytes_new)));
        bs_table_add(bs, bytes, "len", bs_value_object(bs_c_fn_new(bs, bs_bytes_len)));
        bs_table_add(bs, bytes, "reset", bs_value_object(bs_c_fn_new(bs, bs_bytes_reset)));
        bs_table_add(bs, bytes, "slice", bs_value_object(bs_c_fn_new(bs, bs_bytes_slice)));

        bs_table_add(bs, bytes, "push", bs_value_object(bs_c_fn_new(bs, bs_bytes_push)));
        bs_table_add(bs, bytes, "insert", bs_value_object(bs_c_fn_new(bs, bs_bytes_insert)));
        bs_global_set(bs, Bs_Sv_Static("bytes"), bs_value_object(bytes));
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
