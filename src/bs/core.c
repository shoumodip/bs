#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#    define WIN32_LEAN_AND_MEAN
#    include <io.h>
#    include <process.h>
#    include <windows.h>

#    define FD_OPEN _fdopen
#else
#    include <dirent.h>
#    include <signal.h>
#    include <sys/stat.h>
#    include <sys/wait.h>
#    include <unistd.h>

#    define FD_OPEN fdopen
#endif // _WIN32

#include "bs/compiler.h"
#include "bs/core.h"
#include "regex/regex.h"

// IO
typedef struct {
    FILE *file;
    bool  seekable;
} Bs_File;

static void bs_io_file_free(void *userdata, void *instance_data) {
    Bs_File *f = &bs_flex_member_as(instance_data, Bs_File);
    if (f->file && fileno(f->file) > 2) {
        fclose(f->file);
        memset(f, 0, sizeof(*f));
    }
}

static Bs_Value bs_io_file_close(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    Bs_C_Instance *this = (Bs_C_Instance *) args[-1].as.object;
    if (!bs_flex_member_as(this->data, Bs_File).file) {
        bs_error(bs, "cannot close already closed file");
    }

    bs_io_file_free(bs_config(bs)->userdata, this->data);
    return bs_value_nil;
}

// Reader
static Bs_C_Class *bs_io_reader_class;

static Bs_Value bs_io_reader_init(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 1 && arity != 2) {
        bs_error(bs, "expected 1 or 2 arguments, got %zu", arity);
    }

    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
    const Bs_Str *path = (const Bs_Str *) args[0].as.object;

    bool binary = false;
    if (arity == 2) {
        bs_arg_check_value_type(bs, args, 1, BS_VALUE_BOOL);
        binary = args[1].as.boolean;
    }

    FILE *file = fopen(path->data, binary ? "rb" : "r");
    if (!file) {
        return bs_value_nil;
    }

    Bs_File *f = &bs_this_c_instance_data_as(args, Bs_File);
    f->file = file;
    f->seekable = binary;
    return args[-1];
}

static Bs_Value bs_io_reader_read(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity > 1) {
        bs_error(bs, "expected 0 or 1 arguments, got %zu", arity);
    }

    if (arity) {
        bs_arg_check_whole_number(bs, args, 0);
    }

    Bs_File *f = &bs_this_c_instance_data_as(args, Bs_File);
    if (!f->file) {
        bs_error(bs, "cannot read from closed file");
    }

    size_t count = 0;
    if (arity == 1) {
        count = args[0].as.number;
    } else {
        const long start = ftell(f->file);
        if (start == -1) {
            return bs_value_nil;
        }

        if (fseek(f->file, 0, SEEK_END) == -1) {
            return bs_value_nil;
        }

        const long offset = ftell(f->file);
        if (offset == -1) {
            return bs_value_nil;
        }

        if (fseek(f->file, start, SEEK_SET) == -1) {
            return bs_value_nil;
        }

        count = offset;
    }

    Bs_Buffer *b = &bs_config(bs)->buffer;
    bs_da_push_many(bs, b, NULL, count);

    count = fread(b->data + b->count, sizeof(char), count, f->file);

    Bs_Value result = bs_value_nil;
    if (!ferror(f->file)) {
        result = bs_value_object(bs_str_new(bs, (Bs_Sv) {b->data + b->count, count}));
    }

    return result;
}

static Bs_Value bs_io_reader_readln(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Bs_File *f = &bs_this_c_instance_data_as(args, Bs_File);
    if (!f->file) {
        bs_error(bs, "cannot read from closed file");
    }

    Bs_Buffer   *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    while (true) {
        const char c = fgetc(f->file);
        if (c == '\n' || c == EOF) {
            break;
        }
        bs_da_push(bs, b, c);
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

static Bs_Value bs_io_reader_eof(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Bs_File *f = &bs_this_c_instance_data_as(args, Bs_File);
    return bs_value_bool(!f->file || feof(f->file));
}

static Bs_Value bs_io_reader_seek(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_whole_number(bs, args, 0);
    bs_arg_check_whole_number(bs, args, 1);

    Bs_File *f = &bs_this_c_instance_data_as(args, Bs_File);
    if (!f->file) {
        bs_error(bs, "cannot seek in closed file");
    }

    if (!f->seekable) {
        bs_error(bs, "cannot seek in pipe or non binary file");
    }

    const int whence = args[1].as.number;
    if (whence > 2) {
        bs_error_at(bs, 2, "invalid whence '%d'", whence);
    }

    return bs_value_bool(fseek(f->file, args[0].as.number, whence) != -1);
}

static Bs_Value bs_io_reader_tell(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    Bs_File *f = &bs_this_c_instance_data_as(args, Bs_File);
    if (!f->file) {
        bs_error(bs, "cannot get position of closed file");
    }

    if (!f->seekable) {
        bs_error(bs, "cannot get position of pipe or non binary file");
    }

    const long position = ftell(f->file);
    return position == -1 ? bs_value_nil : bs_value_num(position);
}

// Writer
static Bs_C_Class *bs_io_writer_class;

static Bs_Value bs_io_writer_init(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 1 && arity != 2) {
        bs_error(bs, "expected 1 or 2 arguments, got %zu", arity);
    }

    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
    const Bs_Str *path = (const Bs_Str *) args[0].as.object;

    bool binary = false;
    if (arity == 2) {
        bs_arg_check_value_type(bs, args, 1, BS_VALUE_BOOL);
        binary = args[1].as.boolean;
    }

    FILE *file = fopen(path->data, binary ? "wb" : "w");
    if (!file) {
        return bs_value_nil;
    }

    Bs_File *f = &bs_this_c_instance_data_as(args, Bs_File);
    f->file = file;
    return args[-1];
}

static Bs_Value bs_io_writer_flush(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    Bs_File *f = &bs_this_c_instance_data_as(args, Bs_File);
    if (!f->file) {
        bs_error(bs, "cannot flush closed file");
    }

    fflush(f->file);
    return bs_value_nil;
}

static Bs_Value bs_io_writer_write(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_File *f = &bs_this_c_instance_data_as(args, Bs_File);
    if (!f->file) {
        bs_error(bs, "cannot write into closed file");
    }

    Bs_Writer w = bs_file_writer(f->file);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(&w, " ");
        }
        bs_value_write(bs, &w, args[i]);
    }

    return bs_value_bool(!ferror(f->file));
}

static Bs_Value bs_io_writer_writeln(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_File *f = &bs_this_c_instance_data_as(args, Bs_File);
    if (!f->file) {
        bs_error(bs, "cannot write into closed file");
    }

    Bs_Writer w = bs_file_writer(f->file);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(&w, " ");
        }
        bs_value_write(bs, &w, args[i]);
    }
    bs_fmt(&w, "\n");

    return bs_value_bool(!ferror(f->file));
}

// DirEntry
typedef struct {
    Bs_Str *name;
    bool    isdir;
} Bs_Dir_Entry;

static Bs_C_Class *bs_io_direntry_class;

static void bs_io_direntry_mark(Bs *bs, void *instance_data) {
    Bs_Dir_Entry *e = &bs_flex_member_as(instance_data, Bs_Dir_Entry);
    bs_mark(bs, (Bs_Object *) e->name);
}

static Bs_Value bs_io_direntry_init(Bs *bs, Bs_Value *args, size_t arity) {
    bs_error(bs, "not supposed to be used directly");
    return bs_value_nil;
}

static Bs_Value bs_io_direntry_name(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Bs_Dir_Entry *e = &bs_this_c_instance_data_as(args, Bs_Dir_Entry);
    return bs_value_object(e->name);
}

static Bs_Value bs_io_direntry_isdir(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Bs_Dir_Entry *e = &bs_this_c_instance_data_as(args, Bs_Dir_Entry);
    return bs_value_bool(e->isdir);
}

// IO Functions
static Bs_Value bs_io_input(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity > 1) {
        bs_error(bs, "expected 0 or 1 arguments, got %zu", arity);
    }

    if (arity) {
        bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
        printf(Bs_Sv_Fmt, Bs_Sv_Arg(*(const Bs_Str *) args[0].as.object));
        fflush(stdout);
    }

    Bs_Buffer   *b = &bs_config(bs)->buffer;
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

static Bs_Value bs_io_print(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer w = bs_file_writer(stdout);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(&w, " ");
        }
        bs_value_write(bs, &w, args[i]);
    }
    return bs_value_nil;
}

static Bs_Value bs_io_eprint(Bs *bs, Bs_Value *args, size_t arity) {
    Bs_Writer w = bs_file_writer(stderr);
    for (size_t i = 0; i < arity; i++) {
        if (i) {
            bs_fmt(&w, " ");
        }
        bs_value_write(bs, &w, args[i]);
    }
    return bs_value_nil;
}

