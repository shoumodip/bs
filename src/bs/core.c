#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

#include <fcntl.h>
#include <signal.h>

#ifdef _WIN32
#else
#    include <sys/wait.h>
#endif // _WIN32

#include <unistd.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "bs/core.h"
#include "bs/object.h"

// IO
void bs_io_file_free(void *userdata, void *instance_data) {
    FILE *f = bs_static_cast(instance_data, FILE *);
    if (f && fileno(f) > 2) {
        fclose(f);
    }
}

Bs_Value bs_io_file_close(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    Bs_C_Instance *this = (Bs_C_Instance *)args[-1].as.object;
    if (!bs_static_cast(this->data, FILE *)) {
        bs_error(bs, "cannot close already closed file");
    }

    bs_io_file_free(bs_config(bs)->userdata, this->data);
    bs_static_cast(this->data, FILE *) = NULL;
    return bs_value_nil;
}

// Reader
Bs_Value bs_io_reader_init(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    Bs_Buffer *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    Bs_Writer w = bs_buffer_writer(b);
    bs_fmt(&w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[0].as.object));
    bs_da_push(b->bs, b, '\0');

    FILE *file = fopen(bs_buffer_reset(b, start).data, "r");
    if (!file) {
        return bs_value_nil;
    }

    bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, FILE *) = file;
    return args[-1];
}

Bs_Value bs_io_reader_read(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity > 1) {
        bs_error(bs, "expected 0 or 1 arguments, got %zu", arity);
    }

    if (arity) {
        bs_arg_check_whole_number(bs, args, 0);
    }

    FILE *f = bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, FILE *);
    if (!f) {
        bs_error(bs, "cannot read from closed file");
    }

    size_t count = 0;
    if (arity == 1) {
        count = args[0].as.number;
    } else {
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

Bs_Value bs_io_reader_readln(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    FILE *f = bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, FILE *);
    if (!f) {
        bs_error(bs, "cannot read from closed file");
    }

    Bs_Buffer *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    while (true) {
        const char c = fgetc(f);
        if (c == '\n' || c == EOF) {
            break;
        }
        bs_da_push(bs, b, c);
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

Bs_Value bs_io_reader_seek(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_whole_number(bs, args, 0);
    bs_arg_check_whole_number(bs, args, 1);

    FILE *f = bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, FILE *);
    if (!f) {
        bs_error(bs, "cannot seek in closed file");
    }

    const int whence = args[1].as.number;
    if (whence > 2) {
        bs_error_at(bs, 2, "invalid whence '%d'", whence);
    }

    return bs_value_bool(fseek(f, args[0].as.number, whence) != -1);
}

Bs_Value bs_io_reader_tell(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    FILE *f = bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, FILE *);
    if (!f) {
        bs_error(bs, "cannot get position of closed file");
    }

    const long position = ftell(f);
    return position == -1 ? bs_value_nil : bs_value_num(position);
}

// Writer
Bs_Value bs_io_writer_init(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    Bs_Buffer *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    Bs_Writer w = bs_buffer_writer(b);
    bs_fmt(&w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[0].as.object));
    bs_da_push(b->bs, b, '\0');

    FILE *file = fopen(bs_buffer_reset(b, start).data, "w");
    if (!file) {
        return bs_value_nil;
    }

    bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, FILE *) = file;
    return args[-1];
}

Bs_Value bs_io_writer_flush(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    FILE *f = bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, FILE *);
    if (!f) {
        bs_error(bs, "cannot flush closed file");
    }

    fflush(f);
    return bs_value_nil;
}

Bs_Value bs_io_writer_write(Bs *bs, Bs_Value *args, size_t arity) {
    FILE *f = bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, FILE *);
    if (!f) {
        bs_error(bs, "cannot write into closed file");
    }

    Bs_Writer w = bs_file_writer(f);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(&w, " ");
        }
        bs_value_write(bs, &w, args[i]);
    }

    return bs_value_bool(!ferror(f));
}

Bs_Value bs_io_writer_writeln(Bs *bs, Bs_Value *args, size_t arity) {
    FILE *f = bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, FILE *);
    if (!f) {
        bs_error(bs, "cannot write into closed file");
    }

    Bs_Writer w = bs_file_writer(f);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(&w, " ");
        }
        bs_value_write(bs, &w, args[i]);
    }
    bs_fmt(&w, "\n");

    return bs_value_bool(!ferror(f));
}

Bs_Value bs_io_input(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity > 1) {
        bs_error(bs, "expected 0 or 1 arguments, got %zu", arity);
    }

    if (arity) {
        bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
        printf(Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[0].as.object));
        fflush(stdout);
    }

    Bs_Buffer *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    while (true) {
        const char c = fgetc(stdin);
        if (c == '\n' || c == EOF) {
            break;
        }
        bs_da_push(bs, b, c);
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

Bs_Value bs_io_print(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer w = bs_file_writer(stdout);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(&w, " ");
        }
        bs_value_write(bs, &w, args[i]);
    }
    return bs_value_nil;
}

Bs_Value bs_io_eprint(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer w = bs_file_writer(stderr);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(&w, " ");
        }
        bs_value_write(bs, &w, args[i]);
    }
    return bs_value_nil;
}

Bs_Value bs_io_println(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer w = bs_file_writer(stdout);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(&w, " ");
        }
        bs_value_write(bs, &w, args[i]);
    }
    bs_fmt(&w, "\n");
    return bs_value_nil;
}

