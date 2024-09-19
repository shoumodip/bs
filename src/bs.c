#include <stdio.h>

#include "bs/core.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "error: file path not provided\n");
        fprintf(stderr, "usage: %s <file>\n", *argv);
        exit(1);
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

    const Bs_Result result = bs_run(bs, bs_sv_from_cstr(path), Bs_Sv(contents, size));
    free(contents);

    bs_free(bs);
    return result.exit == -1 ? !result.ok : result.exit;
}