static Bs_Value bs_io_println(Bs *bs, Bs_Value *args, size_t arity) {
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

static Bs_Value bs_io_eprintln(Bs *bs, Bs_Value *args, size_t arity) {
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

static Bs_Value bs_io_readdir(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *path = (const Bs_Str *) args[0].as.object;
    Bs_Array     *a = bs_array_new(bs);

#if defined(_WIN32) || defined(_WIN64)
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", path->data);

    WIN32_FIND_DATA findFileData;
    HANDLE          hFind = FindFirstFile(searchPath, &findFileData);
    do {
        if (hFind == INVALID_HANDLE_VALUE) {
            return bs_value_nil;
        }

        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0) {
            continue;
        }

        Bs_C_Instance *instance = bs_c_instance_new(bs, bs_io_direntry_class);

        Bs_Dir_Entry *e = &bs_flex_member_as(instance->data, Bs_Dir_Entry);
        e->name = bs_str_new(bs, bs_sv_from_cstr(findFileData.cFileName));
        e->isdir = (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        bs_array_set(bs, a, a->count, bs_value_object(instance));
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
#else
    DIR *dir = opendir(path->data);
    if (!dir) {
        return bs_value_nil;
    }

    errno = 0;
    struct dirent *entry = NULL;
    while ((entry = readdir(dir))) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }

        Bs_C_Instance *instance = bs_c_instance_new(bs, bs_io_direntry_class);

        Bs_Dir_Entry *e = &bs_flex_member_as(instance->data, Bs_Dir_Entry);
        e->name = bs_str_new(bs, bs_sv_from_cstr(entry->d_name));
        e->isdir = entry->d_type == DT_DIR;
        bs_array_set(bs, a, a->count, bs_value_object(instance));
    }

    if (errno) {
        closedir(dir);
        return bs_value_nil;
    }

    closedir(dir);
#endif

    return bs_value_object(a);
}

static Bs_Value bs_io_readfile(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 1 && arity != 2) {
        bs_error(bs, "expected 1 or 2 arguments, got %zu", arity);
    }

    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
    const Bs_Str *path = (const Bs_Str *) args[0].as.object;

    bool binary = false;
    if (arity == 2) {
        bs_arg_check_value_type(bs, args, 1, BS_VALUE_BOOL);
        binary = args[1].as.boolean;
    }

    size_t size;
    char  *contents = bs_read_file(path->data, &size, binary);
    if (!contents) {
        return bs_value_nil;
    }

    Bs_Str *str = bs_str_new(bs, Bs_Sv(contents, size));

    free(contents);
    return bs_value_object(str);
}

// OS
static Bs_Value bs_os_exit(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

    bs_unwind(bs, args[0].as.number);
    assert(false && "unreachable");
}

static Bs_Value bs_os_clock(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

#if defined(_WIN32) || defined(_WIN64)
    LARGE_INTEGER frequency, counter;
    if (!QueryPerformanceFrequency(&frequency)) {
        bs_error(bs, "could not get clock");
    }

    if (!QueryPerformanceCounter(&counter)) {
        bs_error(bs, "could not get clock");
    }

    return bs_value_num((double) counter.QuadPart / frequency.QuadPart);
#else
    struct timespec clock;
    if (clock_gettime(CLOCK_MONOTONIC, &clock) < 0) {
        bs_error(bs, "could not get clock");
    }

    return bs_value_num(clock.tv_sec + clock.tv_nsec * 1e-9);
#endif // _WIN32
}

static Bs_Value bs_os_sleep(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);

    const double seconds = args[0].as.number;

#if defined(_WIN32) || defined(_WIN64)
    DWORD milliseconds = (DWORD) (seconds * 1000);
    DWORD remaining_microseconds = (DWORD) ((seconds - (milliseconds / 1000.0)) * 1000000);

    Sleep(milliseconds);

    if (remaining_microseconds > 0) {
        LARGE_INTEGER start, end, frequency;
        if (!QueryPerformanceFrequency(&frequency)) {
            bs_error(bs, "could not sleep");
        }

        if (!QueryPerformanceCounter(&start)) {
            bs_error(bs, "could not sleep");
        }

        do {
            if (!QueryPerformanceCounter(&end)) {
                bs_error(bs, "could not sleep");
            }
        } while (((end.QuadPart - start.QuadPart) * 1000000 / frequency.QuadPart) <
                 remaining_microseconds);
    }
#else

    struct timespec req, rem;
    req.tv_sec = (time_t) seconds;
    req.tv_nsec = (long) ((seconds - req.tv_sec) * 1e9);

    while (nanosleep(&req, &rem) == -1) {
        if (errno != EINTR) {
            bs_error(bs, "could not sleep");
        }

        req = rem;
    }
#endif // _WIN32

    return bs_value_nil;
}

static Bs_Value bs_os_getenv(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *name = (const Bs_Str *) args[0].as.object;

#if defined(_WIN32) || defined(_WIN64)
    Bs_Buffer  *b = &bs_config(bs)->buffer;
    const DWORD size = GetEnvironmentVariableA(name->data, NULL, 0);
    if (size) {
        bs_da_push_many(bs, b, NULL, size);
        if (GetEnvironmentVariableA(name->data, b->data + b->count, size)) {
            return bs_value_object(bs_str_new(bs, bs_sv_from_cstr(b->data + b->count)));
        }
    }
#else
    const char *value = getenv(name->data);
    if (value) {
        return bs_value_object(bs_str_new(bs, bs_sv_from_cstr(value)));
    }
#endif // _WIN32

    return bs_value_nil;
}

static Bs_Value bs_os_setenv(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
    bs_arg_check_object_type(bs, args, 1, BS_OBJECT_STR);

    const Bs_Str *key = (const Bs_Str *) args[0].as.object;
    const Bs_Str *value = (const Bs_Str *) args[1].as.object;

#if defined(_WIN32) || defined(_WIN64)
    return bs_value_bool(SetEnvironmentVariable(key->data, value->data) != 0);
#else
    return bs_value_bool(setenv(key->data, value->data, true) == 0);
#endif // _WIN32
}

static Bs_Value bs_os_getcwd(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_object(bs_config(bs)->cwd);
}

static Bs_Value bs_os_setcwd(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *path = (const Bs_Str *) args[0].as.object;

#if defined(_WIN32) || defined(_WIN64)
    const bool ok = SetCurrentDirectory(path->data);
#else
    const bool ok = chdir(path->data) >= 0;
#endif

    if (ok) {
        bs_update_cwd(bs);
    }

    return bs_value_bool(ok);
}

// Process
typedef struct {
#if defined(_WIN32) || defined(_WIN64)
    PROCESS_INFORMATION piProcInfo;
#else
    pid_t pid;
#endif // _WIN32

    Bs_C_Instance *stdout_read;
    Bs_C_Instance *stderr_read;
    Bs_C_Instance *stdin_write;
} Bs_Process;

static void bs_process_mark(Bs *bs, void *instance_data) {
    Bs_Process *p = &bs_flex_member_as(instance_data, Bs_Process);
    bs_mark(bs, (Bs_Object *) p->stdout_read);
    bs_mark(bs, (Bs_Object *) p->stderr_read);
    bs_mark(bs, (Bs_Object *) p->stdin_write);
}

static Bs_C_Instance *bs_pipe_new(Bs *bs, int fd, bool write, bool binary) {
    Bs_C_Instance *instance =
        bs_c_instance_new(bs, write ? bs_io_writer_class : bs_io_reader_class);

    Bs_File *f = &bs_flex_member_as(instance->data, Bs_File);
    f->file = FD_OPEN(fd, write ? "w" : "r");

#if defined(_WIN32) || defined(_WIN64)
    setmode(fd, binary ? _O_BINARY : _O_TEXT);
#endif // _WIN32

    return instance;
}

static Bs_Value bs_process_init(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity < 1 || arity > 5) {
        bs_error(bs, "expected 1 to 5 arguments, got %zu", arity);
    }
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_ARRAY);

    bool capture_stdout = false;
    bool capture_stderr = false;
    bool capture_stdin = false;
    bool capture_binary = false;

    if (arity >= 2) {
        bs_arg_check_value_type(bs, args, 1, BS_VALUE_BOOL);
        capture_stdout = args[1].as.boolean;
    }

    if (arity >= 3) {
        bs_arg_check_value_type(bs, args, 2, BS_VALUE_BOOL);
        capture_stderr = args[2].as.boolean;
    }

    if (arity == 4) {
        bs_arg_check_value_type(bs, args, 3, BS_VALUE_BOOL);
        capture_stdin = args[3].as.boolean;
    }

    if (arity == 5) {
        bs_arg_check_value_type(bs, args, 4, BS_VALUE_BOOL);
        capture_binary = args[4].as.boolean;
    }

    const Bs_Array *array = (const Bs_Array *) args[0].as.object;
    if (!array->count) {
        bs_error(bs, "cannot execute empty array as process");
    }

    for (size_t i = 0; i < array->count; i++) {
        char      buffer[64];
        const int count = snprintf(buffer, sizeof(buffer), "command string #%zu", i + 1);
        assert(count >= 0 && count + 1 < sizeof(buffer));
        bs_check_object_type_at(bs, 1, array->data[i], BS_OBJECT_STR, buffer);
    }

    Bs_Process *p = &bs_this_c_instance_data_as(args, Bs_Process);

