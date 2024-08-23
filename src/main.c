#include "bs.h"

int main(int argc, char **argv) {
    int result = 0;

    if (argc < 2) {
        fprintf(stderr, "error: file path not provided\n");
        fprintf(stderr, "usage: %s <path>\n", *argv);
        exit(1);
    }
    const char *path = argv[1];

    size_t size = 0;
    char *contents = read_file(path, &size);
    if (!contents) {
        fprintf(stderr, "error: could not read file '%s'\n", path);
        exit(1);
    }

    Bs bs = {.compiler.lexer = lexer_new(path, (SV){contents, size})};

    const ObjectFn *fn = bs_compile(&bs);
    free(contents);

    if (!fn) {
        return_defer(1);
    }

    if (!bs_interpret(&bs, fn, false)) {
        bs_trace(&bs);
        return_defer(1);
    }

defer:
    bs_free(&bs);
    return result;
}
