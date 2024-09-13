#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

#include <regex.h>
#include <sys/wait.h>
#include <unistd.h>

#include "bs/core.h"
#include "bs/hash.h"

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
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_value_type(bs, args[1], BS_VALUE_BOOL, "argument #2");

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
    bs_check_arity(bs, arity, 1);
    bs_check_object_c_type(bs, args[0], &bs_file_data_spec, "argument #1");

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (!c->data) {
        bs_error(bs, "cannot close already closed file");
    }

    bs_file_data_free(bs, c->data);
    c->data = NULL;

    return bs_value_nil;
}

static Bs_Value bs_io_read(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_c_type(bs, args[0], &bs_file_data_spec, "argument #1");
    bs_check_whole_number(bs, args[1], "argument #2");

    FILE *f = ((Bs_C_Data *)args[0].as.object)->data;
    if (!f) {
        bs_error(bs, "cannot read from closed file");
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
    bs_check_arity(bs, arity, 1);
    bs_check_object_c_type(bs, args[0], &bs_file_data_spec, "argument #1");

    FILE *f = ((Bs_C_Data *)args[0].as.object)->data;
    if (!f) {
        bs_error(bs, "cannot flush closed file");
    }

    fflush(f);
    return bs_value_nil;
}

static Bs_Value bs_io_write(Bs *bs, Bs_Value *args, size_t arity) {
    if (!arity) {
        bs_error(bs, "expected at least 1 argument, got 0");
    }

    bs_check_object_c_type(bs, args[0], &bs_file_data_spec, "argument #1");

    FILE *f = ((Bs_C_Data *)args[0].as.object)->data;
    if (!f) {
        bs_error(bs, "cannot write into closed file");
    }

    Bs_Writer w = bs_file_writer(f);
    for (size_t i = 1; i < arity; i++) {
        if (i > 1) {
            bs_fmt(&w, " ");
        }
        bs_value_write(bs, &w, args[i]);
    }

    return bs_value_bool(!ferror(f));
}

static Bs_Value bs_io_print(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer *w = bs_stdout_get(bs);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(w, " ");
        }
        bs_value_write(bs, w, args[i]);
    }
    return bs_value_nil;
}

static Bs_Value bs_io_eprint(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer *w = bs_stderr_get(bs);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(w, " ");
        }
        bs_value_write(bs, w, args[i]);
    }
    return bs_value_nil;
}

static Bs_Value bs_io_writeln(Bs *bs, Bs_Value *args, size_t arity) {
    if (!arity) {
        bs_error(bs, "expected at least 1 argument, got 0");
    }

    bs_check_object_c_type(bs, args[0], &bs_file_data_spec, "argument #1");

    FILE *f = ((Bs_C_Data *)args[0].as.object)->data;
    if (!f) {
        bs_error(bs, "cannot write into closed file");
    }

    Bs_Writer w = bs_file_writer(f);
    for (size_t i = 1; i < arity; i++) {
        if (i > 1) {
            bs_fmt(&w, " ");
        }
        bs_value_write(bs, &w, args[i]);
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
        bs_value_write(bs, w, args[i]);
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
        bs_value_write(bs, w, args[i]);
    }
    bs_fmt(w, "\n");
    return bs_value_nil;
}

// OS
static Bs_Value bs_os_exit(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_whole_number(bs, args[0], "argument #1");

    bs_unwind(bs, args[0].as.number);
    assert(false && "unreachable");
}

static Bs_Value bs_os_clock(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    struct timespec clock;
    if (clock_gettime(CLOCK_MONOTONIC, &clock) < 0) {
        bs_error(bs, "could not get clock");
    }

    return bs_value_num(clock.tv_sec + clock.tv_nsec * 1e-9);
}

static Bs_Value bs_os_sleep(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");

    const double seconds = args[0].as.number;

    struct timespec req, rem;
    req.tv_sec = (time_t)seconds;
    req.tv_nsec = (long)((seconds - req.tv_sec) * 1e9);

    while (nanosleep(&req, &rem) == -1) {
        if (errno != EINTR) {
            bs_error(bs, "could not sleep");
        }

        req = rem;
    }

    return bs_value_nil;
}

static Bs_Value bs_os_getenv(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");

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
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");

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
    bs_check_arity(bs, arity, 2);
    bs_check_object_c_type(bs, args[0], &bs_process_data_spec, "argument #1");
    bs_check_whole_number(bs, args[1], "argument #2");

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (!c->data) {
        bs_error(bs, "cannot kill already terminated process");
    }

    const pid_t pid = (uintptr_t)c->data;
    c->data = NULL;

    if (kill(pid, args[1].as.number) < 0) {
        bs_error(bs, "could not kill process");
    }

    return bs_value_nil;
}

static Bs_Value bs_process_wait(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_object_c_type(bs, args[0], &bs_process_data_spec, "argument #1");

    Bs_C_Data *c = (Bs_C_Data *)args[0].as.object;
    if (!c->data) {
        bs_error(bs, "cannot wait for already terminated process");
    }

    const pid_t pid = (uintptr_t)c->data;
    c->data = NULL;

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        bs_error(bs, "could not wait for process");
    }

    return WIFEXITED(status) ? bs_value_num(WEXITSTATUS(status)) : bs_value_nil;
}

