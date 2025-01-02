#include <bs/object.h>
#include <raylib.h>

static Bs_Value rl_init_window(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_arg_check_whole_number(bs, args, 0);
    bs_arg_check_whole_number(bs, args, 1);
    bs_arg_check_object_type(bs, args, 2, BS_OBJECT_STR);

    const int width = args[0].as.number;
    const int height = args[1].as.number;
    const Bs_Str *title = (const Bs_Str *)args[2].as.object;

    InitWindow(width, height, title->data);
    return bs_value_nil;
}

static Bs_Value rl_init_audio_device(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    InitAudioDevice();
    return bs_value_nil;
}

static Bs_Value rl_set_target_fps(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);
    SetTargetFPS(args[0].as.number);
    return bs_value_nil;
}

static Bs_Value rl_close_window(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    CloseWindow();
    return bs_value_nil;
}

static Bs_Value rl_close_audio_device(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    CloseAudioDevice();
    return bs_value_nil;
}

static Bs_Value rl_window_should_close(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_bool(WindowShouldClose());
}

static Bs_Value rl_begin_drawing(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    BeginDrawing();
    return bs_value_nil;
}

static Bs_Value rl_end_drawing(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    EndDrawing();
    return bs_value_nil;
}

static Bs_Value rl_clear_background(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);
    ClearBackground(GetColor(args[0].as.number));
    return bs_value_nil;
}

static Bs_Value rl_draw_rectangle(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 5);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 2, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 3, BS_VALUE_NUM);
    bs_arg_check_whole_number(bs, args, 4);

    const int x = args[0].as.number;
    const int y = args[1].as.number;
    const int width = args[2].as.number;
    const int height = args[3].as.number;
    const Color color = GetColor(args[4].as.number);

    DrawRectangleLines(x, y, width, height, color);
    return bs_value_nil;
}

static Bs_Value rl_is_key_pressed(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);
    return bs_value_bool(IsKeyPressed(args[0].as.number));
}

static Bs_Value rl_get_frame_time(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(GetFrameTime());
}

static Bs_Value rl_texture_init(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *path = (const Bs_Str *)args[0].as.object;

    Texture texture = LoadTexture(path->data);
    if (!IsTextureValid(texture)) {
        return bs_value_nil;
    }

    bs_this_c_instance_data_as(args, Texture) = texture;
    return args[-1];
}

static Bs_Value rl_texture_draw(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 5);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 2, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 3, BS_VALUE_NUM);
    bs_arg_check_whole_number(bs, args, 4);

    const float scale = args[3].as.number;

    const Texture texture = bs_this_c_instance_data_as(args, Texture);
    const Rectangle src = {0, 0, texture.width, texture.height};
    const Rectangle dst = {
        args[0].as.number + (texture.width * scale) / 2.0,
        args[1].as.number + (texture.height * scale) / 2.0,
        texture.width * scale,
        texture.height * scale,
    };

    const Vector2 origin = {texture.width * scale / 2.0, texture.height * scale / 2.0};
    const float rotation = args[2].as.number;
    const Color tint = GetColor(args[4].as.number);

    DrawTexturePro(texture, src, dst, origin, rotation, tint);
    return bs_value_nil;
}

static Bs_Value rl_texture_width(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(bs_this_c_instance_data_as(args, Texture).width);
}

static Bs_Value rl_texture_height(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(bs_this_c_instance_data_as(args, Texture).height);
}

static Bs_Value rl_texture_unload(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    UnloadTexture(bs_this_c_instance_data_as(args, Texture));
    return bs_value_nil;
}

static Bs_Value rl_sound_init(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *path = (const Bs_Str *)args[0].as.object;

    Sound sound = LoadSound(path->data);
    if (!IsSoundValid(sound)) {
        return bs_value_nil;
    }

    bs_this_c_instance_data_as(args, Sound) = sound;
    return args[-1];
}

