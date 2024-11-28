#include "bs/vm.h"

#define CROSSLINE_IMPLEMENTATION
#include "crossline/crossline.h"

#ifdef _WIN32
#    include <io.h>
#    define isatty _isatty
#    define fileno _fileno
#    define PATH_SEPARATOR '\\'
#else
#    include <unistd.h>
#    define PATH_SEPARATOR '/'
#endif

static void bs_error_write_colors(Bs_Error_Writer *w, Bs_Error error) {
    if (error.native) {
        fprintf(stderr, "[C]: ");
    } else if (error.type != BS_ERROR_STANDALONE) {
        fprintf(stderr, Bs_Loc_Fmt, Bs_Loc_Arg(error.loc));
    }

    if (error.type == BS_ERROR_TRACE) {
        crossline_color_set_on(0, CROSSLINE_FGCOLOR_YELLOW);
        fprintf(stderr, "in ");
        crossline_color_set_on(0, CROSSLINE_FGCOLOR_DEFAULT);
    } else if (error.type != BS_ERROR_PANIC) {
        crossline_color_set_on(0, CROSSLINE_FGCOLOR_RED);
        fprintf(stderr, "error: ");
        crossline_color_set_on(0, CROSSLINE_FGCOLOR_DEFAULT);
    }

    fprintf(stderr, Bs_Sv_Fmt "\n", Bs_Sv_Arg(error.message));

    if (!error.native && error.type != BS_ERROR_STANDALONE) {
        fprintf(stderr, "\n    ");

        crossline_color_set_on(0, CROSSLINE_FGCOLOR_CYAN);
        fprintf(stderr, "%zu | ", error.loc.row);
        crossline_color_set_on(0, CROSSLINE_FGCOLOR_DEFAULT);

        const Bs_Sv line = bs_sv_trim(error.loc.line, '\r'); // Video Game OS fix
        fprintf(stderr, Bs_Sv_Fmt "\n", Bs_Sv_Arg(line));

        const int count = snprintf(NULL, 0, "    %zu", error.loc.row);
        assert(count >= 0);
        for (size_t i = 0; i < count; i++) {
            fputc(' ', stderr);
        }

        crossline_color_set_on(0, CROSSLINE_FGCOLOR_CYAN);
        fputs(" | ", stderr);
        crossline_color_set_on(0, CROSSLINE_FGCOLOR_DEFAULT);

        for (size_t i = 0; i + 1 < error.loc.col; i++) {
            fputc(line.data[i] == '\t' ? '\t' : ' ', stderr);
        }

        crossline_color_set_on(0, CROSSLINE_FGCOLOR_MAGENTA);
        fputs("^\n", stderr);
        crossline_color_set_on(0, CROSSLINE_FGCOLOR_DEFAULT);
    }

    if (!error.explanation.size && !error.example.size && error.continued) {
        fputc('\n', stderr);
    }

    if (error.explanation.size) {
        crossline_color_set_on(0, CROSSLINE_FGCOLOR_YELLOW);
        fprintf(stderr, "\n" Bs_Sv_Fmt "\n", Bs_Sv_Arg(error.explanation));
        crossline_color_set_on(0, CROSSLINE_FGCOLOR_DEFAULT);
    }

    if (error.example.size) {
        crossline_color_set_on(0, CROSSLINE_FGCOLOR_GREEN);
        fprintf(stderr, "\n```\n" Bs_Sv_Fmt "\n```\n", Bs_Sv_Arg(error.example));
        crossline_color_set_on(0, CROSSLINE_FGCOLOR_DEFAULT);
    }

    if ((error.explanation.size || error.example.size) && error.continued) {
        fprintf(stderr, "\n");
    }
}

static bool bs_repl_block(char *line, size_t size, Bs_Sv *input, Bs_Sv ending, bool do_indent) {
    size_t indent = 0;
    while (true) {
        const size_t available = size - input->size;
        const size_t start = input->size;

        if (do_indent) {
            assert(available > indent * CROSSLINE_INDENT_SIZE);
            for (size_t i = 0; i < indent; i++) {
                memcpy(
                    &line[start + i * CROSSLINE_INDENT_SIZE],
                    CROSSLINE_INDENT,
                    CROSSLINE_INDENT_SIZE);
            }
            line[start + indent * CROSSLINE_INDENT_SIZE] = '\0';

            if (!crossline_readline2("| ", line + start, available)) {
                return false;
            }
        } else {
            assert(available);
            if (!crossline_readline("| ", line + start, available)) {
                return false;
            }
        }

        *input = bs_sv_from_cstr(line);
        if (bs_sv_suffix(*input, ending)) {
            break;
        }

        if (do_indent) {
            size_t new_indent = 0;
            for (size_t i = start; i < input->size && line[i] == ' '; i++) {
                new_indent++;
            }

            if (new_indent % CROSSLINE_INDENT_SIZE == 0) {
                indent = new_indent / CROSSLINE_INDENT_SIZE;
            }
        }

        line[input->size++] = '\n';
    }

    input->data += 2;
    input->size -= 4;
    return true;
}

static void bs_history_path(char *buffer, size_t size) {
    const char *base;

#ifdef _WIN32
    base = getenv("APPDATA");
    if (base) {
        snprintf(buffer, size, "%s%cbs_history", base, PATH_SEPARATOR);
        return;
    }
#else
    base = getenv("XDG_CACHE_HOME");
    if (base) {
        snprintf(buffer, size, "%s%cbs_history", base, PATH_SEPARATOR);
        return;
    }

    base = getenv("HOME");
    if (base) {
        snprintf(buffer, size, "%s%c.cache%cbs_history", base, PATH_SEPARATOR, PATH_SEPARATOR);
        return;
    }
#endif

    snprintf(buffer, size, ".bs_history");
}

