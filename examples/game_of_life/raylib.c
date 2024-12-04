#include <math.h>

#include <bs/object.h>
#include <raylib.h>

Bs_Value rl_init_window(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_arg_check_whole_number(bs, args, 0);
    bs_arg_check_whole_number(bs, args, 1);
    bs_arg_check_object_type(bs, args, 2, BS_OBJECT_STR);

    const int width = args[0].as.number;
    const int height = args[1].as.number;
    const Bs_Str *title = (const Bs_Str *)args[2].as.object;

    // BS strings are null terminated by default for FFI convenience
    InitWindow(width, height, title->data);
    return bs_value_nil;
}

Bs_Value rl_close_window(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    CloseWindow();
    return bs_value_nil;
}

Bs_Value rl_window_should_close(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_bool(WindowShouldClose());
}

Bs_Value rl_begin_drawing(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    BeginDrawing();
    return bs_value_nil;
}

Bs_Value rl_end_drawing(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    EndDrawing();
    return bs_value_nil;
}

Bs_Value rl_clear_background(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);
    ClearBackground(GetColor(args[0].as.number));
    return bs_value_nil;
}

Bs_Value rl_set_exit_key(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);
    SetExitKey(args[0].as.number);
    return bs_value_nil;
}

Bs_Value rl_set_config_flags(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);
    SetConfigFlags(args[0].as.number);
    return bs_value_nil;
}

Bs_Value rl_draw_line(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 5);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 2, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 3, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 4, BS_VALUE_NUM);

    const int startPosX = round(args[0].as.number);
    const int startPosY = round(args[1].as.number);
    const int endPosX = round(args[2].as.number);
    const int endPosY = round(args[3].as.number);
    const Color color = GetColor(args[4].as.number);

    DrawLine(startPosX, startPosY, endPosX, endPosY, color);
    return bs_value_nil;
}

Bs_Value rl_draw_text(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 5);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 2, BS_VALUE_NUM);
    bs_arg_check_whole_number(bs, args, 3);
    bs_arg_check_whole_number(bs, args, 4);

    const Bs_Str *text = (const Bs_Str *)args[0].as.object;
    const int x = round(args[1].as.number);
    const int y = round(args[2].as.number);
    const int size = args[3].as.number;
    const Color color = GetColor(args[4].as.number);

    DrawText(text->data, x, y, size, color);
    return bs_value_nil;
}

Bs_Value rl_draw_rectangle(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 5);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 2, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 3, BS_VALUE_NUM);
    bs_arg_check_whole_number(bs, args, 4);

    const int posX = round(args[0].as.number);
    const int posY = round(args[1].as.number);
    const int width = round(args[2].as.number);
    const int height = round(args[3].as.number);
    const Color color = GetColor(args[4].as.number);

    DrawRectangle(posX, posY, width, height, color);
    return bs_value_nil;
}

Bs_Value rl_get_screen_width(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(GetScreenWidth());
}

Bs_Value rl_get_screen_height(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(GetScreenHeight());
}

Bs_Value rl_get_frame_time(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(GetFrameTime());
}

Bs_Value rl_get_mouse_x(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(GetMouseX());
}

Bs_Value rl_get_mouse_y(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(GetMouseY());
}

Bs_Value rl_is_mouse_button_released(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);
    return bs_value_bool(IsMouseButtonReleased(args[0].as.number));
}

Bs_Value rl_is_key_pressed(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);
    return bs_value_bool(IsKeyPressed(args[0].as.number));
}

Bs_Value rl_measure_text(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
    bs_arg_check_whole_number(bs, args, 1);

    const Bs_Str *text = (const Bs_Str *)args[0].as.object;
    const int size = args[1].as.number;

    return bs_value_num(MeasureText(text->data, size));
}

BS_LIBRARY_INIT void bs_library_init(Bs *bs, Bs_C_Lib *library) {
    static const Bs_FFI ffi[] = {
        {"init_window", rl_init_window},
        {"close_window", rl_close_window},
        {"window_should_close", rl_window_should_close},
        {"begin_drawing", rl_begin_drawing},
        {"end_drawing", rl_end_drawing},
        {"clear_background", rl_clear_background},
        {"set_exit_key", rl_set_exit_key},
        {"set_config_flags", rl_set_config_flags},
        {"draw_line", rl_draw_line},
        {"draw_text", rl_draw_text},
        {"draw_rectangle", rl_draw_rectangle},
        {"get_screen_width", rl_get_screen_width},
        {"get_screen_height", rl_get_screen_height},
        {"get_frame_time", rl_get_frame_time},
        {"get_mouse_x", rl_get_mouse_x},
        {"get_mouse_y", rl_get_mouse_y},
        {"is_mouse_button_released", rl_is_mouse_button_released},
        {"is_key_pressed", rl_is_key_pressed},
        {"measure_text", rl_measure_text},
    };
    bs_c_lib_ffi(bs, library, ffi, bs_c_array_size(ffi));

    bs_c_lib_set(bs, library, Bs_Sv_Static("MOUSE_BUTTON_LEFT"), bs_value_num(MOUSE_BUTTON_LEFT));

    bs_c_lib_set(
        bs, library, Bs_Sv_Static("FLAG_WINDOW_RESIZABLE"), bs_value_num(FLAG_WINDOW_RESIZABLE));
}