#if defined(_WIN32) || defined(_WIN64)
    STARTUPINFOA siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE stdout_pipe_read = NULL, stdout_pipe_write = NULL;
    if (capture_stdout) {
        if (!CreatePipe(&stdout_pipe_read, &stdout_pipe_write, &saAttr, 0)) {
            bs_error(bs, "could not capture stdout");
        }
        SetHandleInformation(stdout_pipe_read, HANDLE_FLAG_INHERIT, 0);
    } else {
        stdout_pipe_write = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    HANDLE stderr_pipe_read = NULL, stderr_pipe_write = NULL;
    if (capture_stderr) {
        if (!CreatePipe(&stderr_pipe_read, &stderr_pipe_write, &saAttr, 0)) {
            bs_error(bs, "could not capture stderr");
        }
        SetHandleInformation(stderr_pipe_read, HANDLE_FLAG_INHERIT, 0);
    } else {
        stderr_pipe_write = GetStdHandle(STD_ERROR_HANDLE);
    }

    HANDLE stdin_pipe_read = NULL, stdin_pipe_write = NULL;
    if (capture_stdin) {
        if (!CreatePipe(&stdin_pipe_read, &stdin_pipe_write, &saAttr, 0)) {
            bs_error(bs, "could not capture stdin");
        }
        SetHandleInformation(stdin_pipe_write, HANDLE_FLAG_INHERIT, 0);
    } else {
        stdin_pipe_read = GetStdHandle(STD_INPUT_HANDLE);
    }

    siStartInfo.hStdOutput = stdout_pipe_write;
    siStartInfo.hStdError = stderr_pipe_write;
    siStartInfo.hStdInput = stdin_pipe_read;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&p->piProcInfo, sizeof(PROCESS_INFORMATION));

    Bs_Buffer   *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    for (size_t i = 0; i < array->count; i++) {
        if (i) {
            bs_da_push(bs, b, ' ');
        }

        const Bs_Str *it_str = (const Bs_Str *) array->data[i].as.object;
        Bs_Sv         it = Bs_Sv(it_str->data, it_str->size);

        const bool need_quoting = it.size == 0 || bs_sv_find(it, '\t', NULL) ||
                                  bs_sv_find(it, '\v', NULL) || bs_sv_find(it, ' ', NULL);

        if (need_quoting) {
            bs_da_push(bs, b, '"');
        }

        for (size_t j = 0; j < it.size; j++) {
            switch (it.data[j]) {
            default:
                break;

            case '\\':
                if (j + 1 < it.size && it.data[j + 1] == '"') {
                    bs_da_push(bs, b, '\\');
                }
                break;

            case '"':
                bs_da_push(bs, b, '\\');
                break;
            }

            if (!i && it.data[j] == '/') {
                bs_da_push(bs, b, '\\');
            } else {
                bs_da_push(bs, b, it.data[j]);
            }
        }

        if (need_quoting) {
            bs_da_push(bs, b, '"');
        }
    }
    bs_da_push(bs, b, '\0');

    char *cmdline = b->data + start;
    b->count = start;

    if (!CreateProcessA(
            NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &p->piProcInfo)) {
        return bs_value_nil;
    }

    CloseHandle(p->piProcInfo.hThread);

    if (capture_stdout) {
        p->stdout_read = bs_pipe_new(
            bs, _open_osfhandle((intptr_t) stdout_pipe_read, _O_RDONLY), false, capture_binary);
        CloseHandle(stdout_pipe_write);
    }

    if (capture_stderr) {
        p->stderr_read = bs_pipe_new(
            bs, _open_osfhandle((intptr_t) stderr_pipe_read, _O_RDONLY), false, capture_binary);
        CloseHandle(stderr_pipe_write);
    }

    if (capture_stdin) {
        p->stdin_write = bs_pipe_new(
            bs, _open_osfhandle((intptr_t) stdin_pipe_write, _O_WRONLY), true, capture_binary);
        CloseHandle(stdin_pipe_read);
    }
#else
    int fail[2];
    if (pipe(fail) < 0 || fcntl(fail[1], F_SETFD, FD_CLOEXEC) < 0) {
        bs_error(bs, "could not create failure pipe");
    }

    int stdout_pipe[2];
    if (capture_stdout && pipe(stdout_pipe) < 0) {
        bs_error(bs, "could not capture stdout");
    }

    int stderr_pipe[2];
    if (capture_stderr && pipe(stderr_pipe) < 0) {
        bs_error(bs, "could not capture stderr");
    }

    int stdin_pipe[2];
    if (capture_stdin && pipe(stdin_pipe) < 0) {
        bs_error(bs, "could not capture stdin");
    }

    const pid_t pid = fork();
    if (pid < 0) {
        close(fail[0]);
        close(fail[1]);

        if (capture_stdout) {
            close(stdout_pipe[0]);
            close(stdout_pipe[1]);
        }

        if (capture_stderr) {
            close(stderr_pipe[0]);
            close(stderr_pipe[1]);
        }

        if (capture_stdin) {
            close(stdin_pipe[0]);
            close(stdin_pipe[1]);
        }

        bs_error(bs, "could not fork process");
    }

    if (pid == 0) {
        char **cargv = malloc((array->count + 1) * sizeof(char *));
        assert(cargv);

        for (size_t i = 0; i < array->count; i++) {
            Bs_Str *str = (Bs_Str *) array->data[i].as.object;
            cargv[i] = str->data;
        }

        cargv[array->count] = NULL;

        if (capture_stdout) {
            close(stdout_pipe[0]);
            dup2(stdout_pipe[1], STDOUT_FILENO);
            close(stdout_pipe[1]);
        }

        if (capture_stderr) {
            close(stderr_pipe[0]);
            dup2(stderr_pipe[1], STDERR_FILENO);
            close(stderr_pipe[1]);
        }

        if (capture_stdin) {
            close(stdin_pipe[1]);
            dup2(stdin_pipe[0], STDIN_FILENO);
            close(stdin_pipe[0]);
        }

        close(fail[0]);
        execvp(*cargv, (char *const *) cargv);
        write(fail[1], "F", 1);
        close(fail[1]);
        exit(127);
    }

    close(fail[1]);
    char       buffer[1];
    const long count = read(fail[0], buffer, sizeof(buffer));
    close(fail[0]);

    if (count > 0) {
        waitpid(pid, NULL, 0); // Wait for the child to kill itself so it doesn't become a zombie
        return bs_value_nil;
    }

    p->pid = pid;

    if (capture_stdout) {
        close(stdout_pipe[1]);
        p->stdout_read = bs_pipe_new(bs, stdout_pipe[0], false, capture_binary);
    }

    if (capture_stderr) {
        close(stderr_pipe[1]);
        p->stderr_read = bs_pipe_new(bs, stderr_pipe[0], false, capture_binary);
    }

    if (capture_stdin) {
        close(stdin_pipe[0]);
        p->stdin_write = bs_pipe_new(bs, stdin_pipe[1], true, capture_binary);
    }
#endif // _WIN32

    return args[-1];
}

static Bs_Value bs_process_kill(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

    Bs_Process *p = &bs_this_c_instance_data_as(args, Bs_Process);

#if defined(_WIN32) || defined(_WIN64)
    if (!TerminateProcess(p->piProcInfo.hProcess, 1)) {
        return bs_value_bool(false);
    }
#else
    if (!p->pid || kill(p->pid, args[0].as.number) < 0) {
        return bs_value_bool(false);
    }

    p->pid = 0;
#endif // _WIN32

    p->stdout_read = NULL;
    p->stderr_read = NULL;
    p->stdin_write = NULL;
    return bs_value_bool(true);
}

static Bs_Value bs_process_wait(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    Bs_Process *p = &bs_this_c_instance_data_as(args, Bs_Process);

#if defined(_WIN32) || defined(_WIN64)
    if (WaitForSingleObject(p->piProcInfo.hProcess, INFINITE) == WAIT_FAILED) {
        return bs_value_nil;
    }

    DWORD exit_code;
    if (!GetExitCodeProcess(p->piProcInfo.hProcess, &exit_code)) {
        return bs_value_nil;
    }

    CloseHandle(p->piProcInfo.hProcess);
    const Bs_Value return_value = bs_value_num(exit_code);
#else
    if (!p->pid) {
        return bs_value_nil;
    }

    int status;
    if (waitpid(p->pid, &status, 0) < 0) {
        return bs_value_nil;
    }

    p->pid = 0;

    const Bs_Value return_value =
        WIFEXITED(status) ? bs_value_num(WEXITSTATUS(status)) : bs_value_nil;
#endif // _WIN32

    p->stdout_read = NULL;
    p->stderr_read = NULL;
    p->stdin_write = NULL;
    return return_value;
}

static Bs_Value bs_process_stdout(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Bs_Process *p = &bs_this_c_instance_data_as(args, Bs_Process);
    return p->stdout_read ? bs_value_object(p->stdout_read) : bs_value_nil;
}

static Bs_Value bs_process_stderr(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Bs_Process *p = &bs_this_c_instance_data_as(args, Bs_Process);
    return p->stderr_read ? bs_value_object(p->stderr_read) : bs_value_nil;
}

static Bs_Value bs_process_stdin(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Bs_Process *p = &bs_this_c_instance_data_as(args, Bs_Process);
    return p->stdin_write ? bs_value_object(p->stdin_write) : bs_value_nil;
}

// Regex
typedef struct {
    regex_t regex;
} Bs_Regex;

static Bs_C_Class *bs_regex_class;

static void bs_regex_free(void *userdata, void *instance_data) {
    regfree(&bs_flex_member_as(instance_data, regex_t));
}

static Bs_Value bs_regex_init(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *pattern = (const Bs_Str *) args[0].as.object;

    regex_t regex;
    if (regcomp(&regex, pattern->data, REG_EXTENDED)) {
        return bs_value_nil;
    }

    bs_this_c_instance_data_as(args, regex_t) = regex;
    return args[-1];
}

// Str
static Bs_Value bs_str_slice(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 1 && arity != 2) {
        bs_error(bs, "expected 1 or 2 arguments, got %zu", arity);
    }

    bs_arg_check_whole_number(bs, args, 0);
    if (arity == 2) {
        bs_arg_check_whole_number(bs, args, 1);
    }

    const Bs_Str *str = (const Bs_Str *) args[-1].as.object;
    const size_t  begin = args[0].as.number;
    const size_t  end = (arity == 2) ? args[1].as.number : str->size;

    if (begin == end) {
        return bs_value_object(bs_str_new(bs, Bs_Sv_Static("")));
    }

    if (begin >= str->size || end > str->size) {
        bs_error(bs, "cannot slice string of length %zu from %zu to %zu", str->size, begin, end);
    }

    return bs_value_object(bs_str_new(bs, Bs_Sv(str->data + begin, end - begin)));
}

