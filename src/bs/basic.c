#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "bs/basic.h"

Bs_Sv bs_sv_from_cstr(const char *data) {
    return (Bs_Sv){data, strlen(data)};
}

Bs_Sv bs_sv_trim(Bs_Sv s, char ch) {
    while (s.size && *s.data == ch) {
        s.data++;
        s.size--;
    }

    while (s.size && s.data[s.size - 1] == ch) {
        s.size--;
    }

    return s;
}

Bs_Sv bs_sv_drop(Bs_Sv *s, size_t count) {
    const Bs_Sv result = Bs_Sv(s->data, count);
    s->data += count;
    s->size -= count;
    return result;
}

Bs_Sv bs_sv_split(Bs_Sv *s, char ch) {
    const char *p = memchr(s->data, ch, s->size);
    if (!p) {
        const Bs_Sv result = *s;
        s->data += s->size;
        s->size = 0;
        return result;
    }

    const Bs_Sv result = Bs_Sv(s->data, p - s->data);
    s->data = p + 1;
    s->size -= result.size + 1;
    return result;
}

bool bs_sv_eq(Bs_Sv a, Bs_Sv b) {
    return a.size == b.size && !memcmp(a.data, b.data, b.size);
}

bool bs_sv_find(Bs_Sv s, char ch, size_t *index) {
    const char *p = memchr(s.data, ch, s.size);
    if (!p) {
        return false;
    }

    if (index) {
        *index = p - s.data;
    }
    return true;
}

bool bs_sv_prefix(Bs_Sv a, Bs_Sv b) {
    return a.size >= b.size && !memcmp(a.data, b.data, b.size);
}

bool bs_sv_suffix(Bs_Sv a, Bs_Sv b) {
    return a.size >= b.size && !memcmp(a.data + a.size - b.size, b.data, b.size);
}

static void bs_file_write(Bs_Writer *w, Bs_Sv sv) {
    fwrite(sv.data, sv.size, 1, w->data);
}

Bs_Writer bs_file_writer(FILE *f) {
    return (Bs_Writer){.data = f, .write = bs_file_write};
}

static char bs_fmt_buffer[1024];

void bs_vfmt(Bs_Writer *w, const char *fmt, va_list args) {
    const int count = vsnprintf(bs_fmt_buffer, sizeof(bs_fmt_buffer), fmt, args);
    assert(count >= 0 && count + 1 < sizeof(bs_fmt_buffer));
    w->write(w, Bs_Sv(bs_fmt_buffer, count));
}

void bs_fmt(Bs_Writer *w, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    bs_vfmt(w, fmt, args);
    va_end(args);
}

static bool bs_stderr_colors;

bool bs_get_stderr_colors(void) {
    return bs_stderr_colors;
}

void bs_set_stderr_colors(bool on) {
    bs_stderr_colors = on;
}

#ifdef _WIN32
#    include <io.h>
#    include <windows.h>

#    define isatty _isatty
#    define fileno _fileno
#else
#    include <unistd.h>
#endif // _WIN32

bool bs_try_stderr_colors(void) {
    bs_stderr_colors = false;

#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_ERROR_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) {
        return;
    }

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, mode)) {
        return;
    }
#endif // _WIN32

    bs_stderr_colors = isatty(fileno(stderr));
    return bs_stderr_colors;
}

void bs_evfmt(Bs_Writer *w, const char *fmt, va_list args) {
    const int count = vsnprintf(bs_fmt_buffer, sizeof(bs_fmt_buffer), fmt, args);
    assert(count >= 0 && count + 1 < sizeof(bs_fmt_buffer));

    w->write(
        w, bs_stderr_colors ? Bs_Sv_Static("\033[1;31merror:\033[0m ") : Bs_Sv_Static("error: "));
    w->write(w, Bs_Sv(bs_fmt_buffer, count));
}

void bs_efmt(Bs_Writer *w, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    bs_evfmt(w, fmt, args);
    va_end(args);
}

char *bs_read_file(const char *path, size_t *size) {
    char *result = NULL;

    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) == -1) {
        bs_return_defer(NULL);
    }

    const long offset = ftell(f);
    if (offset == -1) {
        bs_return_defer(NULL);
    }

    if (fseek(f, 0, SEEK_SET) == -1) {
        bs_return_defer(NULL);
    }

    result = malloc(offset + 1);
    if (!result) {
        bs_return_defer(NULL);
    }

    fread(result, 1, offset, f);
    if (ferror(f)) {
        free(result);
        bs_return_defer(NULL);
    }
    result[offset] = '\0';

    *size = offset;

defer:
    fclose(f);
    return result;
}
