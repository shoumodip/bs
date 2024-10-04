#include <bs/object.h>

Bs_Value bs_add(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);
    return bs_value_num(args[0].as.number + args[1].as.number);
}

Bs_Value bs_sub(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);
    return bs_value_num(args[0].as.number - args[1].as.number);
}

BS_LIBRARY_INIT void bs_library_init(Bs *bs, Bs_C_Lib *library) {
    static const Bs_FFI ffi[] = {
        {"add", bs_add},
        {"sub", bs_sub},
    };
    bs_c_lib_ffi(bs, library, ffi, bs_c_array_size(ffi));
}