static Bs_Value bs_str_reverse(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    const Bs_Str *src = (const Bs_Str *) args[-1].as.object;

    Bs_Buffer   *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    for (size_t i = 0; i < src->size; i++) {
        bs_da_push(bs, b, src->data[src->size - i - 1]);
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

static Bs_Value bs_str_repeat(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

    const Bs_Str *src = (const Bs_Str *) args[-1].as.object;
    const size_t  count = args[0].as.number;

    Bs_Buffer   *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    for (size_t i = 0; i < count; i++) {
        bs_da_push_many(bs, b, src->data, src->size);
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

static Bs_Value bs_str_tolower(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    const Bs_Str *src = (const Bs_Str *) args[-1].as.object;

    Bs_Buffer   *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    for (size_t i = 0; i < src->size; i++) {
        bs_da_push(bs, b, tolower(src->data[i]));
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

static Bs_Value bs_str_toupper(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    const Bs_Str *src = (const Bs_Str *) args[-1].as.object;

    Bs_Buffer   *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    for (size_t i = 0; i < src->size; i++) {
        bs_da_push(bs, b, toupper(src->data[i]));
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

static Bs_Value bs_str_tonumber(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    const Bs_Str *src = (const Bs_Str *) args[-1].as.object;

    char        *end;
    const double value = strtod(src->data, &end);

    if (end == src->data || *end != '\0' || errno == ERANGE) {
        return bs_value_nil;
    }
    return bs_value_num(value);
}

static Bs_Value bs_str_find(Bs *bs, Bs_Value *args, size_t arity) {
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

    const Bs_Str *str = (const Bs_Str *) args[-1].as.object;
    const size_t  offset = arity == 2 ? args[1].as.number : 0;
    if (offset > str->size) {
        bs_error_at(bs, 2, "cannot take offset of %zu in string of length %zu", offset, str->size);
    }

    if (args[0].as.object->type == BS_OBJECT_STR) {
        const Bs_Str *pattern = (const Bs_Str *) args[0].as.object;
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
        const regex_t regex =
            bs_flex_member_as(((Bs_C_Instance *) args[0].as.object)->data, regex_t);

        regmatch_t match;
        if (!regexec(&regex, str->data + offset, 1, &match, 0)) {
            return bs_value_num(offset + match.rm_so);
        }
    }

    return bs_value_nil;
}

static Bs_Value bs_str_split(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);

    const Bs_Check checks[] = {
        bs_check_object(BS_OBJECT_STR),
        bs_check_c_instance(bs_regex_class),
    };
    bs_arg_check_multi(bs, args, 0, checks, bs_c_array_size(checks));

    const Bs_Str *str = (const Bs_Str *) args[-1].as.object;

    Bs_Array *a = bs_array_new(bs);
    size_t    j = 0;

    if (args[0].as.object->type == BS_OBJECT_STR) {
        const Bs_Str *pattern = (const Bs_Str *) args[0].as.object;
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
        const regex_t regex =
            bs_flex_member_as(((Bs_C_Instance *) args[0].as.object)->data, regex_t);

        int        eflags = 0;
        regmatch_t match;
        while (!regexec(&regex, str->data + j, 1, &match, eflags)) {
            eflags = REG_NOTBOL;
            if (match.rm_so == match.rm_eo) {
                break;
            }

            bs_array_set(
                bs,
                a,
                a->count,
                bs_value_object(bs_str_new(bs, Bs_Sv(str->data + j, match.rm_so))));

            j += match.rm_eo;
        }
    }

    if (j != str->size) {
        bs_array_set(
            bs, a, a->count, bs_value_object(bs_str_new(bs, Bs_Sv(str->data + j, str->size - j))));
    }

    return bs_value_object(a);
}

static Bs_Value bs_str_replace(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);

    const Bs_Check checks[] = {
        bs_check_object(BS_OBJECT_STR),
        bs_check_c_instance(bs_regex_class),
    };
    bs_arg_check_multi(bs, args, 0, checks, bs_c_array_size(checks));
    bs_arg_check_object_type(bs, args, 1, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[-1].as.object;
    const Bs_Str *replacement = (const Bs_Str *) args[1].as.object;

    if (args[0].as.object->type == BS_OBJECT_STR) {
        const Bs_Str *pattern = (const Bs_Str *) args[0].as.object;
        if (!pattern->size) {
            return bs_value_object(str);
        }

        if (str->size < pattern->size) {
            return bs_value_object(str);
        }

        Bs_Buffer   *b = &bs_config(bs)->buffer;
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
        Bs_Buffer   *b = &bs_config(bs)->buffer;
        const size_t start = b->count;

        const char   *cursor = str->data;
        const regex_t regex =
            bs_flex_member_as(((Bs_C_Instance *) args[0].as.object)->data, regex_t);

        int        eflags = 0;
        regmatch_t matches[10];
        while (!regexec(&regex, cursor, bs_c_array_size(matches), matches, eflags)) {
            eflags = REG_NOTBOL;
            if (matches[0].rm_so == matches[0].rm_eo) {
                break;
            }

            bs_da_push_many(bs, b, cursor, matches[0].rm_so);

            for (size_t i = 0; i < replacement->size; i++) {
                if (replacement->data[i] == '\\' && isdigit(replacement->data[i + 1])) {
                    const size_t j = replacement->data[++i] - '0';
                    if (j < bs_c_array_size(matches) && matches[j].rm_so != -1) {
                        bs_da_push_many(
                            bs, b, cursor + matches[j].rm_so, matches[j].rm_eo - matches[j].rm_so);
                    }
                } else {
                    bs_da_push(bs, b, replacement->data[i]);
                }
            }

            cursor += matches[0].rm_eo;
        }
        bs_da_push_many(bs, b, cursor, str->size - (cursor - str->data));

        return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
    }
}

static Bs_Value bs_str_compare(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *this = (const Bs_Str *) args[-1].as.object;
    const Bs_Str *that = (const Bs_Str *) args[0].as.object;

    const size_t min = bs_min(this->size, that->size);
    for (size_t i = 0; i < min; i++) {
        if (this->data[i] != that->data[i]) {
            return bs_value_num((this->data[i] < that->data[i]) ? -1 : 1);
        }
    }

    if (this->size < that->size) {
        return bs_value_num(-1);
    } else if (this->size > that->size) {
        return bs_value_num(1);
    }

    return bs_value_num(0);
}

static Bs_Value bs_str_trim(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[-1].as.object;
    const Bs_Str *pattern = (const Bs_Str *) args[0].as.object;
    if (!str->size || !pattern->size) {
        return bs_value_object(str);
    }

    if (pattern->size > str->size) {
        return bs_value_object(str);
    }

    Bs_Sv       str_sv = Bs_Sv(str->data, str->size);
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
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[-1].as.object;
    const Bs_Str *pattern = (const Bs_Str *) args[0].as.object;
    if (!str->size || !pattern->size) {
        return bs_value_object(str);
    }

    if (pattern->size > str->size) {
        return bs_value_object(str);
    }

    Bs_Sv       str_sv = Bs_Sv(str->data, str->size);
    const Bs_Sv pattern_sv = Bs_Sv(pattern->data, pattern->size);
    while (bs_sv_prefix(str_sv, pattern_sv)) {
        str_sv.data += pattern_sv.size;
        str_sv.size -= pattern_sv.size;
    }

    return bs_value_object(bs_str_new(bs, str_sv));
}

static Bs_Value bs_str_rtrim(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[-1].as.object;
    const Bs_Str *pattern = (const Bs_Str *) args[0].as.object;
    if (!str->size || !pattern->size) {
        return bs_value_object(str);
    }

    if (pattern->size > str->size) {
        return bs_value_object(str);
    }

    Bs_Sv       str_sv = Bs_Sv(str->data, str->size);
    const Bs_Sv pattern_sv = Bs_Sv(pattern->data, pattern->size);
    while (bs_sv_suffix(str_sv, pattern_sv)) {
        str_sv.size -= pattern_sv.size;
    }

    return bs_value_object(bs_str_new(bs, str_sv));
}

static Bs_Value bs_str_lpad(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
    bs_arg_check_whole_number(bs, args, 1);

    const Bs_Str *str = (const Bs_Str *) args[-1].as.object;
    const Bs_Str *pattern = (const Bs_Str *) args[0].as.object;
    const size_t  count = args[1].as.number;
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

    Bs_Buffer   *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    for (size_t i = 0; i < padding; i += pattern->size) {
        bs_da_push_many(bs, b, pattern->data, bs_min(pattern->size, padding - i));
    }
    bs_da_push_many(bs, b, str->data, str->size);

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

static Bs_Value bs_str_rpad(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
    bs_arg_check_whole_number(bs, args, 1);

    const Bs_Str *str = (const Bs_Str *) args[-1].as.object;
    const Bs_Str *pattern = (const Bs_Str *) args[0].as.object;
    const size_t  count = args[1].as.number;
    if (!str->size || !pattern->size) {
        return bs_value_object(str);
    }

    if (pattern->size > str->size) {
        return bs_value_object(str);
    }

    if (str->size >= count) {
        return bs_value_object(str);
    }

    Bs_Buffer   *b = &bs_config(bs)->buffer;
    const size_t start = b->count;

    bs_da_push_many(bs, b, str->data, str->size);
    for (size_t i = str->size; i < count; i += pattern->size) {
        bs_da_push_many(bs, b, pattern->data, bs_min(pattern->size, count - i));
    }

    return bs_value_object(bs_str_new(bs, bs_buffer_reset(b, start)));
}

static Bs_Value bs_str_prefix(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[-1].as.object;
    const Bs_Str *pattern = (const Bs_Str *) args[0].as.object;

    return bs_value_bool(
        bs_sv_prefix(Bs_Sv(str->data, str->size), Bs_Sv(pattern->data, pattern->size)));
}

static Bs_Value bs_str_suffix(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[-1].as.object;
    const Bs_Str *pattern = (const Bs_Str *) args[0].as.object;

    return bs_value_bool(
        bs_sv_suffix(Bs_Sv(str->data, str->size), Bs_Sv(pattern->data, pattern->size)));
}

// Bit
static Bs_Value bs_bit_ceil(Bs *bs, Bs_Value *args, size_t arity) {
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

static Bs_Value bs_bit_floor(Bs *bs, Bs_Value *args, size_t arity) {
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
static Bs_Value bs_ascii_char(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_ascii_code(bs, args, 0);

    const char ch = args[0].as.number;
    return bs_value_object(bs_str_new(bs, Bs_Sv(&ch, 1)));
}

static Bs_Value bs_ascii_code(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_num(*str->data);
}

static Bs_Value bs_ascii_isalnum(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isalnum(*str->data));
}

static Bs_Value bs_ascii_isalpha(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isalpha(*str->data));
}

static Bs_Value bs_ascii_iscntrl(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(iscntrl(*str->data));
}

static Bs_Value bs_ascii_isdigit(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isdigit(*str->data));
}

static Bs_Value bs_ascii_islower(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(islower(*str->data));
}

static Bs_Value bs_ascii_isgraph(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isgraph(*str->data));
}

static Bs_Value bs_ascii_isprint(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isprint(*str->data));
}

static Bs_Value bs_ascii_ispunct(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(ispunct(*str->data));
}

static Bs_Value bs_ascii_isspace(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isspace(*str->data));
}

static Bs_Value bs_ascii_isupper(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *str = (const Bs_Str *) args[0].as.object;
    if (str->size != 1) {
        bs_error_at(bs, 1, "expected string of length 1, got %zu", str->size);
    }

    return bs_value_bool(isupper(*str->data));
}

// Bytes
static Bs_C_Class *bs_bytes_class;

static void bs_bytes_free(void *userdata, void *instance_data) {
    Bs_Buffer *b = &bs_flex_member_as(instance_data, Bs_Buffer);
    if (b->bs) {
        bs_da_free(b->bs, b);
    }
}

static void bs_bytes_show(Bs_Pretty_Printer *p, const void *instance_data) {
    const Bs_Buffer *b = &bs_flex_member_as(instance_data, Bs_Buffer);
    const Bs_Sv      sv = Bs_Sv(b->data, b->count);
    if (p->depth) {
        bs_pretty_printer_quote(p, sv);
    } else {
        p->writer->write(p->writer, sv);
    }
}

static Bs_Value bs_bytes_init(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity > 1) {
        bs_error(bs, "expected 0 or 1 arguments, got %zu", arity);
    }
    Bs_Buffer *b = &bs_this_c_instance_data_as(args, Bs_Buffer);

    if (arity) {
        bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
        const Bs_Str *src = (const Bs_Str *) args[0].as.object;
        bs_da_push_many(bs, b, src->data, src->size);
    }

    b->bs = bs;
    return bs_value_nil;
}

static Bs_Value bs_bytes_count(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    const Bs_Buffer *b = &bs_this_c_instance_data_as(args, Bs_Buffer);
    return bs_value_num(b->count);
}

static Bs_Value bs_bytes_reset(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

    Bs_Buffer   *b = &bs_this_c_instance_data_as(args, Bs_Buffer);
    const size_t reset = args[0].as.number;

    if (reset > b->count) {
        bs_error(bs, "cannot reset Bytes of length %zu to %zu", b->count, reset);
    }

    b->count = reset;
    return bs_value_nil;
}

static Bs_Value bs_bytes_slice(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 0 && arity != 2) {
        bs_error(bs, "expected 0 or 2 arguments, got %zu", arity);
    }

    if (arity) {
        bs_arg_check_whole_number(bs, args, 0);
        bs_arg_check_whole_number(bs, args, 1);
    }

    const Bs_Buffer *b = &bs_this_c_instance_data_as(args, Bs_Buffer);
    const size_t     begin = arity ? bs_min(args[0].as.number, args[1].as.number) : 0;
    const size_t     end = arity ? bs_max(args[0].as.number, args[1].as.number) : b->count;

    if (begin == end) {
        return bs_value_object(bs_str_new(bs, Bs_Sv_Static("")));
    }

    if (begin >= b->count || end > b->count) {
        bs_error(bs, "cannot slice Bytes of length %zu from %zu to %zu", b->count, begin, end);
    }

    return bs_value_object(bs_str_new(bs, Bs_Sv(b->data + begin, end - begin)));
}

static Bs_Value bs_bytes_push(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);

    const Bs_Check checks[] = {
        bs_check_object(BS_OBJECT_STR),
        bs_check_c_instance(bs_bytes_class),
        bs_check_ascii,
    };
    bs_arg_check_multi(bs, args, 0, checks, bs_c_array_size(checks));

    Bs_Buffer *b = &bs_this_c_instance_data_as(args, Bs_Buffer);
    if (args[0].type == BS_VALUE_NUM) {
        bs_da_push(bs, b, (char) args[0].as.number);
    } else if (args[0].as.object->type == BS_OBJECT_STR) {
        const Bs_Str *src = (const Bs_Str *) args[0].as.object;
        bs_da_push_many(bs, b, src->data, src->size);
    } else if (args[0].as.object->type == BS_OBJECT_C_INSTANCE) {
        const Bs_Buffer *s =
            &bs_flex_member_as(((Bs_C_Instance *) args[0].as.object)->data, Bs_Buffer);
        bs_da_push_many(bs, b, s->data, s->count);
    }

    return args[-1];
}

static Bs_Value bs_bytes_insert(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_whole_number(bs, args, 0);

    const Bs_Check checks[] = {
        bs_check_object(BS_OBJECT_STR),
        bs_check_c_instance(bs_bytes_class),
        bs_check_ascii,
    };
    bs_arg_check_multi(bs, args, 1, checks, bs_c_array_size(checks));

    Bs_Buffer   *b = &bs_this_c_instance_data_as(args, Bs_Buffer);
    const size_t index = args[0].as.number;

    if (index > b->count) {
        bs_error(bs, "cannot insert at %zu into Bytes of length %zu", index, b->count);
    }

    char        temp = 0;
    size_t      size = sizeof(temp);
    const char *data = &temp;
    if (args[1].type == BS_VALUE_NUM) {
        temp = args[1].as.number;
    } else if (args[1].as.object->type == BS_OBJECT_STR) {
        const Bs_Str *src = (const Bs_Str *) args[1].as.object;
        data = src->data;
        size = src->size;
    } else if (args[1].as.object->type == BS_OBJECT_C_INSTANCE) {
        const Bs_Buffer *s =
            &bs_flex_member_as(((Bs_C_Instance *) args[1].as.object)->data, Bs_Buffer);
        data = s->data;
        size = s->count;
    }

    bs_da_push_many(bs, b, NULL, size);
    memmove(b->data + index + size, b->data + index, b->count - index);
    memcpy(b->data + index, data, size);
    b->count += size;

    return args[-1];
}

static Bs_Value bs_bytes_get(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

    const Bs_Buffer *b = &bs_this_c_instance_data_as(args, Bs_Buffer);
    const size_t     index = args[0].as.number;

    if (index >= b->count) {
        bs_error(bs, "cannot get byte at index %zu in Bytes of length %zu", index, b->count);
    }

    return bs_value_num(b->data[index]);
}

static Bs_Value bs_bytes_set(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_whole_number(bs, args, 0);
    bs_arg_check_ascii_code(bs, args, 1);

    Bs_Buffer   *b = &bs_this_c_instance_data_as(args, Bs_Buffer);
    const size_t index = args[0].as.number;
    const char   code = args[1].as.number;

    if (index >= b->count) {
        bs_error(bs, "cannot set byte at index %zu in Bytes of length %zu", index, b->count);
    }

    b->data[index] = (char) code;
    return bs_value_nil;
}

// Array
static Bs_Value bs_array_map(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_callable(bs, args, 0);

    const Bs_Array *src = (const Bs_Array *) args[-1].as.object;
    const Bs_Value  fn = args[0];
    Bs_Array       *dst = bs_array_new(bs);

    for (size_t i = 0; i < src->count; i++) {
        const Bs_Value input = src->data[i];
        const Bs_Value output = bs_call(bs, fn, &input, 1);
        bs_array_set(bs, dst, i, output);
    }

    return bs_value_object(dst);
}

static Bs_Value bs_array_filter(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_callable(bs, args, 0);

    const Bs_Array *src = (const Bs_Array *) args[-1].as.object;
    const Bs_Value  fn = args[0];
    Bs_Array       *dst = bs_array_new(bs);

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
    if (arity != 1 && arity != 2) {
        bs_error(bs, "expected 1 or 2 arguments, got %zu", arity);
    }

    bs_arg_check_callable(bs, args, 0);

    const Bs_Array *src = (const Bs_Array *) args[-1].as.object;
    const Bs_Value  fn = args[0];
    Bs_Value        acc = arity == 2 ? args[1] : bs_value_nil;

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
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    Bs_Buffer   *b = &bs_config(bs)->buffer;
    const size_t start = b->count;
    {
        Bs_Writer w = bs_buffer_writer(b);

        const Bs_Array *array = (const Bs_Array *) args[-1].as.object;
        const Bs_Str   *str = (const Bs_Str *) args[0].as.object;
        const Bs_Sv     separator = Bs_Sv(str->data, str->size);

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
    if (arity != 1 && arity != 2) {
        bs_error(bs, "expected 1 or 2 arguments, got %zu", arity);
    }

    if (arity == 2) {
        bs_arg_check_whole_number(bs, args, 1);
    }

    const Bs_Array *array = (const Bs_Array *) args[-1].as.object;
    const Bs_Value  pred = args[0];
    const size_t    offset = arity == 2 ? args[1].as.number : 0;

    for (size_t i = offset; i < array->count; i++) {
        if (bs_value_equal(array->data[i], pred)) {
            return bs_value_num(i);
        }
    }

    return bs_value_nil;
}

static Bs_Value bs_array_push(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    Bs_Array *a = (Bs_Array *) args[-1].as.object;
    bs_array_set(bs, a, a->count, args[0]);
    return args[-1];
}

static Bs_Value bs_array_insert(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_whole_number(bs, args, 0);

    Bs_Array      *a = (Bs_Array *) args[-1].as.object;
    const size_t   index = args[0].as.number;
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
    return args[-1];
}

static Bs_Value bs_array_pop(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Bs_Array *a = (Bs_Array *) args[-1].as.object;
    if (!a->count) {
        bs_error(bs, "cannot pop from empty array");
    }

    return a->data[--a->count];
}

typedef struct {
    Bs      *bs;
    Bs_Value fn;
} Bs_Array_Sort_Context;

static Bs_Array_Sort_Context sort_context;

static int bs_array_sort_compare(const void *a, const void *b) {
    const Bs_Value args[] = {
        *(const Bs_Value *) a,
        *(const Bs_Value *) b,
    };

    const Bs_Value output = bs_call(sort_context.bs, sort_context.fn, args, bs_c_array_size(args));
    return bs_value_is_falsey(output) ? 1 : -1;
}

static Bs_Value bs_array_sort(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_callable(bs, args, 0);

    sort_context.bs = bs;
    sort_context.fn = args[0];

    Bs_Array *src = (Bs_Array *) args[-1].as.object;
    qsort(src->data, src->count, sizeof(*src->data), bs_array_sort_compare);
    return args[-1];
}

static Bs_Value bs_array_resize(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

    Bs_Array    *src = (Bs_Array *) args[-1].as.object;
    const size_t size = args[0].as.number;
    if (size > src->count) {
        bs_array_set(bs, src, size - 1, bs_value_nil);
    } else {
        src->count = size;
    }
    return args[-1];
}

static Bs_Value bs_array_reverse(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);

    Bs_Array *src = (Bs_Array *) args[-1].as.object;
    for (size_t i = 0; i < src->count / 2; i++) {
        const Bs_Value t = src->data[i];
        src->data[i] = src->data[src->count - i - 1];
        src->data[src->count - i - 1] = t;
    }
    return args[-1];
}

static Bs_Value bs_array_fill(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);

    Bs_Array *src = (Bs_Array *) args[-1].as.object;
    for (size_t i = 0; i < src->count; i++) {
        src->data[i] = args[0];
    }
    return args[-1];
}

