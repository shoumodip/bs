#include "compiler.h"

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

    Vm *vm = vm_new();
    const ObjectFn *fn = compile(vm, path, (SV){contents, size});
    free(contents);

    if (!fn) {
        return_defer(1);
    }

    if (!vm_interpret(vm, fn, false)) {
        return_defer(1);
    }

defer:
    vm_free(vm);
    return result;
}
