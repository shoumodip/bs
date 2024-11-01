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

int main(int argc, char **argv) {
    const char *help_format = "";
    const char *sample_format = "";
    const char *result_format = "";
    const char *normal_format = "";
    if (bs_try_stderr_colors()) {
        help_format = "\033[33m";
        sample_format = "\033[32m";
        result_format = "\033[35m";
        normal_format = "\033[0m";
        crossline_prompt_color_set(CROSSLINE_FGCOLOR_BLUE);
    }

    if (argc < 2 || !strcmp(argv[1], "-")) {
        Bs *bs = bs_new();
        bs_core_init(bs, argc - 1, argv + 1);

        Bs_Result result = {0};

        if (isatty(fileno(stdin))) {
            Bs_Writer *w = &bs_config(bs)->log;
            bs_fmt(
                w, "%sWelcome to the BS Repl! Use :h to get help.%s\n", help_format, normal_format);

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
                    bs_fmt(
                        w,
                        "%sUse :q or CTRL-d to quit.\n\n"
                        "Use :! to execute shell commands:%s\n"
                        "%s:!ls -A\n"
                        ":!vim main.bs%s\n\n"
                        "%sUse :{ and :} to execute multiple lines at once:%s\n"
                        "%s:{\n"
                        "for _ in 0, 5 {\n"
                        "    io.println(\"Hello, world!\");\n"
                        "}\n"
                        ":}%s\n\n"
                        "%sWebsite: https://shoumodip.github.io/bs%s\n",
                        help_format,
                        normal_format,
                        sample_format,
                        normal_format,
                        help_format,
                        normal_format,
                        sample_format,
                        normal_format,
                        help_format,
                        normal_format);
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

                    bs_fmt(w, "%s", result_format);
                    bs_value_write(bs, w, result.value);
                    bs_fmt(w, "%s\n", normal_format);
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
        Bs_Writer w = bs_file_writer(stderr);
        bs_efmt(&w, "could not read file '%s'\n", path);
        return 1;
    }

    Bs *bs = bs_new();
    bs_core_init(bs, argc - 1, argv + 1);

    const Bs_Result result = bs_run(bs, bs_sv_from_cstr(path), Bs_Sv(contents, size), false);
    free(contents);

    bs_free(bs);
    return result.exit == -1 ? !result.ok : result.exit;
}