static Bs_Value bs_process_spawn(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_object_type(bs, args[0], BS_OBJECT_ARRAY, "argument #1");

    const Bs_Array *array = (const Bs_Array *)args[0].as.object;
    if (!array->count) {
        bs_error(bs, "cannot execute empty array as process");
    }

    for (size_t i = 0; i < array->count; i++) {
        bs_check_object_type(bs, array->data[i], BS_OBJECT_STR, "command argument");
    }

    const pid_t pid = fork();
    if (pid < 0) {
        bs_error(bs, "could not fork process");
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

// Bit
static Bs_Value bs_bit_ceil(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_whole_number(bs, args[0], "argument #1");

    size_t a = args[0].as.number;
    if (a == 0) {
        return bs_value_num(1);
    }

    a--;
    a |= a >> 1;
    a |= a >> 2;
    a |= a >> 4;
    a |= a >> 8;
    a |= a >> 16;
    a |= a >> 32;
    return bs_value_num(a + 1);
}

static Bs_Value bs_bit_floor(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_whole_number(bs, args[0], "argument #1");

    size_t a = args[0].as.number;
    if (a == 0) {
        return bs_value_num(0);
    }

    a |= a >> 1;
    a |= a >> 2;
    a |= a >> 4;
    a |= a >> 8;
    a |= a >> 16;
    a |= a >> 32;
    return bs_value_num(a - (a >> 1));
}

// Str
static Bs_Value bs_str_slice(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_whole_number(bs, args[1], "argument #2");
    bs_check_whole_number(bs, args[2], "argument #3");

    Bs_Str *str = (Bs_Str *)args[0].as.object;
    const size_t begin = bs_min(args[1].as.number, args[2].as.number);
    const size_t end = bs_max(args[1].as.number, args[2].as.number);

    if (begin >= str->size || end > str->size) {
        bs_error(bs, "cannot slice string of length %zu from %zu to %zu", str->size, begin, end);
    }

    return bs_value_object(bs_str_new(bs, Bs_Sv(str->data + begin, end - begin)));
}

static Bs_Value bs_str_reverse(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");

    const Bs_Str *src = (const Bs_Str *)args[0].as.object;
    Bs_Str *dst = bs_str_new(bs, Bs_Sv(src->data, src->size));
    for (size_t i = 0; i < dst->size / 2; i++) {
        const char c = dst->data[i];
        dst->data[i] = dst->data[dst->size - i - 1];
        dst->data[dst->size - i - 1] = c;
    }

    return bs_value_object(dst);
}

static Bs_Value bs_str_tolower(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");

    const Bs_Str *src = (const Bs_Str *)args[0].as.object;
    Bs_Str *dst = bs_str_new(bs, Bs_Sv(src->data, src->size));
    for (size_t i = 0; i < dst->size; i++) {
        dst->data[i] = tolower(dst->data[i]);
    }

    return bs_value_object(dst);
}

static Bs_Value bs_str_toupper(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");

    const Bs_Str *src = (const Bs_Str *)args[0].as.object;
    Bs_Str *dst = bs_str_new(bs, Bs_Sv(src->data, src->size));
    for (size_t i = 0; i < dst->size; i++) {
        dst->data[i] = toupper(dst->data[i]);
    }

    return bs_value_object(dst);
}

static Bs_Value bs_str_tonumber(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");

    const Bs_Str *src = (const Bs_Str *)args[0].as.object;

    Bs_Buffer *b = bs_buffer_get(bs);
    const size_t start = b->count;

    bs_da_push_many(b->bs, b, src->data, src->size);
    bs_da_push(b->bs, b, '\0');

    const char *input = bs_buffer_reset(b, start).data;
    char *end;

    const double value = strtod(input, &end);
    return (end == input || *end != '\0' || errno == ERANGE) ? bs_value_nil : bs_value_num(value);
}

static Bs_Value bs_str_find(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 2 && arity != 3) {
        bs_error(bs, "expected 2 or 3 arguments, got %zu", arity);
    }

    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");

    if (arity == 3) {
        bs_check_whole_number(bs, args[2], "argument #3");
    }

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;
    if (!pattern->size) {
        return bs_value_nil;
    }

    const size_t offset = arity == 3 ? args[2].as.number : 0;
    if (offset > str->size) {
        bs_error(bs, "cannot take offset of %zu in string of length %zu", offset, str->size);
    }

    if (str->size < pattern->size + offset) {
        return bs_value_nil;
    }

    const Bs_Sv pattern_sv = Bs_Sv(pattern->data, pattern->size);
    for (size_t i = offset; i + pattern->size <= str->size; i++) {
        if (bs_sv_eq(Bs_Sv(str->data + i, pattern->size), pattern_sv)) {
            return bs_value_num(i);
        }
    }

    return bs_value_nil;
}

static Bs_Value bs_str_split(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;

    Bs_Array *a = bs_array_new(bs);
    if (!pattern->size) {
        bs_array_set(bs, a, a->count, bs_value_object(str));
        return bs_value_object(a);
    }
    const Bs_Sv pattern_sv = Bs_Sv(pattern->data, pattern->size);

    size_t i = 0, j = 0;
    while (i + pattern->size <= str->size) {
        if (bs_sv_eq(Bs_Sv(str->data + i, pattern->size), pattern_sv)) {
            bs_array_set(
                bs, a, a->count, bs_value_object(bs_str_new(bs, Bs_Sv(str->data + j, i - j))));

            i += pattern->size;
            j = i;
        } else {
            i++;
        }
    }

    if (j != str->size) {
        bs_array_set(
            bs, a, a->count, bs_value_object(bs_str_new(bs, Bs_Sv(str->data + j, str->size - j))));
    }

    return bs_value_object(a);
}

static Bs_Value bs_str_replace(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");
    bs_check_object_type(bs, args[2], BS_OBJECT_STR, "argument #3");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;
    if (!pattern->size) {
        return bs_value_object(str);
    }

    const Bs_Str *replacement = (const Bs_Str *)args[2].as.object;

    if (str->size < pattern->size) {
        return bs_value_object(str);
    }

    Bs_Buffer *b = bs_buffer_get(bs);
    const size_t start = b->count;

    const Bs_Sv pattern_sv = Bs_Sv(pattern->data, pattern->size);
    for (size_t i = 0; i + pattern->size <= str->size; i++) {
        if (bs_sv_eq(Bs_Sv(str->data + i, pattern->size), pattern_sv)) {
            bs_da_push_many(bs, b, replacement->data, replacement->size);
            i += pattern->size - 1;
        } else {
            bs_da_push(bs, b, str->data[i]);
        }
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

static Bs_Value bs_str_trim(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;
    if (!str->size || !pattern->size) {
        return bs_value_object(str);
    }

    if (pattern->size > str->size) {
        return bs_value_object(str);
    }

    Bs_Sv str_sv = Bs_Sv(str->data, str->size);
    const Bs_Sv pattern_sv = Bs_Sv(pattern->data, pattern->size);

    while (bs_sv_prefix(str_sv, pattern_sv)) {
        str_sv.data += pattern_sv.size;
        str_sv.size -= pattern_sv.size;
    }

    while (bs_sv_suffix(str_sv, pattern_sv)) {
        str_sv.size -= pattern_sv.size;
    }

    return bs_value_object(bs_str_new(bs, str_sv));
}

static Bs_Value bs_str_ltrim(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;
    if (!str->size || !pattern->size) {
        return bs_value_object(str);
    }

    if (pattern->size > str->size) {
        return bs_value_object(str);
    }

    size_t i = 0;

    Bs_Sv str_sv = Bs_Sv(str->data, str->size);
    const Bs_Sv pattern_sv = Bs_Sv(pattern->data, pattern->size);
    while (bs_sv_prefix(str_sv, pattern_sv)) {
        str_sv.data += pattern_sv.size;
        str_sv.size -= pattern_sv.size;
    }

    return bs_value_object(bs_str_new(bs, str_sv));
}

static Bs_Value bs_str_rtrim(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;
    if (!str->size || !pattern->size) {
        return bs_value_object(str);
    }

    if (pattern->size > str->size) {
        return bs_value_object(str);
    }

    Bs_Sv str_sv = Bs_Sv(str->data, str->size);
    const Bs_Sv pattern_sv = Bs_Sv(pattern->data, pattern->size);
    while (bs_sv_suffix(str_sv, pattern_sv)) {
        str_sv.size -= pattern_sv.size;
    }

    return bs_value_object(bs_str_new(bs, str_sv));
}

static Bs_Value bs_str_lpad(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");
    bs_check_whole_number(bs, args[2], "argument #3");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;
    const size_t count = args[2].as.number;
    if (!str->size || !pattern->size) {
        return bs_value_object(str);
    }

    if (pattern->size > str->size) {
        return bs_value_object(str);
    }

    if (str->size >= count) {
        return bs_value_object(str);
    }
    const size_t padding = count - str->size;

    Bs_Str *result = bs_str_new(bs, Bs_Sv(NULL, count));
    memcpy(result->data + padding, str->data, str->size);

    for (size_t i = 0; i < padding; i += pattern->size) {
        memcpy(result->data + i, pattern->data, bs_min(pattern->size, padding - i));
    }

    return bs_value_object(result);
}

static Bs_Value bs_str_rpad(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");
    bs_check_whole_number(bs, args[2], "argument #3");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;
    const size_t count = args[2].as.number;
    if (!str->size || !pattern->size) {
        return bs_value_object(str);
    }

    if (pattern->size > str->size) {
        return bs_value_object(str);
    }

    if (str->size >= count) {
        return bs_value_object(str);
    }

    Bs_Str *result = bs_str_new(bs, Bs_Sv(NULL, count));
    memcpy(result->data, str->data, str->size);

    for (size_t i = str->size; i < count; i += pattern->size) {
        memcpy(result->data + i, pattern->data, bs_min(pattern->size, count - i));
    }

    return bs_value_object(result);
}

// Ascii
static Bs_Value bs_ascii_char(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_whole_number(bs, args[0], "argument #1");

    const size_t code = args[0].as.number;
    if (code > 0xff) {
        bs_error(bs, "invalid ascii code '%zu'", code);
    }

    const char ch = code;
    return bs_value_object(bs_str_new(bs, Bs_Sv(&ch, 1)));
}

static Bs_Value bs_ascii_code(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    if (str->size != 1) {
        bs_error(bs, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_num(*str->data);
}

// Bytes
static void bs_bytes_data_free(Bs *bs, void *data) {
    bs_da_free(bs, (Bs_Buffer *)data);
    free(data);
}

static void bs_bytes_data_write(Bs_Writer *w, const void *data) {
    const Bs_Buffer *b = data;
    w->write(w, Bs_Sv(b->data, b->count));
}

static const Bs_C_Data_Spec bs_bytes_data_spec = {
    .name = Bs_Sv_Static("bytes"),
    .free = bs_bytes_data_free,
    .write = bs_bytes_data_write,
};

static Bs_Value bs_bytes_new(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    Bs_Buffer *buffer = bs_realloc(bs, NULL, 0, sizeof(Bs_Buffer));
    memset(buffer, 0, sizeof(*buffer));
    buffer->bs = bs;

    return bs_value_object(bs_c_data_new(bs, buffer, &bs_bytes_data_spec));
}

static Bs_Value bs_bytes_len(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_object_c_type(bs, args[0], &bs_bytes_data_spec, "argument #1");

    const Bs_Buffer *b = ((Bs_C_Data *)args[0].as.object)->data;
    return bs_value_num(b->count);
}

static Bs_Value bs_bytes_reset(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_c_type(bs, args[0], &bs_bytes_data_spec, "argument #1");
    bs_check_whole_number(bs, args[1], "argument #2");

    Bs_Buffer *b = ((Bs_C_Data *)args[0].as.object)->data;
    const size_t reset = args[1].as.number;

    if (reset > b->count) {
        bs_error(bs, "cannot reset bytes of length %zu to %zu", b->count, reset);
    }

    b->count = args[1].as.number;
    return bs_value_nil;
}

static Bs_Value bs_bytes_slice(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_check_object_c_type(bs, args[0], &bs_bytes_data_spec, "argument #1");
    bs_check_whole_number(bs, args[1], "argument #2");
    bs_check_whole_number(bs, args[2], "argument #3");

    const Bs_Buffer *b = ((Bs_C_Data *)args[0].as.object)->data;
    const size_t begin = bs_min(args[1].as.number, args[2].as.number);
    const size_t end = bs_max(args[1].as.number, args[2].as.number);

    if (begin >= b->count || end > b->count) {
        bs_error(bs, "cannot slice bytes of length %zu from %zu to %zu", b->count, begin, end);
    }

    return bs_value_object(bs_str_new(bs, Bs_Sv(b->data + begin, end - begin)));
}

static Bs_Value bs_bytes_push(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_c_type(bs, args[0], &bs_bytes_data_spec, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");

    Bs_Buffer *b = ((Bs_C_Data *)args[0].as.object)->data;
    const Bs_Str *src = (const Bs_Str *)args[1].as.object;
    bs_da_push_many(bs, b, src->data, src->size);

    return bs_value_nil;
}

static Bs_Value bs_bytes_insert(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_check_object_c_type(bs, args[0], &bs_bytes_data_spec, "argument #1");
    bs_check_whole_number(bs, args[1], "argument #2");
    bs_check_object_type(bs, args[2], BS_OBJECT_STR, "argument #3");

    Bs_Buffer *b = ((Bs_C_Data *)args[0].as.object)->data;
    const size_t index = args[1].as.number;
    const Bs_Str *src = (const Bs_Str *)args[2].as.object;

    if (index > b->count) {
        bs_error(bs, "cannot insert at %zu into bytes of length %zu", index, b->count);
    }

    bs_da_push_many(bs, b, NULL, src->size);
    memmove(b->data + index + src->size, b->data + index, b->count - index);
    memcpy(b->data + index, src->data, src->size);
    b->count += src->size;

    return bs_value_nil;
}

// Regex
static Bs_Value bs_regex_find(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 2 && arity != 3) {
        bs_error(bs, "expected 2 or 3 arguments, got %zu", arity);
    }

    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");

    if (arity == 3) {
        bs_check_whole_number(bs, args[2], "argument #3");
    }

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;
    if (!pattern->size) {
        return bs_value_nil;
    }

    const size_t offset = arity == 3 ? args[2].as.number : 0;
    if (offset > str->size) {
        bs_error(bs, "cannot take offset of %zu in string of length %zu", offset, str->size);
    }

    Bs_Buffer *b = bs_buffer_get(bs);
    const size_t start = b->count;

    const size_t str_pos = b->count;
    bs_da_push_many(bs, b, str->data + offset, str->size - offset);
    bs_da_push(bs, b, '\0');

    const size_t pattern_pos = b->count;
    bs_da_push_many(bs, b, pattern->data, pattern->size);
    bs_da_push(bs, b, '\0');

    regex_t regex;
    if (regcomp(&regex, b->data + pattern_pos, REG_EXTENDED)) {
        bs_error(bs, "could not compile regex pattern");
    }

    regmatch_t match;
    Bs_Value result = bs_value_nil;
    if (!regexec(&regex, b->data + str_pos, 1, &match, 0)) {
        result = bs_value_num(offset + match.rm_so);
    }
    regfree(&regex);

    bs_buffer_reset(b, start);
    return result;
}

static Bs_Value bs_regex_split(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;

    Bs_Array *a = bs_array_new(bs);
    if (!pattern->size) {
        bs_array_set(bs, a, a->count, bs_value_object(str));
        return bs_value_object(a);
    }

    Bs_Buffer *b = bs_buffer_get(bs);
    const size_t start = b->count;

    const size_t str_pos = b->count;
    bs_da_push_many(bs, b, str->data, str->size);
    bs_da_push(bs, b, '\0');

    const size_t pattern_pos = b->count;
    bs_da_push_many(bs, b, pattern->data, pattern->size);
    bs_da_push(bs, b, '\0');

    regex_t regex;
    if (regcomp(&regex, b->data + pattern_pos, REG_EXTENDED)) {
        bs_error(bs, "could not compile regex pattern");
    }

    regmatch_t match;
    size_t j = 0;
    while (!regexec(&regex, b->data + str_pos + j, 1, &match, 0)) {
        if (match.rm_so == match.rm_eo) {
            break;
        }

        bs_array_set(
            bs,
            a,
            a->count,
            bs_value_object(bs_str_new(bs, Bs_Sv(b->data + str_pos + j, match.rm_so))));

        j += match.rm_eo;
    }

    if (j != str->size) {
        bs_array_set(
            bs, a, a->count, bs_value_object(bs_str_new(bs, Bs_Sv(str->data + j, str->size - j))));
    }

    regfree(&regex);
    bs_buffer_reset(b, start);
    return bs_value_object(a);
}

static Bs_Value bs_regex_replace(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");
    bs_check_object_type(bs, args[2], BS_OBJECT_STR, "argument #3");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;
    if (!pattern->size) {
        return bs_value_object(str);
    }

    const Bs_Str *replacement = (const Bs_Str *)args[2].as.object;

    Bs_Buffer *b = bs_buffer_get(bs);
    const size_t start = b->count;

    const size_t str_pos = b->count;
    bs_da_push_many(bs, b, str->data, str->size);
    bs_da_push(bs, b, '\0');

    const size_t pattern_pos = b->count;
    bs_da_push_many(bs, b, pattern->data, pattern->size);
    bs_da_push(bs, b, '\0');

    regex_t regex;
    if (regcomp(&regex, b->data + pattern_pos, REG_EXTENDED)) {
        bs_error(bs, "could not compile regex pattern");
    }

    Bs_Buffer *o = bs_paths_get(bs);
    const size_t result_pos = o->count;

    const char *cursor = b->data + str_pos;
    regmatch_t matches[10];
    while (!regexec(&regex, cursor, bs_c_array_size(matches), matches, 0)) {
        if (matches[0].rm_so == matches[0].rm_eo) {
            break;
        }

        bs_da_push_many(bs, o, cursor, matches[0].rm_so);

        for (size_t i = 0; i < replacement->size; i++) {
            if (replacement->data[i] == '\\' && isdigit(replacement->data[i + 1])) {
                const size_t match_index = replacement->data[++i] - '0';
                if (match_index < bs_c_array_size(matches) && matches[match_index].rm_so != -1) {
                    bs_da_push_many(
                        bs,
                        o,
                        cursor + matches[match_index].rm_so,
                        matches[match_index].rm_eo - matches[match_index].rm_so);
                }
            } else {
                bs_da_push(bs, o, replacement->data[i]);
            }
        }

        cursor += matches[0].rm_eo;
    }
    bs_da_push_many(bs, o, cursor, strlen(cursor));

    regfree(&regex);
    bs_buffer_reset(b, start);
    return bs_value_object(bs_str_new(bs, bs_buffer_reset(o, result_pos)));
}

// Array
static Bs_Value bs_array_map(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_ARRAY, "argument #1");
    bs_check_callable(bs, args[1], "argument #2");

    const Bs_Array *src = (const Bs_Array *)args[0].as.object;
    const Bs_Value fn = args[1];
    Bs_Array *dst = bs_array_new(bs);

    for (size_t i = 0; i < src->count; i++) {
        const Bs_Value input = src->data[i];
        const Bs_Value output = bs_call(bs, fn, &input, 1);
        bs_array_set(bs, dst, i, output);
    }

    return bs_value_object(dst);
}

static Bs_Value bs_array_filter(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_ARRAY, "argument #1");
    bs_check_callable(bs, args[1], "argument #2");

    const Bs_Array *src = (const Bs_Array *)args[0].as.object;
    const Bs_Value fn = args[1];
    Bs_Array *dst = bs_array_new(bs);

    for (size_t i = 0; i < src->count; i++) {
        const Bs_Value input = src->data[i];
        const Bs_Value output = bs_call(bs, fn, &input, 1);

        if (!bs_value_is_falsey(output)) {
            bs_array_set(bs, dst, dst->count, input);
        }
    }

    return bs_value_object(dst);
}

static Bs_Value bs_array_reduce(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 2 && arity != 3) {
        bs_error(bs, "expected 2 or 3 arguments, got %zu", arity);
    }

    bs_check_object_type(bs, args[0], BS_OBJECT_ARRAY, "argument #1");
    bs_check_callable(bs, args[1], "argument #2");

    const Bs_Array *src = (const Bs_Array *)args[0].as.object;
    const Bs_Value fn = args[1];
    Bs_Value acc = arity == 3 ? args[2] : bs_value_nil;

    for (size_t i = 0; i < src->count; i++) {
        if (acc.type == BS_VALUE_NIL) {
            acc = src->data[i];
            continue;
        }

        const Bs_Value input[] = {acc, src->data[i]};
        acc = bs_call(bs, fn, input, bs_c_array_size(input));
    }

    return acc;
}

static Bs_Value bs_array_copy(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_object_type(bs, args[0], BS_OBJECT_ARRAY, "argument #1");

    const Bs_Array *src = (const Bs_Array *)args[0].as.object;

    Bs_Array *dst = bs_array_new(bs);
    for (size_t i = src->count; i > 0; i--) {
        bs_array_set(bs, dst, i - 1, src->data[i - 1]);
    }

    return bs_value_object(dst);
}

static Bs_Value bs_array_join(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_ARRAY, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");

    Bs_Buffer *b = bs_buffer_get(bs);
    const size_t start = b->count;
    {
        Bs_Writer w = bs_buffer_writer(b);

        const Bs_Array *array = (const Bs_Array *)args[0].as.object;
        const Bs_Str *str = (const Bs_Str *)args[1].as.object;
        const Bs_Sv separator = Bs_Sv(str->data, str->size);

        for (size_t i = 0; i < array->count; i++) {
            if (i) {
                w.write(&w, separator);
            }
            bs_value_write(bs, &w, array->data[i]);
        }
    }
    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

static Bs_Value bs_array_find(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 2 && arity != 3) {
        bs_error(bs, "expected 2 or 3 arguments, got %zu", arity);
    }

    bs_check_object_type(bs, args[0], BS_OBJECT_ARRAY, "argument #1");

    if (arity == 3) {
        bs_check_whole_number(bs, args[2], "argument #3");
    }

    const Bs_Array *array = (const Bs_Array *)args[0].as.object;
    const Bs_Value pred = args[1];
    const size_t offset = arity == 3 ? args[2].as.number : 0;

    for (size_t i = offset; i < array->count; i++) {
        if (bs_value_equal(array->data[i], pred)) {
            return bs_value_num(i);
        }
    }

    return bs_value_nil;
}

static Bs_Value bs_array_equal(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_ARRAY, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_ARRAY, "argument #2");

    const Bs_Array *a = (const Bs_Array *)args[0].as.object;
    const Bs_Array *b = (const Bs_Array *)args[1].as.object;

    if (a->count != b->count) {
        return bs_value_bool(false);
    }

    for (size_t i = 0; i < a->count; i++) {
        if (!bs_value_equal(a->data[i], b->data[i])) {
            return bs_value_bool(false);
        }
    }

    return bs_value_bool(true);
}

typedef struct {
    Bs *bs;
    Bs_Value fn;
} Bs_Array_Sort_Context;

static int bs_array_sort_compare(const void *a, const void *b) {
    static Bs_Array_Sort_Context context;
    if (!a) {
        context = *(const Bs_Array_Sort_Context *)b;
        return 0;
    }

    const Bs_Value args[] = {
        *(const Bs_Value *)a,
        *(const Bs_Value *)b,
    };

    const Bs_Value output = bs_call(context.bs, context.fn, args, bs_c_array_size(args));
    return 1 - !bs_value_is_falsey(output);
}

static Bs_Value bs_array_sort(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_ARRAY, "argument #1");
    bs_check_callable(bs, args[1], "argument #2");

    const Bs_Array *src = (const Bs_Array *)args[0].as.object;
    const Bs_Value fn = args[1];

    // Prepare the context
    const Bs_Array_Sort_Context context = {
        .bs = bs,
        .fn = fn,
    };
    bs_array_sort_compare(NULL, &context);

    qsort(src->data, src->count, sizeof(*src->data), bs_array_sort_compare);
    return bs_value_object(src);
}