static Bs_Value bs_array_slice(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 1 && arity != 2) {
        bs_error(bs, "expected 1 or 2 arguments, got %zu", arity);
    }

    bs_arg_check_whole_number(bs, args, 0);
    if (arity == 2) {
        bs_arg_check_whole_number(bs, args, 1);
    }

    const Bs_Array *src = (const Bs_Array *) args[-1].as.object;
    const size_t    begin = args[0].as.number;
    const size_t    end = (arity == 2) ? args[1].as.number : src->count;

    Bs_Array *dst = bs_array_new(bs);
    if (begin == end) {
        return bs_value_object(dst);
    }

    if (begin >= src->count || end > src->count) {
        bs_error(bs, "cannot slice array of length %zu from %zu to %zu", src->count, begin, end);
    }

    for (size_t i = begin; i < end; i++) {
        bs_array_set(bs, dst, i - begin, src->data[i]);
    }
    return bs_value_object(dst);
}

static Bs_Value bs_array_append(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_ARRAY);

    Bs_Array       *dst = (Bs_Array *) args[-1].as.object;
    const Bs_Array *src = (const Bs_Array *) args[0].as.object;

    for (size_t i = 0; i < src->count; i++) {
        bs_array_set(bs, dst, dst->count, src->data[i]);
    }
    return args[-1];
}