Bs_Value bs_io_eprintln(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer w = bs_file_writer(stderr);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(&w, " ");
        }
        bs_value_write(bs, &w, args[i]);
    }
    bs_fmt(&w, "\n");
    return bs_value_nil;
}

// OS
Bs_Value bs_os_exit(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

    bs_unwind(bs, args[0].as.number);
    assert(false && "unreachable");
}

Bs_Value bs_os_clock(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    struct timespec clock;
    if (clock_gettime(CLOCK_MONOTONIC, &clock) < 0) {
        bs_error(bs, "could not get clock");
    }

    return bs_value_num(clock.tv_sec + clock.tv_nsec * 1e-9);
}

Bs_Value bs_os_sleep(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);

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

Bs_Value bs_os_getenv(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    {
        Bs_Buffer *b = &bs_config(bs)->buffer;
        const size_t start = b->count;

        Bs_Writer w = bs_buffer_writer(b);
        bs_fmt(&w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[0].as.object));
        bs_da_push(b->bs, b, '\0');

        const char *key = bs_buffer_reset(b, start).data;
        const char *value = getenv(key);

        if (value) {
            return bs_value_object(bs_str_new(bs, bs_sv_from_cstr(value)));
        } else {
            return bs_value_nil;
        }
    }
}

Bs_Value bs_os_setenv(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
    bs_arg_check_object_type(bs, args, 1, BS_OBJECT_STR);

    const char *key;
    const char *value;
    {
        Bs_Buffer *b = &bs_config(bs)->buffer;
        const size_t start = b->count;

        Bs_Writer w = bs_buffer_writer(b);

        const size_t key_pos = b->count;
        bs_fmt(&w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[0].as.object));
        bs_da_push(b->bs, b, '\0');

        const size_t value_pos = b->count;
        bs_fmt(&w, Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *)args[1].as.object));
        bs_da_push(b->bs, b, '\0');

        key = b->data + key_pos;
        value = b->data + value_pos;

        bs_buffer_reset(b, start);
    }

    return bs_value_bool(setenv(key, value, true) == 0);
}

// Process
typedef struct {
    pid_t pid;
} Bs_Process;

Bs_Value bs_process_init(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_ARRAY);

    const Bs_Array *array = (const Bs_Array *)args[0].as.object;
    if (!array->count) {
        bs_error(bs, "cannot execute empty array as process");
    }

    for (size_t i = 0; i < array->count; i++) {
        char buffer[64];
        const int count = snprintf(buffer, sizeof(buffer), "command string #%zu", i + 1);
        assert(count >= 0 && count + 1 < sizeof(buffer));
        bs_check_object_type_at(bs, 1, array->data[i], BS_OBJECT_STR, buffer);
    }

    int fail[2];
    if (pipe(fail) < 0) {
        bs_error(bs, "could not create failure pipe");
    }

    if (fcntl(fail[1], F_SETFD, FD_CLOEXEC) < 0) {
        bs_error(bs, "could not create failure pipe");
    }

    const pid_t pid = fork();
    if (pid < 0) {
        close(fail[0]);
        close(fail[1]);
        bs_error(bs, "could not fork process");
    }

    if (pid == 0) {
        Bs_Buffer *b = &bs_config(bs)->buffer;

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

        close(fail[0]);
        execvp(*cargv, (char *const *)cargv);
        write(fail[1], "F", 1);
        close(fail[1]);
        exit(127);
    }

    close(fail[1]);
    char buffer[1];
    const long count = read(fail[0], buffer, sizeof(buffer));
    close(fail[0]);

    if (count > 0) {
        waitpid(pid, NULL, 0); // Wait for the child to kill itself so it doesn't become a zombie
        return bs_value_nil;
    }

    Bs_Process *p = &bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, Bs_Process);
    p->pid = pid;
    return args[-1];
}

Bs_Value bs_process_kill(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

    Bs_Process *p = &bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, Bs_Process);
    if (!p->pid) {
        bs_error(bs, "cannot kill already terminated process");
    }

    if (kill(p->pid, args[0].as.number) < 0) {
        return bs_value_bool(false);
    }

    p->pid = 0;
    return bs_value_bool(true);
}

Bs_Value bs_process_wait(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    Bs_Process *p = &bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, Bs_Process);
    if (!p->pid) {
        bs_error(bs, "cannot wait for already terminated process");
    }

    int status;
    if (waitpid(p->pid, &status, 0) < 0) {
        return bs_value_nil;
    }

    p->pid = 0;
    return WIFEXITED(status) ? bs_value_num(WEXITSTATUS(status)) : bs_value_nil;
}

// Regex
typedef struct {
    pcre2_code *code;
    pcre2_match_data *match_data;
} Bs_Regex;

Bs_C_Class *bs_regex_class;

void bs_regex_free(void *userdata, void *instance_data) {
    Bs_Regex *regex = &bs_static_cast(instance_data, Bs_Regex);
    pcre2_code_free(regex->code);
    pcre2_match_data_free(regex->match_data);
}

Bs_Value bs_regex_init(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;

    int temp1 = 0;
    size_t temp2 = 0;

    Bs_Regex *regex = &bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, Bs_Regex);
    regex->code = pcre2_compile((PCRE2_SPTR)str->data, str->size, 0, &temp1, &temp2, NULL);
    if (!regex->code) {
        return bs_value_nil;
    }

    regex->match_data = pcre2_match_data_create_from_pattern(regex->code, NULL);
    if (!regex->match_data) {
        return bs_value_nil;
    }

    return args[-1];
}