int main(int argc, char **argv) {
    crossline_prompt_color_set(CROSSLINE_FGCOLOR_BLUE);

    Bs *bs = bs_new(argc - 1, argv + 1);
    bs_config(bs)->error.write = bs_error_write_colors;

    if (argc < 2 || !strcmp(argv[1], "-")) {
        Bs_Result result = {0};
        if (isatty(fileno(stdin))) {
            Bs_Writer *w = &bs_config(bs)->log;

            crossline_color_set(CROSSLINE_FGCOLOR_YELLOW);
            bs_fmt(w, "Welcome to the BS Repl! Use :h to get help.\n");
            crossline_color_set(CROSSLINE_FGCOLOR_DEFAULT);

            static char history_path[1024];
            bs_history_path(history_path, sizeof(history_path));
            crossline_history_load(history_path);

            static char line[8 * 1024]; // 8KB is enough for all repl related tasks
            while (true) {
                if (!crossline_readline("> ", line, sizeof(line))) {
                    break;
                }

                Bs_Sv input = bs_sv_from_cstr(line);
                if (!input.size) {
                    continue;
                }

                if (bs_sv_eq(input, Bs_Sv_Static(":q"))) {
                    break;
                }

                if (bs_sv_eq(input, Bs_Sv_Static(":h"))) {
                    crossline_color_set(CROSSLINE_FGCOLOR_YELLOW);
                    bs_fmt(
                        w,
                        "Use :q or CTRL-d to quit.\n\n"
                        "Use :! to execute shell commands:\n");
                    crossline_color_set(CROSSLINE_FGCOLOR_DEFAULT);

                    crossline_color_set(CROSSLINE_FGCOLOR_GREEN);
                    bs_fmt(
                        w,
                        ":!ls -A\n"
                        ":!vim main.bs\n\n");
                    crossline_color_set(CROSSLINE_FGCOLOR_DEFAULT);

                    crossline_color_set(CROSSLINE_FGCOLOR_YELLOW);
                    bs_fmt(w, "Use :{ and :} to execute multiple lines at once:\n");
                    crossline_color_set(CROSSLINE_FGCOLOR_DEFAULT);

                    crossline_color_set(CROSSLINE_FGCOLOR_GREEN);
                    bs_fmt(
                        w,
                        ":{\n"
                        "for _ in 0, 5 {\n"
                        "    io.println(\"Hello, world!\");\n"
                        "}\n"
                        ":}\n\n");
                    crossline_color_set(CROSSLINE_FGCOLOR_DEFAULT);

                    crossline_color_set(CROSSLINE_FGCOLOR_YELLOW);
                    bs_fmt(
                        w,
                        "Use :[ and :] to execute multiple lines without auto indentation (in case "
                        "of copy "
                        "pasting)\n\n");
                    crossline_color_set(CROSSLINE_FGCOLOR_DEFAULT);

                    crossline_color_set(CROSSLINE_FGCOLOR_YELLOW);
                    bs_fmt(w, "Website: https://shoumodip.github.io/bs\n");
                    crossline_color_set(CROSSLINE_FGCOLOR_DEFAULT);
                    continue;
                }

                if (bs_sv_prefix(input, Bs_Sv_Static(":!"))) {
                    system(line + 2);
                    continue;
                }

                if (bs_sv_eq(input, Bs_Sv_Static(":{"))) {
                    if (!bs_repl_block(line, sizeof(line), &input, Bs_Sv_Static(":}"), true)) {
                        bs_error_standalone(bs, "could not read from standard input");
                        bs_free(bs);
                        crossline_history_save(history_path);
                        return 1;
                    }
                } else if (bs_sv_eq(input, Bs_Sv_Static(":["))) {
                    if (!bs_repl_block(line, sizeof(line), &input, Bs_Sv_Static(":]"), false)) {
                        bs_error_standalone(bs, "could not read from standard input");
                        bs_free(bs);
                        crossline_history_save(history_path);
                        return 1;
                    }
                }

                result = bs_run(bs, Bs_Sv_Static("<stdin>"), input, true);
                if (result.ok) {
                    if (result.exit != -1) {
                        break;
                    }

                    crossline_color_set(CROSSLINE_FGCOLOR_MAGENTA);
                    bs_value_write(bs, w, result.value);
                    crossline_color_set(CROSSLINE_FGCOLOR_DEFAULT);
                }
                bs_fmt(w, "\n");
                result = (Bs_Result){0};
            }

            crossline_history_save(history_path);
        } else {
            Bs_Buffer buffer = {0};
            while (!feof(stdin)) {
                bs_da_push_many(bs, &buffer, NULL, 1024);
                char *head = &buffer.data[buffer.count];
                buffer.count += fread(head, 1, buffer.capacity - buffer.count, stdin);
            }

            result = bs_run(bs, Bs_Sv_Static("<stdin>"), bs_buffer_reset(&buffer, 0), false);
            bs_da_free(bs, &buffer);
        }

        bs_free(bs);
        return result.exit == -1 ? !result.ok : result.exit;
    }

    const char *path = argv[1];

    size_t size = 0;
    char *contents = bs_read_file(path, &size);
    if (!contents) {
        bs_error_standalone(bs, "could not read file '%s'", path);
        bs_free(bs);
        return 1;
    }

    const Bs_Result result = bs_run(bs, bs_sv_from_cstr(path), Bs_Sv(contents, size), false);
    free(contents);

    bs_free(bs);
    return result.exit == -1 ? !result.ok : result.exit;
}
