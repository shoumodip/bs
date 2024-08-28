#include "compiler.h"

static bool string_slice(Vm *vm, Value *args, size_t count, Value *result) {
    if (count != 3) {
        vm_error(vm, "expected 3 arguments, got %zu", count);
        return false;
    }

    if (args[0].type != VALUE_OBJECT || args[0].as.object->type != OBJECT_STR) {
        vm_error(vm, "expected argument #1 to be string, got %s", value_type_name(args[0]));
        return false;
    }

    if (args[1].type != VALUE_NUM) {
        vm_error(vm, "expected argument #2 to be number, got %s", value_type_name(args[1]));
        return false;
    }

    if (args[1].as.number != (long)args[1].as.number) {
        vm_error(vm, "expected argument #2 to be whole number, got fractional value");
        return false;
    }

    if (args[2].type != VALUE_NUM) {
        vm_error(vm, "expected argument #3 to be number, got %s", value_type_name(args[2]));
        return false;
    }

    if (args[2].as.number != (long)args[2].as.number) {
        vm_error(vm, "expected argument #3 to be whole number, got fractional value");
        return false;
    }

    if (args[1].as.number < 0) {
        vm_error(vm, "expected argument #2 to be positive number, got negative value");
        return false;
    }

    if (args[2].as.number < 0) {
        vm_error(vm, "expected argument #3 to be positive number, got negative value");
        return false;
    }

    ObjectStr *str = (ObjectStr *)args[0].as.object;
    const size_t begin = min(args[1].as.number, args[2].as.number);
    const size_t end = max(args[1].as.number, args[2].as.number);

    if (begin >= str->size || end > str->size) {
        vm_error(vm, "cannot slice string of length %zu from %zu to %zu", str->size, begin, end);
        return false;
    }

    *result = value_object(object_str_new(vm, str->data + begin, end - begin));
    return true;
}

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

    ObjectTable *string = object_table_new(vm);
    object_table_set(
        vm,
        string,
        object_str_const(vm, "slice", 5),
        value_object(object_native_fn_new(vm, string_slice)));

    vm_native_define(vm, SVStatic("string"), value_object(string));

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
