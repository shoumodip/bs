#include <stdio.h>

#include "bs/core.h"

int main(int argc, char **argv) {
    // Repl
    if (argc < 2 || !strcmp(argv[1], "-")) {
        Bs_Result result = {0};

        Bs *bs = bs_new(false);
        if (!bs) {
            fprintf(stderr, "error: could not create BS instance\n");
            exit(1);
        }
        bs_core_init(bs, argc - 1, argv + 1);

        char line[1024];
        Bs_Writer *w = bs_stdout_get(bs);
        while (true) {
            bs_fmt(w, "> ");

            if (!fgets(line, sizeof(line), stdin)) {
                break;
            }

            result = bs_run(bs, "<stdin>", bs_sv_from_cstr(line), true);
            if (result.ok) {
                if (result.exit != -1) {
                    break;
                }

                bs_value_write(w, result.value);
                bs_fmt(w, "\n");
            }
        }

        bs_fmt(w, "\n");
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

    Bs *bs = bs_new(false);
    if (!bs) {
        fprintf(stderr, "error: could not create BS instance\n");
        free(contents);
        return 1;
    }
    bs_core_init(bs, argc - 1, argv + 1);

    const Bs_Result result = bs_run(bs, path, bs_sv_from_parts(contents, size), false);
    free(contents);

    bs_free(bs);
    return result.exit == -1 ? !result.ok : result.exit;
}