static Bs_Value bs_array_reverse(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_object_type(bs, args[0], BS_OBJECT_ARRAY, "argument #1");

    const Bs_Array *src = (const Bs_Array *)args[0].as.object;
    for (size_t i = 0; i < src->count / 2; i++) {
        const Bs_Value t = src->data[i];
        src->data[i] = src->data[src->count - i - 1];
        src->data[src->count - i - 1] = t;
    }
    return bs_value_object(src);
}

// Table
static Bs_Value bs_table_copy(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_object_type(bs, args[0], BS_OBJECT_TABLE, "argument #1");

    const Bs_Table *src = (const Bs_Table *)args[0].as.object;

    Bs_Table *dst = bs_table_new(bs);
    for (size_t i = 0; i < src->capacity; i++) {
        const Bs_Entry *entry = &src->data[i];
        if (entry->key.type != BS_VALUE_NIL) {
            bs_table_set(bs, dst, entry->key, entry->value);
        }
    }

    return bs_value_object(dst);
}

static Bs_Value bs_table_equal(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_TABLE, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_TABLE, "argument #2");

    const Bs_Table *a = (const Bs_Table *)args[0].as.object;
    const Bs_Table *b = (const Bs_Table *)args[1].as.object;

    if (a->length != b->length) {
        return bs_value_bool(false);
    }

    for (size_t i = 0; i < a->capacity; i++) {
        if (a->data[i].key.type != BS_VALUE_NIL) {
            if (bs_entries_find(b->data, b->capacity, a->data[i].key)->key.type == BS_VALUE_NIL) {
                return bs_value_bool(false);
            }
        }
    }

    return bs_value_bool(true);
}

