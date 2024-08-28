#include "compiler.h"

static bool string_slice(Vm *vm, Value *args, size_t count, Value *result) {
    if (!vm_check_object_type(vm, args[0], OBJECT_STR, "argument #1")) {
        return false;
    }

    if (!vm_check_whole_number(vm, args[1], "argument #2")) {
        return false;
    }

    if (!vm_check_whole_number(vm, args[2], "argument #3")) {
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

typedef struct {
    FILE *file;
    const ObjectStr *path;
} FileData;

static void file_data_free(Vm *vm, void *data) {
    if (data) {
        fclose(((FileData *)data)->file);
        free(data);
    }
}

static void file_data_write(Writer *w, const void *data) {
    if (data) {
        w->fmt(w, "file<" SVFmt ">", SVArg(*((const FileData *)data)->path));
    } else {
        w->fmt(w, "file<>");
    }
}

static const NativeSpec file_spec = {
    .name = SVStatic("file"),
    .free = file_data_free,
    .write = file_data_write,
};

static bool io_open(Vm *vm, Value *args, size_t count, Value *result) {
    if (!vm_check_object_type(vm, args[0], OBJECT_STR, "argument #1")) {
        return false;
    }

    if (!vm_check_value_type(vm, args[1], VALUE_BOOL, "argument #2")) {
        return false;
    }

    const ObjectStr *path = (ObjectStr *)args[0].as.object;
    const bool write = args[1].as.boolean;

    size_t start;
    Writer *w = vm_writer_str_begin(vm, &start);
    w->fmt(w, SVFmt, SVArg(*path));

    SV path_native = vm_writer_str_end(vm, start);
    FILE *file = fopen(path_native.data, write ? "w" : "r");
    if (file) {
        FileData *data = malloc(sizeof(FileData));
        data->file = file;
        data->path = path;
        *result = value_object(object_native_data_new(vm, data, &file_spec));
    }

    return true;
}

static bool io_close(Vm *vm, Value *args, size_t count, Value *result) {
    if (!vm_check_object_native_type(vm, args[0], &file_spec, "argument #1")) {
        return false;
    }

    ObjectNativeData *native = (ObjectNativeData *)args[0].as.object;
    if (native->data) {
        file_data_free(vm, native->data);
        native->data = NULL;
    }

    return true;
}

static bool io_write(Vm *vm, Value *args, size_t count, Value *result) {
    if (!vm_check_object_native_type(vm, args[0], &file_spec, "argument #1")) {
        return false;
    }

    if (!vm_check_object_type(vm, args[1], OBJECT_STR, "argument #2")) {
        return false;
    }

    ObjectNativeData *native = (ObjectNativeData *)args[0].as.object;
    if (!native->data) {
        vm_error(vm, "cannot write into closed file");
        return false;
    }

    ObjectStr *bytes = (ObjectStr *)args[1].as.object;
    fwrite(bytes->data, bytes->size, 1, ((FileData *)native->data)->file);
    *result = value_bool(!ferror(native->data));

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
        value_object(object_native_fn_new(vm, string_slice, 3)));

    vm_native_define(vm, SVStatic("string"), value_object(string));

    ObjectTable *io = object_table_new(vm);
    object_table_set(
        vm,
        io,
        object_str_const(vm, "open", 4),
        value_object(object_native_fn_new(vm, io_open, 2)));

    object_table_set(
        vm,
        io,
        object_str_const(vm, "close", 5),
        value_object(object_native_fn_new(vm, io_close, 1)));

    object_table_set(
        vm,
        io,
        object_str_const(vm, "write", 5),
        value_object(object_native_fn_new(vm, io_write, 2)));

    vm_native_define(vm, SVStatic("io"), value_object(io));

    if (!vm_run(vm, path, (SV){contents, size}, false)) {
        return_defer(1);
    }

defer:
    vm_free(vm);
    free(contents);
    return result;
}