// Str
Bs_Value bs_str_slice(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_whole_number(bs, args, 0);
    bs_arg_check_whole_number(bs, args, 1);

    Bs_Str *str = (Bs_Str *)args[-1].as.object;
    const size_t begin = bs_min(args[0].as.number, args[1].as.number);
    const size_t end = bs_max(args[0].as.number, args[1].as.number);

    if (begin >= str->size || end > str->size) {
        bs_error(bs, "cannot slice string of length %zu from %zu to %zu", str->size, begin, end);
    }

    return bs_value_object(bs_str_new(bs, Bs_Sv(str->data + begin, end - begin)));
}

Bs_Value bs_str_reverse(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    const Bs_Str *src = (const Bs_Str *)args[-1].as.object;

    Bs_Buffer *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    for (size_t i = 0; i < src->size; i++) {
        bs_da_push(bs, b, src->data[src->size - i - 1]);
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

Bs_Value bs_str_tolower(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    const Bs_Str *src = (const Bs_Str *)args[-1].as.object;

    Bs_Buffer *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    for (size_t i = 0; i < src->size; i++) {
        bs_da_push(bs, b, tolower(src->data[i]));
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

Bs_Value bs_str_toupper(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    const Bs_Str *src = (const Bs_Str *)args[-1].as.object;

    Bs_Buffer *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    for (size_t i = 0; i < src->size; i++) {
        bs_da_push(bs, b, toupper(src->data[i]));
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

Bs_Value bs_str_tonumber(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    const Bs_Str *src = (const Bs_Str *)args[-1].as.object;

    Bs_Buffer *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    bs_da_push_many(b->bs, b, src->data, src->size);
    bs_da_push(b->bs, b, '\0');

    const char *input = bs_buffer_reset(b, start).data;
    char *end;

    const double value = strtod(input, &end);
    return (end == input || *end != '\0' || errno == ERANGE) ? bs_value_nil : bs_value_num(value);
}

Bs_Value bs_str_find(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 1 && arity != 2) {
        bs_error(bs, "expected 1 or 2 arguments, got %zu", arity);
    }

    const Bs_Check checks[] = {
        bs_check_object(BS_OBJECT_STR),
        bs_check_c_instance(bs_regex_class),
    };
    bs_arg_check_multi(bs, args, 0, checks, bs_c_array_size(checks));

    if (arity == 2) {
        bs_arg_check_whole_number(bs, args, 1);
    }

    const Bs_Str *str = (const Bs_Str *)args[-1].as.object;
    const size_t offset = arity == 2 ? args[1].as.number : 0;
    if (offset > str->size) {
        bs_error_at(bs, 2, "cannot take offset of %zu in string of length %zu", offset, str->size);
    }

    if (args[0].as.object->type == BS_OBJECT_STR) {
        const Bs_Str *pattern = (const Bs_Str *)args[0].as.object;
        if (!pattern->size) {
            return bs_value_nil;
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
    } else {
        const Bs_Regex *regex =
            &bs_static_cast(((Bs_C_Instance *)args[0].as.object)->data, Bs_Regex);

        const int count = pcre2_match(
            regex->code, (PCRE2_SPTR)str->data, str->size, offset, 0, regex->match_data, NULL);

        if (count >= 0) {
            const size_t *ovector = pcre2_get_ovector_pointer(regex->match_data);
            return bs_value_num(ovector[0]);
        }
    }

    return bs_value_nil;
}

Bs_Value bs_str_split(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);

    const Bs_Check checks[] = {
        bs_check_object(BS_OBJECT_STR),
        bs_check_c_instance(bs_regex_class),
    };
    bs_arg_check_multi(bs, args, 0, checks, bs_c_array_size(checks));

    const Bs_Str *str = (const Bs_Str *)args[-1].as.object;

    Bs_Array *a = bs_array_new(bs);
    size_t j = 0;

    if (args[0].as.object->type == BS_OBJECT_STR) {
        const Bs_Str *pattern = (const Bs_Str *)args[0].as.object;
        if (!pattern->size) {
            bs_array_set(bs, a, a->count, bs_value_object(str));
            return bs_value_object(a);
        }
        const Bs_Sv pattern_sv = Bs_Sv(pattern->data, pattern->size);

        size_t i = 0;
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
    } else {
        const Bs_Regex *regex =
            &bs_static_cast(((Bs_C_Instance *)args[0].as.object)->data, Bs_Regex);

        while (true) {
            const int count = pcre2_match(
                regex->code, (PCRE2_SPTR)str->data, str->size, j, 0, regex->match_data, NULL);

            if (count < 0) {
                break;
            }

            const size_t *ovector = pcre2_get_ovector_pointer(regex->match_data);
            if (ovector[0] == ovector[1]) {
                break;
            }

            bs_array_set(
                bs,
                a,
                a->count,
                bs_value_object(bs_str_new(bs, Bs_Sv(str->data + j, ovector[0] - j))));

            j = ovector[1];
        }
    }

    if (j != str->size) {
        bs_array_set(
            bs, a, a->count, bs_value_object(bs_str_new(bs, Bs_Sv(str->data + j, str->size - j))));
    }

    return bs_value_object(a);
}

Bs_Value bs_str_replace(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);

    const Bs_Check checks[] = {
        bs_check_object(BS_OBJECT_STR),
        bs_check_c_instance(bs_regex_class),
    };
    bs_arg_check_multi(bs, args, 0, checks, bs_c_array_size(checks));
    bs_arg_check_object_type(bs, args, 1, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[-1].as.object;
    const Bs_Str *replacement = (const Bs_Str *)args[1].as.object;

    if (args[0].as.object->type == BS_OBJECT_STR) {
        const Bs_Str *pattern = (const Bs_Str *)args[0].as.object;
        if (!pattern->size) {
            return bs_value_object(str);
        }

        if (str->size < pattern->size) {
            return bs_value_object(str);
        }

        Bs_Buffer *b = &bs_config(bs)->buffer;
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
    } else {
        Bs_Buffer *b = &bs_config(bs)->buffer;
        const size_t start = b->count;

        const Bs_Regex *regex =
            &bs_static_cast(((Bs_C_Instance *)args[0].as.object)->data, Bs_Regex);

        size_t j = 0;
        while (true) {
            const int count = pcre2_match(
                regex->code, (PCRE2_SPTR)str->data, str->size, j, 0, regex->match_data, NULL);

            if (count < 0) {
                break;
            }

            const size_t *ovector = pcre2_get_ovector_pointer(regex->match_data);
            if (ovector[0] == ovector[1]) {
                break;
            }

            bs_da_push_many(bs, b, str->data + j, ovector[0] - j);

            for (size_t i = 0; i < replacement->size; i++) {
                if (i + 1 < replacement->size && replacement->data[i] == '\\' &&
                    isdigit(replacement->data[i + 1])) {
                    const size_t m = replacement->data[++i] - '0';
                    if (m < count) {
                        bs_da_push_many(
                            bs, b, str->data + ovector[m * 2], ovector[m * 2 + 1] - ovector[m * 2]);
                    }
                } else {
                    bs_da_push(bs, b, replacement->data[i]);
                }
            }

            j = ovector[1];
        }
        bs_da_push_many(bs, b, str->data + j, str->size - j);

        return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
    }
}

Bs_Value bs_str_trim(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[-1].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[0].as.object;
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

Bs_Value bs_str_ltrim(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[-1].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[0].as.object;
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

    return bs_value_object(bs_str_new(bs, str_sv));
}

Bs_Value bs_str_rtrim(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[-1].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[0].as.object;
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

Bs_Value bs_str_lpad(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
    bs_arg_check_whole_number(bs, args, 1);

    const Bs_Str *str = (const Bs_Str *)args[-1].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[0].as.object;
    const size_t count = args[1].as.number;
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

    Bs_Buffer *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    for (size_t i = 0; i < padding; i += pattern->size) {
        bs_da_push_many(bs, b, pattern->data, bs_min(pattern->size, padding - i));
    }
    bs_da_push_many(bs, b, str->data, str->size);

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

Bs_Value bs_str_rpad(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
    bs_arg_check_whole_number(bs, args, 1);

    const Bs_Str *str = (const Bs_Str *)args[-1].as.object;
    const Bs_Str *pattern = (const Bs_Str *)args[0].as.object;
    const size_t count = args[1].as.number;
    if (!str->size || !pattern->size) {
        return bs_value_object(str);
    }

    if (pattern->size > str->size) {
        return bs_value_object(str);
    }

    if (str->size >= count) {
        return bs_value_object(str);
    }

    Bs_Buffer *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    bs_da_push_many(bs, b, str->data, str->size);
    for (size_t i = str->size; i < count; i += pattern->size) {
        bs_da_push_many(bs, b, pattern->data, bs_min(pattern->size, count - i));
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

// Bit
Bs_Value bs_bit_ceil(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

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

Bs_Value bs_bit_floor(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

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

// Ascii
Bs_Value bs_ascii_char(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

    const size_t code = args[0].as.number;
    if (code > 0xff) {
        bs_error_at(bs, 1, "invalid ascii code '%zu'", code);
    }

    const char ch = code;
    return bs_value_object(bs_str_new(bs, Bs_Sv(&ch, 1)));
}

Bs_Value bs_ascii_code(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_num(*str->data);
}

Bs_Value bs_ascii_isalnum(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isalnum(*str->data));
}

Bs_Value bs_ascii_isalpha(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isalpha(*str->data));
}

Bs_Value bs_ascii_iscntrl(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(iscntrl(*str->data));
}

Bs_Value bs_ascii_isdigit(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isdigit(*str->data));
}

Bs_Value bs_ascii_islower(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(islower(*str->data));
}

Bs_Value bs_ascii_isgraph(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isgraph(*str->data));
}

Bs_Value bs_ascii_isprint(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isprint(*str->data));
}

Bs_Value bs_ascii_ispunct(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(ispunct(*str->data));
}

Bs_Value bs_ascii_isspace(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isspace(*str->data));
}

Bs_Value bs_ascii_isupper(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *)args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isupper(*str->data));
}

// Bytes
void bs_bytes_free(void *userdata, void *instance_data) {
    Bs_Buffer *b = &bs_static_cast(instance_data, Bs_Buffer);
    bs_da_free(b->bs, b);
}

Bs_Value bs_bytes_init(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Bs_Buffer *b = &bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, Bs_Buffer);
    b->bs = bs;
    return bs_value_nil;
}

Bs_Value bs_bytes_count(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    const Bs_Buffer *b = &bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, Bs_Buffer);
    return bs_value_num(b->count);
}

Bs_Value bs_bytes_reset(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

    Bs_Buffer *b = &bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, Bs_Buffer);
    const size_t reset = args[0].as.number;

    if (reset > b->count) {
        bs_error(bs, "cannot reset bytes of length %zu to %zu", b->count, reset);
    }

    b->count = reset;
    return bs_value_nil;
}

Bs_Value bs_bytes_slice(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_whole_number(bs, args, 0);
    bs_arg_check_whole_number(bs, args, 1);

    const Bs_Buffer *b = &bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, Bs_Buffer);
    const size_t begin = bs_min(args[0].as.number, args[1].as.number);
    const size_t end = bs_max(args[0].as.number, args[1].as.number);

    if (begin == end) {
        return bs_value_object(bs_str_new(bs, Bs_Sv_Static("")));
    }

    if (begin >= b->count || end > b->count) {
        bs_error(bs, "cannot slice bytes of length %zu from %zu to %zu", b->count, begin, end);
    }

    return bs_value_object(bs_str_new(bs, Bs_Sv(b->data + begin, end - begin)));
}