// Math
static Bs_Value bs_math_sin(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    return bs_value_num(sin(args[0].as.number));
}

static Bs_Value bs_math_cos(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    return bs_value_num(cos(args[0].as.number));
}

static Bs_Value bs_math_tan(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    return bs_value_num(tan(args[0].as.number));
}

static Bs_Value bs_math_asin(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    return bs_value_num(asin(args[0].as.number));
}

static Bs_Value bs_math_acos(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    return bs_value_num(acos(args[0].as.number));
}

static Bs_Value bs_math_atan(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    return bs_value_num(atan(args[0].as.number));
}

static Bs_Value bs_math_sqrt(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    return bs_value_num(sqrt(args[0].as.number));
}

static Bs_Value bs_math_ceil(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    return bs_value_num(ceil(args[0].as.number));
}

static Bs_Value bs_math_floor(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    return bs_value_num(floor(args[0].as.number));
}

static Bs_Value bs_math_round(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    return bs_value_num(round(args[0].as.number));
}

static Bs_Value bs_math_random(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num((double)rand() / RAND_MAX);
}

static Bs_Value bs_math_max(Bs *bs, Bs_Value *args, size_t arity) {
    if (!arity) {
        bs_error(bs, "expected at least 1 argument, got 0");
    }

    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    double max = args[0].as.number;
    for (size_t i = 1; i < arity; i++) {
        char label[32];
        const int count = snprintf(label, sizeof(label), "argument #%zu", i + 1);
        assert(count >= 0 && count + 1 < sizeof(label));

        bs_check_value_type(bs, args[i], BS_VALUE_NUM, label);
        max = bs_max(max, args[i].as.number);
    }
    return bs_value_num(max);
}

