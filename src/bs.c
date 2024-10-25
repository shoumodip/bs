#include <stdio.h>

#include "bs/core.h"

int main(int argc, char **argv) {
    if (argc < 2 || !strcmp(argv[1], "-")) {
        Bs_Result result = {0};

        Bs *bs = bs_new();
        bs_core_init(bs, argc - 1, argv + 1);

        static char line[8 * 1024]; // 8KB is enough for all repl related tasks
        Bs_Writer *w = &bs_config(bs)->log;
        while (true) {
            bs_fmt(w, "> ");
            if (!fgets(line, sizeof(line), stdin)) {
                break;
            }

            Bs_Sv input = bs_sv_from_cstr(line);
            if (bs_sv_prefix(input, Bs_Sv_Static(":!"))) {
                system(line + 2);
                continue;
            }

            if (bs_sv_eq(input, Bs_Sv_Static(":{\n"))) {
                do {
                    const size_t size = sizeof(line) - input.size;
                    assert(size);

                    bs_fmt(w, "| ");
                    if (!fgets(line + input.size, size, stdin)) {
                        break;
                    }

                    input = bs_sv_from_cstr(line);
                } while (!bs_sv_suffix(input, Bs_Sv_Static(":}\n")));

                input.data += 3;
                input.size -= 6;
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

    Bs *bs = bs_new();
    bs_core_init(bs, argc - 1, argv + 1);

    const Bs_Result result = bs_run(bs, bs_sv_from_cstr(path), Bs_Sv(contents, size), false);
    free(contents);

    bs_free(bs);
    return result.exit == -1 ? !result.ok : result.exit;
}