static Bs_Value rl_sound_play(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    PlaySound(bs_this_c_instance_data_as(args, Sound));
    return bs_value_nil;
}

static Bs_Value rl_sound_unload(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    UnloadSound(bs_this_c_instance_data_as(args, Sound));
    return bs_value_nil;
}

static Bs_Value rl_music_init(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);

    const Bs_Str *path = (const Bs_Str *)args[0].as.object;

    Music music = LoadMusicStream(path->data);
    if (!IsMusicValid(music)) {
        return bs_value_nil;
    }

    // An official binding would never do this, but do I care?
    music.looping = true;
    PlayMusicStream(music);
    SetMusicVolume(music, 0.2);

    bs_this_c_instance_data_as(args, Music) = music;
    return args[-1];
}

static Bs_Value rl_music_toggle(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    Music music = bs_this_c_instance_data_as(args, Music);
    if (IsMusicStreamPlaying(music)) {
        StopMusicStream(music);
    } else {
        PlayMusicStream(music);
    }
    return bs_value_nil;
}

static Bs_Value rl_music_update(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    UpdateMusicStream(bs_this_c_instance_data_as(args, Music));
    return bs_value_nil;
}

static Bs_Value rl_music_unload(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    UnloadMusicStream(bs_this_c_instance_data_as(args, Music));
    return bs_value_nil;
}

BS_LIBRARY_INIT void bs_library_init(Bs *bs, Bs_C_Lib *library) {
    static const Bs_FFI ffi[] = {
        {"init_window", rl_init_window},
        {"init_audio_device", rl_init_audio_device},
        {"set_target_fps", rl_set_target_fps},
        {"close_window", rl_close_window},
        {"close_audio_device", rl_close_audio_device},
        {"window_should_close", rl_window_should_close},
        {"begin_drawing", rl_begin_drawing},
        {"end_drawing", rl_end_drawing},
        {"clear_background", rl_clear_background},
        {"draw_rectangle", rl_draw_rectangle},
        {"is_key_pressed", rl_is_key_pressed},
        {"get_frame_time", rl_get_frame_time},
    };
    bs_c_lib_ffi(bs, library, ffi, bs_c_array_size(ffi));

    Bs_C_Class *texture_class =
        bs_c_class_new(bs, Bs_Sv_Static("Texture"), sizeof(Texture), rl_texture_init);

    texture_class->can_fail = true;
    bs_c_class_add(bs, texture_class, Bs_Sv_Static("draw"), rl_texture_draw);
    bs_c_class_add(bs, texture_class, Bs_Sv_Static("width"), rl_texture_width);
    bs_c_class_add(bs, texture_class, Bs_Sv_Static("height"), rl_texture_height);
    bs_c_class_add(bs, texture_class, Bs_Sv_Static("unload"), rl_texture_unload);
    bs_c_lib_set(bs, library, texture_class->name, bs_value_object(texture_class));

    Bs_C_Class *sound_class =
        bs_c_class_new(bs, Bs_Sv_Static("Sound"), sizeof(Sound), rl_sound_init);

    sound_class->can_fail = true;
    bs_c_class_add(bs, sound_class, Bs_Sv_Static("play"), rl_sound_play);
    bs_c_class_add(bs, sound_class, Bs_Sv_Static("unload"), rl_sound_unload);
    bs_c_lib_set(bs, library, sound_class->name, bs_value_object(sound_class));

    Bs_C_Class *music_class =
        bs_c_class_new(bs, Bs_Sv_Static("Music"), sizeof(Music), rl_music_init);

    music_class->can_fail = true;
    bs_c_class_add(bs, music_class, Bs_Sv_Static("toggle"), rl_music_toggle);
    bs_c_class_add(bs, music_class, Bs_Sv_Static("update"), rl_music_update);
    bs_c_class_add(bs, music_class, Bs_Sv_Static("unload"), rl_music_unload);
    bs_c_lib_set(bs, library, music_class->name, bs_value_object(music_class));
}