static Bs_Value bs_math_min(Bs *bs, Bs_Value *args, size_t arity) {
    if (!arity) {
        bs_error(bs, "expected at least 1 argument, got 0");
    }

    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    double min = args[0].as.number;
    for (size_t i = 1; i < arity; i++) {
        char label[32];
        const int count = snprintf(label, sizeof(label), "argument #%zu", i + 1);
        assert(count >= 0 && count + 1 < sizeof(label));

        bs_check_value_type(bs, args[i], BS_VALUE_NUM, label);
        min = bs_min(min, args[i].as.number);
    }
    return bs_value_num(min);
}

static Bs_Value bs_math_clamp(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    bs_check_value_type(bs, args[1], BS_VALUE_NUM, "argument #2");
    bs_check_value_type(bs, args[2], BS_VALUE_NUM, "argument #3");

    const double low = bs_min(args[1].as.number, args[2].as.number);
    const double high = bs_max(args[1].as.number, args[2].as.number);
    return bs_value_num(bs_min(bs_max(args[0].as.number, low), high));
}

static Bs_Value bs_math_lerp(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_check_value_type(bs, args[0], BS_VALUE_NUM, "argument #1");
    bs_check_value_type(bs, args[1], BS_VALUE_NUM, "argument #2");
    bs_check_value_type(bs, args[2], BS_VALUE_NUM, "argument #3");

    const double a = args[0].as.number;
    const double b = args[1].as.number;
    const double t = args[2].as.number;
    return bs_value_num(a + (b - a) * t);
}