Bs_Value bs_bytes_write(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    Bs_Buffer *b = &bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, Bs_Buffer);
    const Bs_Str *src = (const Bs_Str *)args[0].as.object;
    bs_da_push_many(bs, b, src->data, src->size);

    return bs_value_nil;
}

Bs_Value bs_bytes_insert(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_whole_number(bs, args, 0);
    bs_arg_check_object_type(bs, args, 1, BS_OBJECT_STR);

    Bs_Buffer *b = &bs_static_cast(((Bs_C_Instance *)args[-1].as.object)->data, Bs_Buffer);
    const size_t index = args[0].as.number;
    const Bs_Str *src = (const Bs_Str *)args[1].as.object;

    if (index > b->count) {
        bs_error(bs, "cannot insert at %zu into bytes of length %zu", index, b->count);
    }

    bs_da_push_many(bs, b, NULL, src->size);
    memmove(b->data + index + src->size, b->data + index, b->count - index);
    memcpy(b->data + index, src->data, src->size);
    b->count += src->size;

    return bs_value_nil;
}

// Array
Bs_Value bs_array_map(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_callable(bs, args, 0);

    const Bs_Array *src = (const Bs_Array *)args[-1].as.object;
    const Bs_Value fn = args[0];
    Bs_Array *dst = bs_array_new(bs);

    for (size_t i = 0; i < src->count; i++) {
        const Bs_Value input = src->data[i];
        const Bs_Value output = bs_call(bs, fn, &input, 1);
        bs_array_set(bs, dst, i, output);
    }

    return bs_value_object(dst);
}

