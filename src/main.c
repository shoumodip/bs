#include <stdio.h>

#include "bs/core.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "error: file path not provided\n");
        fprintf(stderr, "usage: %s <path>\n", *argv);
        exit(1);
    }
    const char *path = argv[1];

    size_t size = 0;
    char *contents = bs_read_file(path, &size);
    if (!contents) {
        fprintf(stderr, "error: could not read file '%s'\n", path);
        exit(1);
    }

    Bs *bs = bs_new();
    bs_core_init(bs, argc - 1, argv + 1);

    const int result = bs_run(bs, path, bs_sv_from_parts(contents, size), false);

    bs_free(bs);
    free(contents);
    return result;
}