// Main
static void bs_add(Bs *bs, Bs_Table *table, const char *key, Bs_Value value) {
    bs_table_set(bs, table, bs_value_object(bs_str_const(bs, bs_sv_from_cstr(key))), value);
}

static void bs_add_fn(Bs *bs, Bs_Table *table, const char *key, const char *name, Bs_C_Fn_Ptr fn) {
    bs_table_set(
        bs,
        table,
        bs_value_object(bs_str_const(bs, bs_sv_from_cstr(key))),
        bs_value_object(bs_c_fn_new(bs, name, fn)));
}

void bs_core_init(Bs *bs, int argc, char **argv) {
    srand(time(NULL));

    {
        Bs_Table *io = bs_table_new(bs);
        bs_add_fn(bs, io, "open", "io.open", bs_io_open);
        bs_add_fn(bs, io, "close", "io.close", bs_io_close);
        bs_add_fn(bs, io, "read", "io.read", bs_io_read);
        bs_add_fn(bs, io, "flush", "io.flush", bs_io_flush);

        bs_add_fn(bs, io, "write", "io.write", bs_io_write);
        bs_add_fn(bs, io, "print", "io.print", bs_io_print);
        bs_add_fn(bs, io, "eprint", "io.eprint", bs_io_eprint);

        bs_add_fn(bs, io, "writeln", "io.writeln", bs_io_writeln);
        bs_add_fn(bs, io, "println", "io.println", bs_io_println);
        bs_add_fn(bs, io, "eprintln", "io.eprintln", bs_io_eprintln);

        bs_add(bs, io, "stdin", bs_value_object(bs_c_data_new(bs, stdin, &bs_file_data_spec)));
        bs_add(bs, io, "stdout", bs_value_object(bs_c_data_new(bs, stdout, &bs_file_data_spec)));
        bs_add(bs, io, "stderr", bs_value_object(bs_c_data_new(bs, stderr, &bs_file_data_spec)));

        bs_global_set(bs, Bs_Sv_Static("io"), bs_value_object(io));
    }

    {
        Bs_Table *os = bs_table_new(bs);
        bs_add_fn(bs, os, "exit", "os.exit", bs_os_exit);
        bs_add_fn(bs, os, "clock", "os.clock", bs_os_clock);
        bs_add_fn(bs, os, "sleep", "os.sleep", bs_os_sleep);

        bs_add_fn(bs, os, "getenv", "os.getenv", bs_os_getenv);
        bs_add_fn(bs, os, "setenv", "os.setenv", bs_os_setenv);

        Bs_Array *args = bs_array_new(bs);
        for (int i = 0; i < argc; i++) {
            bs_array_set(bs, args, i, bs_value_object(bs_str_const(bs, bs_sv_from_cstr(argv[i]))));
        }
        bs_add(bs, os, "args", bs_value_object(args));

        bs_global_set(bs, Bs_Sv_Static("os"), bs_value_object(os));
    }

    {
        Bs_Table *process = bs_table_new(bs);
        bs_add_fn(bs, process, "kill", "process.kill", bs_process_kill);
        bs_add_fn(bs, process, "wait", "process.wait", bs_process_wait);
        bs_add_fn(bs, process, "spawn", "process.spawn", bs_process_spawn);
        bs_global_set(bs, Bs_Sv_Static("process"), bs_value_object(process));
    }

    {
        Bs_Table *bit = bs_table_new(bs);
        bs_add_fn(bs, bit, "ceil", "bit.ceil", bs_bit_ceil);
        bs_add_fn(bs, bit, "floor", "bit.floor", bs_bit_floor);
        bs_global_set(bs, Bs_Sv_Static("bit"), bs_value_object(bit));
    }

    {
        Bs_Table *str = bs_table_new(bs);
        bs_add_fn(bs, str, "slice", "str.slice", bs_str_slice);
        bs_add_fn(bs, str, "reverse", "str.reverse", bs_str_reverse);

        bs_add_fn(bs, str, "tolower", "str.tolower", bs_str_tolower);
        bs_add_fn(bs, str, "toupper", "str.toupper", bs_str_toupper);
        bs_add_fn(bs, str, "tonumber", "str.tonumber", bs_str_tonumber);

        bs_add_fn(bs, str, "find", "str.find", bs_str_find);
        bs_add_fn(bs, str, "split", "str.split", bs_str_split);
        bs_add_fn(bs, str, "replace", "str.replace", bs_str_replace);

        bs_add_fn(bs, str, "trim", "str.trim", bs_str_trim);
        bs_add_fn(bs, str, "ltrim", "str.ltrim", bs_str_ltrim);
        bs_add_fn(bs, str, "rtrim", "str.rtrim", bs_str_rtrim);

        bs_add_fn(bs, str, "lpad", "str.lpad", bs_str_lpad);
        bs_add_fn(bs, str, "rpad", "str.rpad", bs_str_rpad);

        bs_global_set(bs, Bs_Sv_Static("str"), bs_value_object(str));
    }

    {
        Bs_Table *regex = bs_table_new(bs);
        bs_add_fn(bs, regex, "find", "regex.find", bs_regex_find);
        bs_add_fn(bs, regex, "split", "regex.split", bs_regex_split);
        bs_add_fn(bs, regex, "replace", "regex.replace", bs_regex_replace);
        bs_global_set(bs, Bs_Sv_Static("regex"), bs_value_object(regex));
    }

    {
        Bs_Table *ascii = bs_table_new(bs);
        bs_add_fn(bs, ascii, "char", "ascii.char", bs_ascii_char);
        bs_add_fn(bs, ascii, "code", "ascii.code", bs_ascii_code);
        bs_global_set(bs, Bs_Sv_Static("ascii"), bs_value_object(ascii));
    }

    {
        Bs_Table *bytes = bs_table_new(bs);
        bs_add_fn(bs, bytes, "new", "bytes.new", bs_bytes_new);
        bs_add_fn(bs, bytes, "len", "bytes.len", bs_bytes_len);
        bs_add_fn(bs, bytes, "reset", "bytes.reset", bs_bytes_reset);
        bs_add_fn(bs, bytes, "slice", "bytes.slice", bs_bytes_slice);

        bs_add_fn(bs, bytes, "push", "bytes.push", bs_bytes_push);
        bs_add_fn(bs, bytes, "insert", "bytes.insert", bs_bytes_insert);
        bs_global_set(bs, Bs_Sv_Static("bytes"), bs_value_object(bytes));
    }

    {
        Bs_Table *array = bs_table_new(bs);
        bs_add_fn(bs, array, "map", "array.map", bs_array_map);
        bs_add_fn(bs, array, "filter", "array.filter", bs_array_filter);
        bs_add_fn(bs, array, "reduce", "array.reduce", bs_array_reduce);

        bs_add_fn(bs, array, "copy", "array.copy", bs_array_copy);
        bs_add_fn(bs, array, "join", "array.join", bs_array_join);
        bs_add_fn(bs, array, "find", "array.find", bs_array_find);
        bs_add_fn(bs, array, "equal", "array.equal", bs_array_equal);

        bs_add_fn(bs, array, "sort", "array.sort", bs_array_sort);
        bs_add_fn(bs, array, "reverse", "array.reverse", bs_array_reverse);
        bs_global_set(bs, Bs_Sv_Static("array"), bs_value_object(array));
    }

    {
        Bs_Table *table = bs_table_new(bs);
        bs_add_fn(bs, table, "copy", "table.copy", bs_table_copy);
        bs_add_fn(bs, table, "equal", "table.equal", bs_table_equal);
        bs_global_set(bs, Bs_Sv_Static("table"), bs_value_object(table));
    }

    {
        Bs_Table *math = bs_table_new(bs);
        bs_add(bs, math, "PI", bs_value_num(M_PI));
        bs_add_fn(bs, math, "sin", "math.sin", bs_math_sin);
        bs_add_fn(bs, math, "cos", "math.cos", bs_math_cos);
        bs_add_fn(bs, math, "tan", "math.tan", bs_math_tan);
        bs_add_fn(bs, math, "asin", "math.asin", bs_math_asin);
        bs_add_fn(bs, math, "acos", "math.acos", bs_math_acos);
        bs_add_fn(bs, math, "atan", "math.atan", bs_math_atan);

        bs_add_fn(bs, math, "sqrt", "math.sqrt", bs_math_sqrt);
        bs_add_fn(bs, math, "ceil", "math.ceil", bs_math_ceil);
        bs_add_fn(bs, math, "floor", "math.floor", bs_math_floor);
        bs_add_fn(bs, math, "round", "math.round", bs_math_round);
        bs_add_fn(bs, math, "random", "math.random", bs_math_random);

        bs_add_fn(bs, math, "max", "math.max", bs_math_max);
        bs_add_fn(bs, math, "min", "math.min", bs_math_min);
        bs_add_fn(bs, math, "clamp", "math.clamp", bs_math_clamp);
        bs_add_fn(bs, math, "lerp", "math.lerp", bs_math_lerp);
        bs_global_set(bs, Bs_Sv_Static("math"), bs_value_object(math));
    }
}
