#include <stdio.h>

#include "bs/core.h"

#define CROSSLINE_IMPLEMENTATION
#include "crossline/crossline.h"

#ifdef _WIN32
#    include <io.h>
#    define isatty _isatty
#    define fileno _fileno
#else
#    include <unistd.h>
#endif

static void bs_error_write(Bs_Error_Writer *w, Bs_Error error) {
    if (error.native) {
        fprintf(stderr, "[C]: ");
    } else if (error.type != BS_ERROR_USAGE) {
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

    if (error.continued) {
        fprintf(stderr, "\n");
    }
}

int main(int argc, char **argv) {
    crossline_prompt_color_set(CROSSLINE_FGCOLOR_BLUE);

    Bs *bs = bs_new((Bs_Error_Writer){.write = bs_error_write});
    bs_core_init(bs, argc - 1, argv + 1);

    if (argc < 2 || !strcmp(argv[1], "-")) {
        Bs_Result result = {0};
        if (isatty(fileno(stdin))) {
            Bs_Writer *w = &bs_config(bs)->log;

            crossline_color_set(CROSSLINE_FGCOLOR_YELLOW);
            bs_fmt(w, "Welcome to the BS Repl! Use :h to get help.\n");
            crossline_color_set(CROSSLINE_FGCOLOR_DEFAULT);

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
                    bs_fmt(w, "Website: https://shoumodip.github.io/bs\n");
                    crossline_color_set(CROSSLINE_FGCOLOR_DEFAULT);
                    continue;
                }

                if (bs_sv_prefix(input, Bs_Sv_Static(":!"))) {
                    system(line + 2);
                    continue;
                }

                if (bs_sv_eq(input, Bs_Sv_Static(":{"))) {
                    while (true) {
                        const size_t size = sizeof(line) - input.size;
                        assert(size);

                        if (!crossline_readline("| ", line + input.size, size)) {
                            bs_free(bs);
                            return result.exit == -1 ? !result.ok : result.exit;
                        }

                        input = bs_sv_from_cstr(line);
                        if (bs_sv_suffix(input, Bs_Sv_Static(":}"))) {
                            break;
                        }

                        line[input.size++] = '\n';
                    }

                    input.data += 2;
                    input.size -= 4;
                }

                result = bs_run(bs, Bs_Sv_Static("<stdin>.bs"), input, true);
                if (result.ok) {
                    if (result.exit != -1) {
                        break;
                    }

                    crossline_color_set(CROSSLINE_FGCOLOR_MAGENTA);
                    bs_value_write(bs, w, result.value);
                    crossline_color_set(CROSSLINE_FGCOLOR_DEFAULT);
                    bs_fmt(w, "\n");
                }
            }
        } else {
            Bs_Buffer buffer = {0};
            while (!feof(stdin)) {
                bs_da_push_many(bs, &buffer, NULL, 1024);
                char *head = &buffer.data[buffer.count];
                buffer.count += fread(head, 1, buffer.capacity - buffer.count, stdin);
            }

            result = bs_run(bs, Bs_Sv_Static("<stdin>.bs"), bs_buffer_reset(&buffer, 0), false);
            bs_da_free(bs, &buffer);
        }

        bs_free(bs);
        return result.exit == -1 ? !result.ok : result.exit;
    }

    const char *path = argv[1];

    size_t size = 0;
    char *contents = bs_read_file(path, &size);
    if (!contents) {
        Bs_Buffer *b = &bs_config(bs)->buffer;
        const size_t start = b->count;

        Bs_Writer w = bs_buffer_writer(b);
        bs_fmt(&w, "could not read file '%s'", path);

        const Bs_Error error = {
            .type = BS_ERROR_USAGE,
            .message = bs_buffer_reset(b, start),
        };

        Bs_Error_Writer *e = &bs_config(bs)->error;
        e->write(e, error);

        bs_free(bs);
        return 1;
    }

    const Bs_Result result = bs_run(bs, bs_sv_from_cstr(path), Bs_Sv(contents, size), false);
    free(contents);

    bs_free(bs);
    return result.exit == -1 ? !result.ok : result.exit;
}