Bs_Value bs_array_filter(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_callable(bs, args, 0);

    const Bs_Array *src = (const Bs_Array *)args[-1].as.object;
    const Bs_Value fn = args[0];
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

Bs_Value bs_array_reduce(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 1 && arity != 2) {
        bs_error(bs, "expected 1 or 2 arguments, got %zu", arity);
    }

    bs_arg_check_callable(bs, args, 0);

    const Bs_Array *src = (const Bs_Array *)args[-1].as.object;
    const Bs_Value fn = args[0];
    Bs_Value acc = arity == 2 ? args[1] : bs_value_nil;

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

Bs_Value bs_array_copy(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    const Bs_Array *src = (const Bs_Array *)args[-1].as.object;

    Bs_Array *dst = bs_array_new(bs);
    for (size_t i = src->count; i > 0; i--) {
        bs_array_set(bs, dst, i - 1, src->data[i - 1]);
    }

    return bs_value_object(dst);
}

Bs_Value bs_array_join(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    Bs_Buffer *b = &bs_config(bs)->buffer;
    const size_t start = b->count;
    {
        Bs_Writer w = bs_buffer_writer(b);

        const Bs_Array *array = (const Bs_Array *)args[-1].as.object;
        const Bs_Str *str = (const Bs_Str *)args[0].as.object;
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

Bs_Value bs_array_find(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 1 && arity != 2) {
        bs_error(bs, "expected 1 or 2 arguments, got %zu", arity);
    }

    if (arity == 2) {
        bs_arg_check_whole_number(bs, args, 1);
    }

    const Bs_Array *array = (const Bs_Array *)args[-1].as.object;
    const Bs_Value pred = args[0];
    const size_t offset = arity == 2 ? args[1].as.number : 0;

    for (size_t i = offset; i < array->count; i++) {
        if (bs_value_equal(array->data[i], pred)) {
            return bs_value_num(i);
        }
    }

    return bs_value_nil;
}

Bs_Value bs_array_equal(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_ARRAY);

    const Bs_Array *a = (const Bs_Array *)args[-1].as.object;
    const Bs_Array *b = (const Bs_Array *)args[0].as.object;

    if (a == b) {
        return bs_value_bool(true);
    }

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

Bs_Value bs_array_push(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    Bs_Array *a = (Bs_Array *)args[-1].as.object;
    bs_array_set(bs, a, a->count, args[0]);
    return bs_value_nil;
}

Bs_Value bs_array_insert(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_whole_number(bs, args, 0);

    Bs_Array *a = (Bs_Array *)args[-1].as.object;
    const size_t index = args[0].as.number;
    const Bs_Value value = args[1];

    if (index < a->count) {
        bs_array_set(bs, a, a->count, bs_value_nil);

        for (size_t i = index; i + 1 < a->count; i++) {
            a->data[i + 1] = a->data[i];
        }

        a->data[index] = value;
    } else {
        bs_array_set(bs, a, index, value);
    }
    return bs_value_nil;
}

typedef struct {
    Bs *bs;
    Bs_Value fn;
} Bs_Array_Sort_Context;

int bs_array_sort_compare(const void *a, const void *b) {
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

Bs_Value bs_array_sort(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_callable(bs, args, 0);

    Bs_Array *src = (Bs_Array *)args[-1].as.object;
    const Bs_Value fn = args[0];

    // Prepare the context
    const Bs_Array_Sort_Context context = {
        .bs = bs,
        .fn = fn,
    };
    bs_array_sort_compare(NULL, &context);

    qsort(src->data, src->count, sizeof(*src->data), bs_array_sort_compare);
    return bs_value_object(src);
}

Bs_Value bs_array_resize(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

    Bs_Array *src = (Bs_Array *)args[-1].as.object;
    const size_t size = args[0].as.number;
    if (size > src->count) {
        bs_array_set(bs, src, size - 1, bs_value_nil);
    } else {
        src->count = size;
    }
    return bs_value_object(src);
}

Bs_Value bs_array_reverse(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    Bs_Array *src = (Bs_Array *)args[-1].as.object;
    for (size_t i = 0; i < src->count / 2; i++) {
        const Bs_Value t = src->data[i];
        src->data[i] = src->data[src->count - i - 1];
        src->data[src->count - i - 1] = t;
    }
    return bs_value_object(src);
}

// Table
Bs_Value bs_table_copy(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    const Bs_Table *src = (const Bs_Table *)args[-1].as.object;
    Bs_Table *dst = bs_table_new(bs);

    bs_map_copy(bs, &dst->map, &src->map);
    return bs_value_object(dst);
}

Bs_Value bs_table_equal(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_TABLE);

    const Bs_Table *a = (const Bs_Table *)args[-1].as.object;
    const Bs_Table *b = (const Bs_Table *)args[0].as.object;

    if (a == b) {
        return bs_value_bool(true);
    }

    if (a->map.length != b->map.length) {
        return bs_value_bool(false);
    }

    for (size_t i = 0; i < a->map.capacity; i++) {
        if (a->map.data[i].key.type != BS_VALUE_NIL) {
            const Bs_Entry *e = bs_entries_find(b->map.data, b->map.capacity, a->map.data[i].key);
            if (!e || e->key.type == BS_VALUE_NIL) {
                return bs_value_bool(false);
            }

            if (!bs_value_equal(a->map.data[i].value, e->value)) {
                return bs_value_bool(false);
            }
        }
    }

    return bs_value_bool(true);
}

// Math
Bs_Value bs_num_sin(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(sin(args[-1].as.number));
}

Bs_Value bs_math_cos(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(cos(args[-1].as.number));
}

Bs_Value bs_math_tan(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(tan(args[-1].as.number));
}

Bs_Value bs_math_asin(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(asin(args[-1].as.number));
}

Bs_Value bs_math_acos(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(acos(args[-1].as.number));
}

Bs_Value bs_math_atan(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(atan(args[-1].as.number));
}

Bs_Value bs_math_exp(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(exp(args[-1].as.number));
}

Bs_Value bs_math_log(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(log(args[-1].as.number));
}

Bs_Value bs_math_log10(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(log10(args[-1].as.number));
}

Bs_Value bs_math_pow(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    return bs_value_num(pow(args[-1].as.number, args[0].as.number));
}

Bs_Value bs_math_sqrt(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    if (args[-1].as.number < 0) {
        bs_error(bs, "complex numbers are not supported");
    }
    return bs_value_num(sqrt(args[-1].as.number));
}

Bs_Value bs_math_ceil(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(ceil(args[-1].as.number));
}

Bs_Value bs_math_floor(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(floor(args[-1].as.number));
}

Bs_Value bs_math_round(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(round(args[-1].as.number));
}

Bs_Value bs_math_random(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num((double)rand() / RAND_MAX);
}

Bs_Value bs_math_max(Bs *bs, Bs_Value *args, size_t arity) {
    if (!arity) {
        bs_error(bs, "expected at least 1 argument, got 0");
    }

    double max = args[-1].as.number;
    for (size_t i = 0; i < arity; i++) {
        bs_arg_check_value_type(bs, args, i, BS_VALUE_NUM);
        max = bs_max(max, args[i].as.number);
    }
    return bs_value_num(max);
}

Bs_Value bs_math_min(Bs *bs, Bs_Value *args, size_t arity) {
    if (!arity) {
        bs_error(bs, "expected at least 1 argument, got 0");
    }

    double min = args[-1].as.number;
    for (size_t i = 0; i < arity; i++) {
        bs_arg_check_value_type(bs, args, i, BS_VALUE_NUM);
        min = bs_min(min, args[i].as.number);
    }
    return bs_value_num(min);
}

Bs_Value bs_math_clamp(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);

    const double low = bs_min(args[0].as.number, args[1].as.number);
    const double high = bs_max(args[0].as.number, args[1].as.number);
    return bs_value_num(bs_min(bs_max(args[-1].as.number, low), high));
}

Bs_Value bs_math_lerp(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);

    const double a = args[-1].as.number;
    const double b = args[0].as.number;
    const double t = args[1].as.number;
    return bs_value_num(a + (b - a) * t);
}

