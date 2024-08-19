#include <stdio.h>

#include "compiler.h"
#include "vm.h"

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

    VM vm = {0};
    Chunk chunk = {0};
    Compiler compiler = {
        .lexer = lexer_new(path, (SV){contents, size}),
        .gc = &vm.gc,
    };

    if (!compile(&compiler, &chunk)) {
        return_defer(1);
    }

    if (!vm_run(&vm, &chunk)) {
        return_defer(1);
    }

defer:
    vm_free(&vm);
    chunk_free(&chunk);

    free(contents);
    return result;
}