// Table
static Bs_Value bs_table_extend(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_TABLE);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_BOOL);

    Bs_Table       *dst = (Bs_Table *) args[-1].as.object;
    const Bs_Table *src = (const Bs_Table *) args[0].as.object;
    const bool      overwrite = args[1].as.object;

    for (size_t i = 0; i < src->map.capacity; i++) {
        const Bs_Entry *entry = &src->map.data[i];
        if (entry->key.type != BS_VALUE_NIL) {
            if (bs_map_get(bs, &dst->map, entry->key, NULL) && !overwrite) {
                continue;
            }

            bs_map_set(bs, &dst->map, entry->key, entry->value);
        }
    }

    return args[-1];
}

// Math
static Bs_Value bs_num_sin(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(sin(args[-1].as.number));
}

static Bs_Value bs_math_cos(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(cos(args[-1].as.number));
}

static Bs_Value bs_math_tan(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(tan(args[-1].as.number));
}

static Bs_Value bs_math_asin(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(asin(args[-1].as.number));
}

static Bs_Value bs_math_acos(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(acos(args[-1].as.number));
}

static Bs_Value bs_math_atan(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(atan(args[-1].as.number));
}

static Bs_Value bs_math_exp(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(exp(args[-1].as.number));
}

static Bs_Value bs_math_log(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(log(args[-1].as.number));
}

static Bs_Value bs_math_log10(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(log10(args[-1].as.number));
}

static Bs_Value bs_math_pow(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    return bs_value_num(pow(args[-1].as.number, args[0].as.number));
}

static Bs_Value bs_math_sqrt(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    if (args[-1].as.number < 0) {
        bs_error(bs, "complex numbers are not supported");
    }
    return bs_value_num(sqrt(args[-1].as.number));
}

static Bs_Value bs_math_ceil(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(ceil(args[-1].as.number));
}

static Bs_Value bs_math_floor(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(floor(args[-1].as.number));
}

static Bs_Value bs_math_round(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(round(args[-1].as.number));
}

static Bs_Value bs_math_abs(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(fabs(args[-1].as.number));
}

static Bs_Value bs_math_sign(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    const double x = args[-1].as.number;
    return bs_value_num((x > 0) - (x < 0));
}

static Bs_Value bs_math_range(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 2 && arity != 3) {
        bs_error(bs, "expected 2 or 3 arguments, got %zu", arity);
    }

    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);

    const double begin = args[0].as.number;
    const double end = args[1].as.number;

    const bool ascending = end > begin;
    double     step = ascending ? 1 : -1;
    if (arity == 3) {
        bs_arg_check_value_type(bs, args, 2, BS_VALUE_NUM);
        step = args[2].as.number;
        if (step == 0 || ascending != step > 0) {
            bs_error_at(
                bs,
                3,
                "a step of %.15g in %s range would run indefinitely",
                step,
                ascending ? "an ascending" : "a descending");
        }
    }

    Bs_Array *a = bs_array_new(bs);
    for (double i = begin; ascending ? (i < end) : (i > end); i += step) {
        bs_array_set(bs, a, a->count, bs_value_num(i));
    }
    return bs_value_object(a);
}

static Bs_Value bs_math_max(Bs *bs, Bs_Value *args, size_t arity) {
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

static Bs_Value bs_math_min(Bs *bs, Bs_Value *args, size_t arity) {
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

static Bs_Value bs_math_clamp(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);

    const double low = bs_min(args[0].as.number, args[1].as.number);
    const double high = bs_max(args[0].as.number, args[1].as.number);
    return bs_value_num(bs_min(bs_max(args[-1].as.number, low), high));
}

static Bs_Value bs_math_lerp(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);

    const double a = args[-1].as.number;
    const double b = args[0].as.number;
    const double t = args[1].as.number;
    return bs_value_num(a + (b - a) * t);
}

static Bs_Value bs_math_precise(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    const double n = args[-1].as.number;
    const double l = pow(10, args[0].as.number);
    return bs_value_num(round(n * l) / l);
}

static Bs_Value bs_math_tohex(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    bs_check_integer(bs, args[-1], "this");

    char buffer[64];
    int  count = 0;
    if (args[-1].as.number < 0) {
        count = snprintf(buffer, sizeof(buffer), "-%lx", (size_t) -args[-1].as.number);
    } else {
        count = snprintf(buffer, sizeof(buffer), "%lx", (size_t) args[-1].as.number);
    }
    assert(count >= 0 && count + 1 < sizeof(buffer));

    return bs_value_object(bs_str_new(bs, Bs_Sv(buffer, count)));
}

typedef struct {
    uint64_t state[2];
} Bs_Random;

static uint64_t bs_random_u64(Bs_Random *r) {
    uint64_t       s0 = r->state[0];
    uint64_t       s1 = r->state[1];
    const uint64_t result = s0 + s1;

    s1 ^= s0;
    r->state[0] = ((s0 << 55) | (s0 >> 9)) ^ s1 ^ (s1 << 14);
    r->state[1] = (s1 << 36) | (s1 >> 28);
    return result;
}

static Bs_Value bs_random_init(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity > 1) {
        bs_error(bs, "expected 0 or 1 arguments, got %zu", arity);
    }

    uint64_t seed;
    if (arity) {
        bs_arg_check_whole_number(bs, args, 0);
        seed = (uint64_t) args[0].as.number;
    } else {
        seed = time(NULL);

#if defined(_WIN32) || defined(_WIN64)
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        seed ^= (uint64_t) (counter.QuadPart);
        seed ^= (uint64_t) _getpid();
#else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        seed ^= (uint64_t) (ts.tv_sec ^ ts.tv_nsec);
        seed ^= (uint64_t) getpid();
#endif
    }

    Bs_Random r;
    r.state[0] = seed;
    r.state[0] = (r.state[0] ^ (r.state[0] >> 30)) * 0xBF58476D1CE4E5B9ULL;
    r.state[1] = (r.state[0] ^ (r.state[0] >> 27)) * 0x94D049BB133111EBULL;

    bs_this_c_instance_data_as(args, Bs_Random) = r;
    return bs_value_nil;
}

static Bs_Value bs_random_number(Bs *bs, Bs_Value *args, size_t arity) {
    if (arity != 0 && arity != 2) {
        bs_error(bs, "expected 0 or 2 arguments, got %zu", arity);
    }

    if (arity) {
        bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
        bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);
    }

    const double result =
        (double) bs_random_u64(&bs_this_c_instance_data_as(args, Bs_Random)) / UINT64_MAX;
    if (!arity) {
        return bs_value_num(result);
    }

    const double min = bs_min(args[0].as.number, args[1].as.number);
    const double max = bs_max(args[0].as.number, args[1].as.number);
    return bs_value_num(min + result * (max - min));
}

