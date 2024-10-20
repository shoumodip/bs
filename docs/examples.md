# Examples

## `cat`
The UNIX `cat` coreutil.

```bs
# cat.bs

var code = 0;

for i in 1, len(os.args) {
    var path = os.args[i];
    var f = io.Reader(path);
    if !f {
        io.eprintln("Error: could not read file '\(path)'");
        code = 1;
        continue;
    }

    io.print(f.read());
    f.close();
}

os.exit(code);
```
```bsx
# TODO: BSX will be removed soon
```

```console
$ bs cat.bs cat.bs # Poor man's quine
var code = 0;

for i in 1, len(os.args) {
    var path = os.args[i];
    var f = io.Reader(path);
    if !f {
        io.eprintln("Error: could not read file '\(path)'");
        code = 1;
        continue;
    }

    io.print(f.read());

    f.close();
}

os.exit(code);
```

## `grep`
The UNIX `grep` coreutil.

```bs
# grep.bs

fn grep(f, path, pattern) {
    var row = 0;
    while !f.eof() {
        var line = f.readln();

        var col = line.find(pattern);
        if col {
            io.println("\(path):\(row + 1):\(col + 1): \(line)");
        }

        row += 1;
    }
}

if len(os.args) < 2 {
    io.eprintln("Usage: \(os.args[0]) <pattern> [file...]");
    io.eprintln("Error: pattern not provided");
    os.exit(1);
}

var pattern = Regex(os.args[1]);
if !pattern {
    io.eprintln("Error: invalid pattern");
    os.exit(1);
}

if len(os.args) == 2 {
    return grep(io.stdin, "<stdin>", pattern);
}

var code = 0;

for i in 2, len(os.args) {
    var path = os.args[i];

    var f = io.Reader(path);
    if !f {
        io.println("Error: could not open file '\(path)'");
        code = 1;
        continue;
    }

    grep(f, path, pattern);
    f.close();
}

os.exit(code);
```
```bsx
# TODO: BSX will be removed soon
```

```console
$ bs grep.bs '\.[A-z]+\(' grep.bs
grep.bs:3:13:     while !f.eof() {
grep.bs:4:21:         var line = f.readln();
grep.bs:6:23:         var col = line.find(pattern);
grep.bs:8:15:             io.println("\(path):\(row + 1):\(col + 1): \(line)");
grep.bs:14:6:     f.close();
grep.bs:18:7:     io.eprintln("Usage: \(os.args[0]) <pattern> [file...]");
grep.bs:19:7:     io.eprintln("Error: pattern not provided");
grep.bs:20:7:     os.exit(1);
grep.bs:25:7:     io.eprintln("Error: invalid pattern");
grep.bs:26:7:     os.exit(1);
grep.bs:38:15:     var f = io.Reader(path);
grep.bs:40:11:         io.println("Error: could not open file '\(path)'");
grep.bs:48:3: os.exit(code);

$ bs cat.bs cat.bs | bs grep.bs '\.[A-z]+\(' # DON'T CAT INTO GREP!!!
<stdin>:5:15:     var f = io.Reader(path);
<stdin>:7:11:         io.eprintln("Error: could not read file '\(path)'");
<stdin>:12:7:     io.print(f.read());
<stdin>:14:6:     f.close();
<stdin>:17:3: os.exit(code);
```

## Shell
A simple UNIX shell.

