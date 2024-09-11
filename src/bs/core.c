#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <regex.h>
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

static Bs_Value bs_str_find_regex(Bs *bs, Bs_Value *args, size_t arity) {
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

static Bs_Value bs_str_split(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;

    Bs_Array *a = bs_array_new(bs);
    const Bs_Sv pattern_sv = Bs_Sv(pattern->data, pattern->size);

    size_t i = 0, j = 0;
    while (i + pattern->size <= str->size) {
        if (bs_sv_eq(Bs_Sv(str->data + i, pattern->size), pattern_sv)) {
            bs_array_set(
                bs,
                a,
                a->count,
                bs_value_object(bs_str_new(bs, Bs_Sv(str->data + j, i - j))));

            i += pattern->size;
            j = i;
        } else {
            i++;
        }
    }

    if (j != str->size) {
        bs_array_set(
            bs,
            a,
            a->count,
            bs_value_object(bs_str_new(bs, Bs_Sv(str->data + j, str->size - j))));
    }

    return bs_value_object(a);
}

static Bs_Value bs_str_split_regex(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;

    Bs_Buffer *b = bs_buffer_get(bs);
    const size_t start = b->count;

    const size_t str_pos = b->count;
    bs_da_push_many(bs, b, str->data, str->size);
    bs_da_push(bs, b, '\0');

    const size_t pattern_pos = b->count;
    bs_da_push_many(bs, b, pattern->data, pattern->size);
    bs_da_push(bs, b, '\0');

    Bs_Array *a = bs_array_new(bs);

    regex_t regex;
    if (regcomp(&regex, b->data + pattern_pos, REG_EXTENDED)) {
        bs_error(bs, "could not compile regex pattern");
    }

    regmatch_t match;
    size_t j = 0;
    while (!regexec(&regex, b->data + str_pos + j, 1, &match, 0)) {
        bs_array_set(
            bs,
            a,
            a->count,
            bs_value_object(bs_str_new(bs, Bs_Sv(b->data + str_pos + j, match.rm_so))));

        j += match.rm_eo;
    }

    if (j != str->size) {
        bs_array_set(
            bs,
            a,
            a->count,
            bs_value_object(bs_str_new(bs, Bs_Sv(str->data + j, str->size - j))));
    }

    regfree(&regex);
    bs_buffer_reset(b, start);
    return bs_value_object(a);
}

static Bs_Value bs_str_replace(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");
    bs_check_object_type(bs, args[2], BS_OBJECT_STR, "argument #3");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;
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

static Bs_Value bs_str_replace_regex(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_check_object_type(bs, args[0], BS_OBJECT_STR, "argument #1");
    bs_check_object_type(bs, args[1], BS_OBJECT_STR, "argument #2");
    bs_check_object_type(bs, args[2], BS_OBJECT_STR, "argument #3");

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[1].as.object;
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

    Bs_Array *dst = bs_array_new(bs);
    for (size_t i = src->count; i > 0; i--) {
        bs_array_set(bs, dst, i - 1, src->data[i - 1]);
    }

    // Prepare the context
    const Bs_Array_Sort_Context context = {
        .bs = bs,
        .fn = fn,
    };
    bs_array_sort_compare(NULL, &context);

    qsort(dst->data, dst->count, sizeof(*dst->data), bs_array_sort_compare);
    return bs_value_object(dst);
}

static Bs_Value bs_array_reverse(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_check_object_type(bs, args[0], BS_OBJECT_ARRAY, "argument #1");

    const Bs_Array *src = (const Bs_Array *)args[0].as.object;
    Bs_Array *dst = bs_array_new(bs);
    for (size_t i = 0; i < src->count; i++) {
        bs_array_set(bs, dst, src->count - i - 1, src->data[i]);
    }

    return bs_value_object(dst);
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

// Main
static void bs_table_add(Bs *bs, Bs_Table *table, const char *name, Bs_Value value) {
    bs_table_set(bs, table, bs_value_object(bs_str_const(bs, bs_sv_from_cstr(name))), value);
}

void bs_core_init(Bs *bs, int argc, char **argv) {
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
        bs_table_add(bs, os, "sleep", bs_value_object(bs_c_fn_new(bs, bs_os_sleep)));
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
        bs_table_add(bs, str, "reverse", bs_value_object(bs_c_fn_new(bs, bs_str_reverse)));

        bs_table_add(bs, str, "find", bs_value_object(bs_c_fn_new(bs, bs_str_find)));
        bs_table_add(bs, str, "find_regex", bs_value_object(bs_c_fn_new(bs, bs_str_find_regex)));

        bs_table_add(bs, str, "split", bs_value_object(bs_c_fn_new(bs, bs_str_split)));
        bs_table_add(bs, str, "split_regex", bs_value_object(bs_c_fn_new(bs, bs_str_split_regex)));

        bs_table_add(bs, str, "replace", bs_value_object(bs_c_fn_new(bs, bs_str_replace)));
        bs_table_add(
            bs, str, "replace_regex", bs_value_object(bs_c_fn_new(bs, bs_str_replace_regex)));

        bs_global_set(bs, Bs_Sv_Static("str"), bs_value_object(str));
    }

    {
        Bs_Table *array = bs_table_new(bs);
        bs_table_add(bs, array, "map", bs_value_object(bs_c_fn_new(bs, bs_array_map)));
        bs_table_add(bs, array, "filter", bs_value_object(bs_c_fn_new(bs, bs_array_filter)));
        bs_table_add(bs, array, "reduce", bs_value_object(bs_c_fn_new(bs, bs_array_reduce)));

        bs_table_add(bs, array, "join", bs_value_object(bs_c_fn_new(bs, bs_array_join)));
        bs_table_add(bs, array, "sort", bs_value_object(bs_c_fn_new(bs, bs_array_sort)));
        bs_table_add(bs, array, "reverse", bs_value_object(bs_c_fn_new(bs, bs_array_reverse)));
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
}