static Bs_Value bs_random_bytes(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);

    Bs_Random *r = &bs_this_c_instance_data_as(args, Bs_Random);
    size_t     count = args[0].as.number;

    Bs_Value   instance = bs_call(bs, bs_value_object(bs_bytes_class), NULL, 0);
    Bs_Buffer *b = &bs_flex_member_as(((Bs_C_Instance *) instance.as.object)->data, Bs_Buffer);

    while (count) {
        const uint64_t n = bs_random_u64(r);
        const size_t   size = bs_min(sizeof(n), count);

        const char *bytes = (const char *) &n;
        for (size_t i = 0; i < size; i++) {
            bs_da_push(bs, b, (bytes[i] % 128 + 128) % 128);
        }
        count -= size;
    }

    return instance;
}

static Bs_Value bs_random_index(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);

    const Bs_Check checks[] = {
        bs_check_object(BS_OBJECT_STR),
        bs_check_object(BS_OBJECT_ARRAY),
    };
    bs_arg_check_multi(bs, args, 0, checks, bs_c_array_size(checks));

    const Bs_Object *container = args[0].as.object;

    size_t length = 0;
    if (container->type == BS_OBJECT_STR) {
        length = ((const Bs_Str *) container)->size;
    } else {
        length = ((const Bs_Array *) container)->count;
    }

    if (!length) {
        bs_error(bs, "expected non empty %s", bs_object_type_name(container->type));
    }

    const size_t index = bs_random_u64(&bs_this_c_instance_data_as(args, Bs_Random)) % length;
    if (container->type == BS_OBJECT_STR) {
        const Bs_Str *ch = bs_str_new(bs, Bs_Sv(&((const Bs_Str *) container)->data[index], 1));
        return bs_value_object(ch);
    } else {
        return ((const Bs_Array *) container)->data[index];
    }
}

// Metaprogramming
static Bs_C_Class *bs_meta_error_class;

typedef struct {
    size_t  row;
    size_t  col;
    Bs_Str *path;
    Bs_Str *line;

    Bs_Str *message;
    Bs_Str *explanation;
    Bs_Str *example;
} Bs_Meta_Error;

static void bs_meta_error_mark(Bs *bs, void *instance_data) {
    Bs_Meta_Error *e = &bs_flex_member_as(instance_data, Bs_Meta_Error);
    bs_mark(bs, (Bs_Object *) e->path);
    bs_mark(bs, (Bs_Object *) e->line);
    bs_mark(bs, (Bs_Object *) e->message);
    bs_mark(bs, (Bs_Object *) e->explanation);
    bs_mark(bs, (Bs_Object *) e->example);
}

static Bs_Value bs_meta_error_init(Bs *bs, Bs_Value *args, size_t arity) {
    bs_error(bs, "not supposed to be used directly");
    return bs_value_nil;
}

static Bs_Value bs_meta_error_row(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    const size_t row = bs_this_c_instance_data_as(args, Bs_Meta_Error).row;
    return row ? bs_value_num(row) : bs_value_nil;
}

static Bs_Value bs_meta_error_col(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    const size_t col = bs_this_c_instance_data_as(args, Bs_Meta_Error).col;
    return col ? bs_value_num(col) : bs_value_nil;
}

static Bs_Value bs_meta_error_path(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Bs_Str *path = bs_this_c_instance_data_as(args, Bs_Meta_Error).path;
    return path ? bs_value_object(path) : bs_value_nil;
}

static Bs_Value bs_meta_error_line(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Bs_Str *line = bs_this_c_instance_data_as(args, Bs_Meta_Error).line;
    return line ? bs_value_object(line) : bs_value_nil;
}

static Bs_Value bs_meta_error_message(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    // Message is guaranted
    return bs_value_object(bs_this_c_instance_data_as(args, Bs_Meta_Error).message);
}

static Bs_Value bs_meta_error_explanation(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Bs_Str *explanation = bs_this_c_instance_data_as(args, Bs_Meta_Error).explanation;
    return explanation ? bs_value_object(explanation) : bs_value_nil;
}

static Bs_Value bs_meta_error_example(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Bs_Str *example = bs_this_c_instance_data_as(args, Bs_Meta_Error).example;
    return example ? bs_value_object(example) : bs_value_nil;
}

typedef struct {
    Bs            *bs;
    Bs_C_Instance *error;
} Bs_Meta_Error_Context;

static void bs_meta_error_write(Bs_Error_Writer *w, Bs_Error error) {
    Bs_Meta_Error_Context *c = w->data;
    if (c->error) {
        return; // Skip stack trace
    }
    assert(error.type == BS_ERROR_MAIN); // Only compilation errors are possible here

    c->error = bs_c_instance_new(c->bs, bs_meta_error_class);
    Bs_Meta_Error *e = &bs_flex_member_as(c->error->data, Bs_Meta_Error);

    if (!error.native) {
        e->row = error.loc.row;
        e->col = error.loc.col;
        e->path = bs_str_new(c->bs, error.loc.path);
        e->line = bs_str_new(c->bs, error.loc.line);
    }

    e->message = bs_str_new(c->bs, error.message);

    if (error.explanation.size) {
        e->explanation = bs_str_new(c->bs, error.explanation);
    }

    if (error.example.size) {
        e->example = bs_str_new(c->bs, error.example);
    }
}

static Bs_Value bs_meta_compile(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    Bs_Str     *str = (Bs_Str *) args[0].as.object;
    const Bs_Sv input = Bs_Sv(str->data, str->size);

    Bs_Meta_Error_Context context = {.bs = bs};

    Bs_Config            *config = bs_config(bs);
    const Bs_Error_Writer save = config->error;

    config->error = (Bs_Error_Writer) {
        .data = &context,
        .write = bs_meta_error_write,
    };

    const Bs_Closure *closure =
        bs_compile(bs, Bs_Sv_Static("<meta>"), input, false, false, true, 0);

    config->error = save;

    if (!closure) {
        return bs_value_object(context.error);
    }

    closure->fn->source = str;
    return bs_value_object(closure);
}

static Bs_Value bs_meta_call(Bs *bs, Bs_Value *args, size_t arity) {
    if (!arity) {
        bs_error(bs, "expected at least 1 argument, got 0");
    }
    bs_arg_check_callable(bs, args, 0);

    Bs_Meta_Error_Context context = {.bs = bs};

    Bs_Config            *config = bs_config(bs);
    const Bs_Error_Writer error_save = config->error;

    config->error = (Bs_Error_Writer) {
        .data = &context,
        .write = bs_meta_error_write,
    };

    Bs_Value result = bs_value_nil;

    const Bs_Unwind unwind_save = bs_unwind_save(bs);
    if (setjmp(config->unwind.point)) {
        result = bs_value_object(context.error);
        goto end;
    }

    result = bs_call(bs, args[0], args + 1, arity - 1);

end:
    config->error = error_save;
    bs_unwind_restore(bs, &unwind_save);
    return result;
}