```bs
# shell.bs

var home = os.getenv("HOME");

# Return a pretty current working directory
fn pwd() {
    var cwd = os.getcwd();
    if cwd.prefix(home) {
        return "~" ++ cwd.slice(len(home));
    }

    return cwd;
}

# Rawdogging shell lexing with regex any%
var delim = Regex("[ \n\t]+");

# Previous working directory
var previous = nil;

while !io.stdin.eof() {
    var args = io.input("\(pwd()) $ ").split(delim);
    if len(args) == 0 {
        continue;
    }

    var cmd = args[0];

    # Builtin 'exit'
    # Usage:
    #   exit         -> Exits with 0
    #   exit <CODE>  -> Exits with CODE
    if cmd == "exit" {
        if len(args) > 2 {
            io.eprintln("Error: too many arguments to command '\(cmd)'");
            continue;
        }

        var code = if len(args) == 2 then args[1].tonumber() else 0;
        if !code {
            io.eprintln("Error: invalid exit code '\(args[1])'");
            continue;
        }

        os.exit(code);
    }

    # Builtin 'cd'
    # Usage:
    #   cd        -> Go to HOME
    #   cd <DIR>  -> Go to DIR
    #   cd -      -> Go to previous location
    if cmd == "cd" {
        if len(args) > 2 {
            io.eprintln("Error: too many arguments to command '\(cmd)'");
            continue;
        }

        var path = if len(args) == 2 then args[1] else "~";
        if path == "-" {
            if !previous {
                io.eprintln("Error: no previous directory to go into");
                continue;
            }

            path = previous;
        }

        if path.prefix("~") {
            path = home ++ path.slice(1);
        }

        var current = os.getcwd();
        if !os.setcwd(path) {
            io.eprintln("Error: the directory '\(path)' does not exist");
        }

        previous = current;
        continue;
    }

    var p = os.Process(args);
    if !p {
        io.eprintln("Error: unknown command '\(cmd)'");
        continue;
    }
    p.wait();
}

# In case of CTRL-d
io.println();
```
```bsx
# TODO: BSX will be removed soon
```

```console
$ bs shell.bs
~/Git/bs/examples/shell $ ls -a
.  ..  README.md  shell.bs  shell.bsx
```

Use
<a href="https://github.com/hanslub42/rlwrap"><code>rlwrap</code></a>
in order to get readline bindings and filename autocompletion.

```console
$ rlwrap -c bs shell.bs # Like so
```

## Game Of Life

An implementation of
<a href="https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life">Conway's Game Of Life</a>

```bs
# GameOfLife.bs

class GameOfLife {
    init(width, height) {
        this.width = width;
        this.height = height;

        this.board = [];
        this.buffer = [];

        this.board.resize(width * height);
        this.buffer.resize(width * height);
    }

    get(x, y) {
        x %= this.width;
        y %= this.height;
        return this.board[y * this.width + x];
    }

    set(x, y, v) {
        x %= this.width;
        y %= this.height;
        this.board[y * this.width + x] = v;
    }

    step() {
        fn set(x, y, v) {
            x %= this.width;
            y %= this.height;
            this.buffer[y * this.width + x] = v;
        }

        fn nbors(x, y) {
            var count = 0;
            for dy in -1, 2 {
                for dx in -1, 2 {
                    if dx == 0 && dy == 0 {
                        continue;
                    }

                    if this.get(x + dx, y + dy) {
                        count += 1;
                    }
                }
            }
            return count;
        }

        for y in 0, this.height {
            for x in 0, this.width {
                var n = nbors(x, y);
                if n == 2 {
                    set(x, y, this.get(x, y));
                } else {
                    set(x, y, n == 3);
                }
            }
        }

        # Swap buffers
        var t = this.board;
        this.board = this.buffer;
        this.buffer = t;
    }

    # (X, Y) is the center of the glider
    glider(x, y) {
        this.set(x + 0, y - 1, true);
        this.set(x + 1, y + 0, true);
        this.set(x - 1, y + 1, true);
        this.set(x + 0, y + 1, true);
        this.set(x + 1, y + 1, true);
    }
}

return GameOfLife;
```
```bsx
# TODO: BSX will be removed soon
```

### Console

```bs
# game_of_life_tui.bs

var GameOfLife = import("GameOfLife");

var INTERVAL = 0.1;

class GameOfLifeTUI < GameOfLife {
    init(width, height) {
        super.init(width, height);
    }

    show() {
        for y in 0, this.height {
            for x in 0, this.width {
                io.print(if this.get(x, y) then "#" else ".");
            }
            io.println();
        }
    }
}

var gol = GameOfLifeTUI(20, 10);
gol.glider(1, 1);

while true {
    gol.show();
    gol.step();
    io.print("\e[\(gol.height)A\e[\(gol.width)D");
    os.sleep(INTERVAL);
}
```
```bsx
# TODO: BSX will be removed soon
```

