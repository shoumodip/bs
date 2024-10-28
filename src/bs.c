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
    if (argc < 2 || !strcmp(argv[1], "-")) {
        Bs *bs = bs_new();
        bs_core_init(bs, argc - 1, argv + 1);

        Bs_Result result = {0};

        if (isatty(fileno(stdin))) {
            Bs_Writer *w = &bs_config(bs)->log;

            bs_fmt(
                w,
                "Welcome to the BS Repl!\n"
                "You can type BS statements here that will be evaluated.\n\n"
                "Use :q or CTRL-d to quit.\n\n"
                "Use :! to execute shell commands:\n\n"
                ":!ls -A\n"
                ":!vim main.bs\n\n"
                "Use :{ and :} to execute multiple lines at once:\n\n"
                ":{\n"
                "for _ in 0, 5 {\n"
                "    io.println(\"Hello, world!\");\n"
                "}\n"
                ":}\n\n"
                "Website: https://shoumodip.github.io/bs/\n\n");

            static char line[8 * 1024]; // 8KB is enough for all repl related tasks
            while (true) {
                if (!crossline_readline("> ", line, sizeof(line))) {
                    break;
                }

                Bs_Sv input = bs_sv_from_cstr(line);
                if (!input.size) {
                    continue;
                }

                if (bs_sv_prefix(input, Bs_Sv_Static(":!"))) {
                    system(line + 2);
                    continue;
                }

                if (bs_sv_eq(input, Bs_Sv_Static(":q"))) {
                    break;
                }

                if (bs_sv_eq(input, Bs_Sv_Static(":{"))) {
                    while (true) {
                        const size_t size = sizeof(line) - input.size;
                        assert(size);

                        if (!crossline_readline("| ", line + input.size, size)) {
                            break;
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

                    bs_value_write(bs, w, result.value);
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
        fprintf(stderr, "error: could not read file '%s'\n", path);
        return 1;
    }

    Bs *bs = bs_new();
    bs_core_init(bs, argc - 1, argv + 1);

    const Bs_Result result = bs_run(bs, bs_sv_from_cstr(path), Bs_Sv(contents, size), false);
    free(contents);

    bs_free(bs);
    return result.exit == -1 ? !result.ok : result.exit;
}