// Main
static void bs_add(Bs *bs, Bs_Table *table, const char *key, Bs_Value value) {
    bs_table_set(bs, table, bs_value_object(bs_str_new(bs, bs_sv_from_cstr(key))), value);
}

static void bs_add_fn(Bs *bs, Bs_Table *table, const char *key, Bs_C_Fn_Ptr fn) {
    bs_table_set(
        bs,
        table,
        bs_value_object(bs_str_new(bs, bs_sv_from_cstr(key))),
        bs_value_object(bs_c_fn_new(bs, bs_sv_from_cstr(key), fn)));
}

void bs_core_init(Bs *bs, int argc, char **argv) {
    srand(time(NULL));

    {
        Bs_C_Class *io_reader_class = bs_c_class_new(
            bs, Bs_Sv_Static("Reader"), sizeof(FILE *), bs_io_reader_init, bs_io_file_free);

        io_reader_class->can_fail = true;
        bs_c_class_add(bs, io_reader_class, Bs_Sv_Static("close"), bs_io_file_close);
        bs_c_class_add(bs, io_reader_class, Bs_Sv_Static("read"), bs_io_reader_read);
        bs_c_class_add(bs, io_reader_class, Bs_Sv_Static("readln"), bs_io_reader_readln);

        bs_c_class_add(bs, io_reader_class, Bs_Sv_Static("seek"), bs_io_reader_seek);
        bs_c_class_add(bs, io_reader_class, Bs_Sv_Static("tell"), bs_io_reader_tell);

        Bs_C_Class *io_writer_class = bs_c_class_new(
            bs, Bs_Sv_Static("Writer"), sizeof(FILE *), bs_io_writer_init, bs_io_file_free);

        io_writer_class->can_fail = true;
        bs_c_class_add(bs, io_writer_class, Bs_Sv_Static("close"), bs_io_file_close);
        bs_c_class_add(bs, io_writer_class, Bs_Sv_Static("flush"), bs_io_writer_flush);
        bs_c_class_add(bs, io_writer_class, Bs_Sv_Static("write"), bs_io_writer_write);
        bs_c_class_add(bs, io_writer_class, Bs_Sv_Static("writeln"), bs_io_writer_writeln);

        Bs_Table *io = bs_table_new(bs);
        bs_add(bs, io, "Reader", bs_value_object(io_reader_class));
        bs_add(bs, io, "Writer", bs_value_object(io_writer_class));

        bs_add_fn(bs, io, "input", bs_io_input);

        bs_add_fn(bs, io, "print", bs_io_print);
        bs_add_fn(bs, io, "eprint", bs_io_eprint);

        bs_add_fn(bs, io, "println", bs_io_println);
        bs_add_fn(bs, io, "eprintln", bs_io_eprintln);

        {
            Bs_C_Instance *io_stdin = bs_c_instance_new(bs, io_reader_class);
            bs_static_cast(io_stdin->data, FILE *) = stdin;
            bs_add(bs, io, "stdin", bs_value_object(io_stdin));
        }

        {
            Bs_C_Instance *io_stdout = bs_c_instance_new(bs, io_writer_class);
            bs_static_cast(io_stdout->data, FILE *) = stdout;
            bs_add(bs, io, "stdout", bs_value_object(io_stdout));
        }

        {
            Bs_C_Instance *io_stderr = bs_c_instance_new(bs, io_writer_class);
            bs_static_cast(io_stderr->data, FILE *) = stderr;
            bs_add(bs, io, "stderr", bs_value_object(io_stderr));
        }

        {
            bs_add(bs, io, "SEEK_SET", bs_value_num(SEEK_SET));
            bs_add(bs, io, "SEEK_CUR", bs_value_num(SEEK_CUR));
            bs_add(bs, io, "SEEK_END", bs_value_num(SEEK_END));
        }

        bs_global_set(bs, Bs_Sv_Static("io"), bs_value_object(io));
    }

    {
        Bs_Table *os = bs_table_new(bs);
        bs_add_fn(bs, os, "exit", bs_os_exit);
        bs_add_fn(bs, os, "clock", bs_os_clock);
        bs_add_fn(bs, os, "sleep", bs_os_sleep);

        bs_add_fn(bs, os, "getenv", bs_os_getenv);
        bs_add_fn(bs, os, "setenv", bs_os_setenv);

        Bs_Array *args = bs_array_new(bs);
        for (int i = 0; i < argc; i++) {
            bs_array_set(bs, args, i, bs_value_object(bs_str_new(bs, bs_sv_from_cstr(argv[i]))));
        }
        bs_add(bs, os, "args", bs_value_object(args));

        Bs_C_Class *process_class =
            bs_c_class_new(bs, Bs_Sv_Static("Process"), sizeof(Bs_Process), bs_process_init, NULL);

        process_class->can_fail = true;
        bs_c_class_add(bs, process_class, Bs_Sv_Static("kill"), bs_process_kill);
        bs_c_class_add(bs, process_class, Bs_Sv_Static("wait"), bs_process_wait);

        bs_add(bs, os, "Process", bs_value_object(process_class));

        bs_global_set(bs, Bs_Sv_Static("os"), bs_value_object(os));
    }

    {
        bs_regex_class = bs_c_class_new(
            bs, Bs_Sv_Static("Regex"), sizeof(Bs_Regex), bs_regex_init, bs_regex_free);

        bs_regex_class->can_fail = true;
        bs_global_set(bs, Bs_Sv_Static("Regex"), bs_value_object(bs_regex_class));
    }

    {
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("slice"), bs_str_slice);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("reverse"), bs_str_reverse);

        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("toupper"), bs_str_toupper);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("tolower"), bs_str_tolower);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("tonumber"), bs_str_tonumber);

        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("find"), bs_str_find);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("split"), bs_str_split);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("replace"), bs_str_replace);

        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("trim"), bs_str_trim);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("ltrim"), bs_str_ltrim);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("rtrim"), bs_str_rtrim);

        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("lpad"), bs_str_lpad);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("rpad"), bs_str_rpad);
    }

    {
        Bs_Table *bit = bs_table_new(bs);
        bs_add_fn(bs, bit, "ceil", bs_bit_ceil);
        bs_add_fn(bs, bit, "floor", bs_bit_floor);
        bs_global_set(bs, Bs_Sv_Static("bit"), bs_value_object(bit));
    }

    {
        Bs_Table *ascii = bs_table_new(bs);
        bs_add_fn(bs, ascii, "char", bs_ascii_char);
        bs_add_fn(bs, ascii, "code", bs_ascii_code);
        bs_add_fn(bs, ascii, "isalnum", bs_ascii_isalnum);
        bs_add_fn(bs, ascii, "isalpha", bs_ascii_isalpha);
        bs_add_fn(bs, ascii, "iscntrl", bs_ascii_iscntrl);
        bs_add_fn(bs, ascii, "isdigit", bs_ascii_isdigit);
        bs_add_fn(bs, ascii, "islower", bs_ascii_islower);
        bs_add_fn(bs, ascii, "isgraph", bs_ascii_isgraph);
        bs_add_fn(bs, ascii, "isprint", bs_ascii_isprint);
        bs_add_fn(bs, ascii, "ispunct", bs_ascii_ispunct);
        bs_add_fn(bs, ascii, "isspace", bs_ascii_isspace);
        bs_add_fn(bs, ascii, "isupper", bs_ascii_isupper);
        bs_global_set(bs, Bs_Sv_Static("ascii"), bs_value_object(ascii));
    }

    {
        Bs_C_Class *bytes_class = bs_c_class_new(
            bs, Bs_Sv_Static("Bytes"), sizeof(Bs_Buffer), bs_bytes_init, bs_bytes_free);

        bs_c_class_add(bs, bytes_class, Bs_Sv_Static("count"), bs_bytes_count);
        bs_c_class_add(bs, bytes_class, Bs_Sv_Static("reset"), bs_bytes_reset);

        bs_c_class_add(bs, bytes_class, Bs_Sv_Static("slice"), bs_bytes_slice);
        bs_c_class_add(bs, bytes_class, Bs_Sv_Static("write"), bs_bytes_write);
        bs_c_class_add(bs, bytes_class, Bs_Sv_Static("insert"), bs_bytes_insert);

        bs_global_set(bs, Bs_Sv_Static("Bytes"), bs_value_object(bytes_class));
    }

    {
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("map"), bs_array_map);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("filter"), bs_array_filter);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("reduce"), bs_array_reduce);

        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("copy"), bs_array_copy);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("join"), bs_array_join);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("find"), bs_array_find);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("equal"), bs_array_equal);

        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("push"), bs_array_push);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("insert"), bs_array_insert);

        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("sort"), bs_array_sort);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("resize"), bs_array_resize);
        bs_builtin_object_methods_add(
            bs, BS_OBJECT_ARRAY, Bs_Sv_Static("reverse"), bs_array_reverse);
    }

    {
        bs_builtin_object_methods_add(bs, BS_OBJECT_TABLE, Bs_Sv_Static("copy"), bs_table_copy);
        bs_builtin_object_methods_add(bs, BS_OBJECT_TABLE, Bs_Sv_Static("equal"), bs_table_equal);
    }

    {
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("sin"), bs_num_sin);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("cos"), bs_math_cos);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("tan"), bs_math_tan);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("asin"), bs_math_asin);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("acos"), bs_math_acos);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("atan"), bs_math_atan);

        bs_builtin_number_methods_add(bs, Bs_Sv_Static("exp"), bs_math_exp);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("log"), bs_math_log);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("log10"), bs_math_log10);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("pow"), bs_math_pow);

        bs_builtin_number_methods_add(bs, Bs_Sv_Static("sqrt"), bs_math_sqrt);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("ceil"), bs_math_ceil);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("floor"), bs_math_floor);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("round"), bs_math_round);

        bs_builtin_number_methods_add(bs, Bs_Sv_Static("max"), bs_math_max);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("min"), bs_math_min);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("clamp"), bs_math_clamp);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("lerp"), bs_math_lerp);

        Bs_Table *math = bs_table_new(bs);
        bs_add(bs, math, "E", bs_value_num(M_E));
        bs_add(bs, math, "PI", bs_value_num(M_PI));
        bs_add_fn(bs, math, "random", bs_math_random);
        bs_global_set(bs, Bs_Sv_Static("math"), bs_value_object(math));
    }
}