Run the program.

```console
$ bs game_of_life_tui.bs
```

### Raylib

```c
// raylib.c

#include <math.h>

#include "bs/object.h"
#include "raylib.h"

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
        {"measure_text", rl_measure_text},
    };
    bs_c_lib_ffi(bs, library, ffi, bs_c_array_size(ffi));

    bs_c_lib_set(bs, library, Bs_Sv_Static("MOUSE_BUTTON_LEFT"), bs_value_num(MOUSE_BUTTON_LEFT));

    bs_c_lib_set(
        bs, library, Bs_Sv_Static("FLAG_WINDOW_RESIZABLE"), bs_value_num(FLAG_WINDOW_RESIZABLE));
}
```

Compile the raylib module.

```console
$ cc -o raylib.so -fPIC -shared raylib.c -lbs -lraylib    # On Linux
$ cc -o raylib.dylib -fPIC -shared raylib.c -lbs -lraylib # On macOS
$ cl /LD /Fe:raylib.dll raylib.c bs.lib raylib.lib        # On Windows
```

Load it from BS.

```bs
# game_of_life_raylib.bs

var rl = import("raylib");
var GameOfLife = import("GameOfLife");

var GRID = 0x5A524CFF;
var BACKGROUND = 0x282828FF;
var FOREGROUND = 0x89B482FF;

var INTERVAL = 0.1;
var FONT_SIZE = 40;

var width = 800;
var height = 600;

var cell_size = 0;
var padding_x = 0;
var padding_y = 0;

class GameOfLifeRaylib < GameOfLife {
    init(width, height) {
        super.init(width, height);
        this.clock = 0;
    }

    show() {
        width = rl.get_screen_width();
        height = rl.get_screen_height() - FONT_SIZE * 1.2;

        var cw = width / this.width;
        var ch = height / this.height;
        cell_size = cw.min(ch);

        padding_x = (width - this.width * cell_size) / 2;
        padding_y = (height - this.height * cell_size) / 2;

        rl.draw_rectangle(
            padding_x,
            padding_y,
            (this.width * cell_size),
            (this.height * cell_size),
            BACKGROUND);

        {
            var label = "Click to spawn glider";
            rl.draw_text(
                label,
                (width - rl.measure_text(label, FONT_SIZE)) / 2,
                (this.height * cell_size + FONT_SIZE * 0.1),
                FONT_SIZE,
                FOREGROUND);
        }

        for i in 0, this.width + 1 {
            var x = (padding_x + i * cell_size);
            rl.draw_line(x, 0, x, (this.height * cell_size), GRID);
        }

        for i in 0, this.height + 1 {
            var y = (padding_y + i * cell_size);
            rl.draw_line(padding_x, y, (padding_x + this.width * cell_size), y, GRID);
        }

        for y in 0, this.height {
            for x in 0, this.width {
                if this.get(x, y) {
                    rl.draw_rectangle(
                        (padding_x + x * cell_size),
                        (padding_y + y * cell_size),
                        cell_size,
                        cell_size,
                        FOREGROUND);
                }
            }
        }
    }
}

rl.init_window(width, height, "Game Of Life");
rl.set_exit_key(ascii.code("Q"));
rl.set_config_flags(rl.FLAG_WINDOW_RESIZABLE);

var gol = GameOfLifeRaylib(20, 20);
gol.glider(2, 2);

while !rl.window_should_close() {
    rl.begin_drawing();
    rl.clear_background(0x181818FF);

    gol.show();

    gol.clock += rl.get_frame_time();
    if gol.clock >= INTERVAL {
        gol.clock = 0;
        gol.step();
    }

    if rl.is_mouse_button_released(rl.MOUSE_BUTTON_LEFT) {
        var x = ((rl.get_mouse_x() - padding_x) / cell_size).round();
        var y = ((rl.get_mouse_y() - padding_y) / cell_size).round();
        gol.glider(x, y);
    }

    rl.end_drawing();
}

rl.close_window();
```
```bsx
# TODO: BSX will be removed soon
```

Run the program.

```console
$ bs game_of_life_raylib.bs
```