static Bs_Value bs_meta_eval(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    Bs_Str     *str = (Bs_Str *) args[0].as.object;
    const Bs_Sv input = Bs_Sv(str->data, str->size);

    const Bs_Closure *closure =
        bs_compile(bs, Bs_Sv_Static("<meta>"), input, false, false, true, 0);

    if (!closure) {
        Bs_Error error = bs_error_begin(bs);
        error.type = BS_ERROR_TRACE;
        error.message = Bs_Sv_Static("eval()");

        Bs_Config *config = bs_config(bs);
        config->error.write(&config->error, error);
        bs_error_end(bs, error.native);
    }

    closure->fn->source = str;
    return bs_call(bs, bs_value_object(closure), NULL, 0);
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
    {
        bs_io_reader_class =
            bs_c_class_new(bs, Bs_Sv_Static("Reader"), sizeof(Bs_File), bs_io_reader_init);

        bs_io_reader_class->can_fail = true;
        bs_io_reader_class->free = bs_io_file_free;

        bs_c_class_add(bs, bs_io_reader_class, Bs_Sv_Static("close"), bs_io_file_close);
        bs_c_class_add(bs, bs_io_reader_class, Bs_Sv_Static("read"), bs_io_reader_read);
        bs_c_class_add(bs, bs_io_reader_class, Bs_Sv_Static("readln"), bs_io_reader_readln);

        bs_c_class_add(bs, bs_io_reader_class, Bs_Sv_Static("eof"), bs_io_reader_eof);
        bs_c_class_add(bs, bs_io_reader_class, Bs_Sv_Static("seek"), bs_io_reader_seek);
        bs_c_class_add(bs, bs_io_reader_class, Bs_Sv_Static("tell"), bs_io_reader_tell);

        bs_io_writer_class =
            bs_c_class_new(bs, Bs_Sv_Static("Writer"), sizeof(Bs_File), bs_io_writer_init);

        bs_io_writer_class->can_fail = true;
        bs_io_writer_class->free = bs_io_file_free;

        bs_c_class_add(bs, bs_io_writer_class, Bs_Sv_Static("close"), bs_io_file_close);
        bs_c_class_add(bs, bs_io_writer_class, Bs_Sv_Static("flush"), bs_io_writer_flush);
        bs_c_class_add(bs, bs_io_writer_class, Bs_Sv_Static("write"), bs_io_writer_write);
        bs_c_class_add(bs, bs_io_writer_class, Bs_Sv_Static("writeln"), bs_io_writer_writeln);

        bs_io_direntry_class =
            bs_c_class_new(bs, Bs_Sv_Static("DirEntry"), sizeof(Bs_Dir_Entry), bs_io_direntry_init);

        bs_io_direntry_class->mark = bs_io_direntry_mark;

        bs_c_class_add(bs, bs_io_direntry_class, Bs_Sv_Static("name"), bs_io_direntry_name);
        bs_c_class_add(bs, bs_io_direntry_class, Bs_Sv_Static("isdir"), bs_io_direntry_isdir);

        Bs_Table *io = bs_table_new(bs);
        bs_add(bs, io, "Reader", bs_value_object(bs_io_reader_class));
        bs_add(bs, io, "Writer", bs_value_object(bs_io_writer_class));
        bs_add(bs, io, "DirEntry", bs_value_object(bs_io_direntry_class));

        bs_add_fn(bs, io, "input", bs_io_input);

        bs_add_fn(bs, io, "print", bs_io_print);
        bs_add_fn(bs, io, "eprint", bs_io_eprint);

        bs_add_fn(bs, io, "println", bs_io_println);
        bs_add_fn(bs, io, "eprintln", bs_io_eprintln);

        bs_add_fn(bs, io, "readdir", bs_io_readdir);
        bs_add_fn(bs, io, "readfile", bs_io_readfile);

        {
            Bs_C_Instance *io_stdin = bs_c_instance_new(bs, bs_io_reader_class);
            bs_flex_member_as(io_stdin->data, Bs_File).file = stdin;
            bs_add(bs, io, "stdin", bs_value_object(io_stdin));
        }

        {
            Bs_C_Instance *io_stdout = bs_c_instance_new(bs, bs_io_writer_class);
            bs_flex_member_as(io_stdout->data, Bs_File).file = stdout;
            bs_add(bs, io, "stdout", bs_value_object(io_stdout));
        }

        {
            Bs_C_Instance *io_stderr = bs_c_instance_new(bs, bs_io_writer_class);
            bs_flex_member_as(io_stderr->data, Bs_File).file = stderr;
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

        bs_add_fn(bs, os, "getcwd", bs_os_getcwd);
        bs_add_fn(bs, os, "setcwd", bs_os_setcwd);

        Bs_Array *args = bs_array_new(bs);
        for (int i = 0; i < argc; i++) {
            bs_array_set(bs, args, i, bs_value_object(bs_str_new(bs, bs_sv_from_cstr(argv[i]))));
        }
        bs_add(bs, os, "args", bs_value_object(args));

        Bs_C_Class *process_class =
            bs_c_class_new(bs, Bs_Sv_Static("Process"), sizeof(Bs_Process), bs_process_init);

        process_class->can_fail = true;
        process_class->mark = bs_process_mark;

        bs_c_class_add(bs, process_class, Bs_Sv_Static("kill"), bs_process_kill);
        bs_c_class_add(bs, process_class, Bs_Sv_Static("wait"), bs_process_wait);
        bs_c_class_add(bs, process_class, Bs_Sv_Static("stdout"), bs_process_stdout);
        bs_c_class_add(bs, process_class, Bs_Sv_Static("stderr"), bs_process_stderr);
        bs_c_class_add(bs, process_class, Bs_Sv_Static("stdin"), bs_process_stdin);

        bs_add(bs, os, "Process", bs_value_object(process_class));

#if defined(_WIN32) || defined(_WIN64)
        bs_add(bs, os, "name", bs_value_object(bs_str_new(bs, Bs_Sv_Static("Windows"))));
#elif defined(__APPLE__) || defined(__MACH__)
        bs_add(bs, os, "name", bs_value_object(bs_str_new(bs, Bs_Sv_Static("macOS"))));
#elif defined(__linux__)
        bs_add(bs, os, "name", bs_value_object(bs_str_new(bs, Bs_Sv_Static("Linux"))));
#else
        bs_add(bs, os, "name", bs_value_object(bs_str_new(bs, Bs_Sv_Static("Unknown"))));
#endif

#if defined(__x86_64__) || defined(_M_X64)
        bs_add(bs, os, "arch", bs_value_object(bs_str_new(bs, Bs_Sv_Static("x86_64"))));
#elif defined(__aarch64__) || defined(_M_ARM64)
        bs_add(bs, os, "arch", bs_value_object(bs_str_new(bs, Bs_Sv_Static("ARM64"))));
#else
        bs_add(bs, os, "arch", bs_value_object(bs_str_new(bs, Bs_Sv_Static("Unknown"))));
#endif

        bs_global_set(bs, Bs_Sv_Static("os"), bs_value_object(os));
    }

    {
        bs_regex_class = bs_c_class_new(bs, Bs_Sv_Static("Regex"), sizeof(Bs_Regex), bs_regex_init);
        bs_regex_class->can_fail = true;
        bs_regex_class->free = bs_regex_free;

        bs_global_set(bs, Bs_Sv_Static("Regex"), bs_value_object(bs_regex_class));
    }

    {
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("slice"), bs_str_slice);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("reverse"), bs_str_reverse);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("repeat"), bs_str_repeat);

        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("toupper"), bs_str_toupper);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("tolower"), bs_str_tolower);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("tonumber"), bs_str_tonumber);

        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("find"), bs_str_find);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("split"), bs_str_split);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("replace"), bs_str_replace);

        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("compare"), bs_str_compare);

        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("trim"), bs_str_trim);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("ltrim"), bs_str_ltrim);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("rtrim"), bs_str_rtrim);

        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("lpad"), bs_str_lpad);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("rpad"), bs_str_rpad);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("prefix"), bs_str_prefix);
        bs_builtin_object_methods_add(bs, BS_OBJECT_STR, Bs_Sv_Static("suffix"), bs_str_suffix);
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
        bs_bytes_class =
            bs_c_class_new(bs, Bs_Sv_Static("Bytes"), sizeof(Bs_Buffer), bs_bytes_init);

        bs_bytes_class->free = bs_bytes_free;
        bs_bytes_class->show = bs_bytes_show;

        bs_c_class_add(bs, bs_bytes_class, Bs_Sv_Static("count"), bs_bytes_count);
        bs_c_class_add(bs, bs_bytes_class, Bs_Sv_Static("reset"), bs_bytes_reset);

        bs_c_class_add(bs, bs_bytes_class, Bs_Sv_Static("slice"), bs_bytes_slice);
        bs_c_class_add(bs, bs_bytes_class, Bs_Sv_Static("push"), bs_bytes_push);
        bs_c_class_add(bs, bs_bytes_class, Bs_Sv_Static("insert"), bs_bytes_insert);

        bs_c_class_add(bs, bs_bytes_class, Bs_Sv_Static("get"), bs_bytes_get);
        bs_c_class_add(bs, bs_bytes_class, Bs_Sv_Static("set"), bs_bytes_set);

        bs_global_set(bs, Bs_Sv_Static("Bytes"), bs_value_object(bs_bytes_class));
    }

    {
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("map"), bs_array_map);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("filter"), bs_array_filter);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("reduce"), bs_array_reduce);

        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("join"), bs_array_join);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("find"), bs_array_find);

        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("push"), bs_array_push);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("insert"), bs_array_insert);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("pop"), bs_array_pop);

        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("sort"), bs_array_sort);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("resize"), bs_array_resize);
        bs_builtin_object_methods_add(
            bs, BS_OBJECT_ARRAY, Bs_Sv_Static("reverse"), bs_array_reverse);

        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("fill"), bs_array_fill);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("slice"), bs_array_slice);
        bs_builtin_object_methods_add(bs, BS_OBJECT_ARRAY, Bs_Sv_Static("append"), bs_array_append);
    }

    {
        // Table
        bs_builtin_object_methods_add(bs, BS_OBJECT_TABLE, Bs_Sv_Static("extend"), bs_table_extend);
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
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("abs"), bs_math_abs);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("sign"), bs_math_sign);

        bs_builtin_number_methods_add(bs, Bs_Sv_Static("max"), bs_math_max);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("min"), bs_math_min);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("clamp"), bs_math_clamp);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("lerp"), bs_math_lerp);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("precise"), bs_math_precise);
        bs_builtin_number_methods_add(bs, Bs_Sv_Static("tohex"), bs_math_tohex);

        Bs_Table *math = bs_table_new(bs);

        {
            Bs_C_Class *random_class =
                bs_c_class_new(bs, Bs_Sv_Static("Random"), sizeof(Bs_Random), bs_random_init);

            bs_c_class_add(bs, random_class, Bs_Sv_Static("number"), bs_random_number);
            bs_c_class_add(bs, random_class, Bs_Sv_Static("bytes"), bs_random_bytes);
            bs_c_class_add(bs, random_class, Bs_Sv_Static("index"), bs_random_index);
            bs_add(bs, math, "Random", bs_value_object(random_class));
        }

        bs_add_fn(bs, math, "range", bs_math_range);
        bs_add(bs, math, "E", bs_value_num(2.7182818284590452354));
        bs_add(bs, math, "PI", bs_value_num(3.14159265358979323846));
        bs_global_set(bs, Bs_Sv_Static("math"), bs_value_object(math));
    }

    {
        Bs_Table *meta = bs_table_new(bs);

        {
            bs_meta_error_class = bs_c_class_new(
                bs, Bs_Sv_Static("Error"), sizeof(Bs_Meta_Error), bs_meta_error_init);

            bs_meta_error_class->mark = bs_meta_error_mark;

            bs_c_class_add(bs, bs_meta_error_class, Bs_Sv_Static("row"), bs_meta_error_row);
            bs_c_class_add(bs, bs_meta_error_class, Bs_Sv_Static("col"), bs_meta_error_col);
            bs_c_class_add(bs, bs_meta_error_class, Bs_Sv_Static("path"), bs_meta_error_path);
            bs_c_class_add(bs, bs_meta_error_class, Bs_Sv_Static("line"), bs_meta_error_line);
            bs_c_class_add(bs, bs_meta_error_class, Bs_Sv_Static("message"), bs_meta_error_message);
            bs_c_class_add(bs, bs_meta_error_class, Bs_Sv_Static("example"), bs_meta_error_example);
            bs_c_class_add(
                bs, bs_meta_error_class, Bs_Sv_Static("explanation"), bs_meta_error_explanation);

            bs_add(bs, meta, "Error", bs_value_object(bs_meta_error_class));
        }

        bs_add_fn(bs, meta, "compile", bs_meta_compile);
        bs_add_fn(bs, meta, "call", bs_meta_call);
        bs_add_fn(bs, meta, "eval", bs_meta_eval);
        bs_global_set(bs, Bs_Sv_Static("meta"), bs_value_object(meta));
    }
}
