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

$ cat grep.bs | bs grep.bs '\.[A-z]+\(' # DON'T CAT INTO GREP!!!
<stdin>:3:13:     while !f.eof() {
<stdin>:4:21:         var line = f.readln();
<stdin>:6:23:         var col = line.find(pattern);
<stdin>:8:15:             io.println("\(path):\(row + 1):\(col + 1): \(line)");
<stdin>:16:7:     io.eprintln("Usage: \(os.args[0]) <pattern> [file...]");
<stdin>:17:7:     io.eprintln("Error: pattern not provided");
<stdin>:18:7:     os.exit(1);
<stdin>:23:7:     io.eprintln("Error: invalid pattern");
<stdin>:24:7:     os.exit(1);
<stdin>:36:15:     var f = io.Reader(path);
<stdin>:38:11:         io.println("Error: could not open file '\(path)'");
<stdin>:44:6:     f.close();
<stdin>:47:3: os.exit(code);
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

```console
$ bs shell.bs
~/Git/bs/examples/shell $ ls -a
.  ..  README.md  shell.bs
```

Use
<a href="https://github.com/hanslub42/rlwrap"><code>rlwrap</code></a>
in order to get readline bindings and filename autocompletion.

```console
$ rlwrap -c bs shell.bs # Like so
```

## Rule110

An implementation of
<a href="https://en.wikipedia.org/wiki/Rule_110">Rule110</a>
for proof of Turing Completeness.

```bs
# rule110.bs

var board = [].resize(30).fill(0);
board[len(board) - 2] = 1;

for i in 0, len(board) - 2 {
    for _, j in board {
        io.print(if j != 0 then "*" else " ");
    }
    io.println();

    var pattern = (board[0] << 1) | board[1];
    for j in 1, len(board) - 1 {
        pattern = ((pattern << 1) & 7) | board[j + 1];
        board[j] = (110 >> pattern) & 1;
    }
}
```

```console
$ bs rule110.bs
                            *
                           **
                          ***
                         ** *
                        *****
                       **   *
                      ***  **
                     ** * ***
                    ******* *
                   **     ***
                  ***    ** *
                 ** *   *****
                *****  **   *
               **   * ***  **
              ***  **** * ***
             ** * **  ***** *
            ******** **   ***
           **      ****  ** *
          ***     **  * *****
         ** *    *** ****   *
        *****   ** ***  *  **
       **   *  ***** * ** ***
      ***  ** **   ******** *
     ** * ******  **      ***
    *******    * ***     ** *
   **     *   **** *    *****
  ***    **  **  ***   **   *
 ** *   *** *** ** *  ***  **
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
```

Compile the raylib module.

```console
$ cc -o raylib.so -fPIC -shared raylib.c -lbs -lraylib    # On Linux
$ cc -o raylib.dylib -fPIC -shared raylib.c -lbs -lraylib # On macOS
$ cl /LD /Fe:raylib.dll raylib.c bs.lib raylib.lib        # On Windows
```

```bs
# game_of_life_raylib.bs

var rl = import("raylib");
var GameOfLife = import("GameOfLife");

var GRID = 0x5A524CFF;
var BACKGROUND = 0x282828FF;
var FOREGROUND = 0x89B482FF;

var INTERVAL = 0.1;
var FONT_SIZE = 30;

var width = 800;
var height = 600;

var cell_size = 0;
var padding_x = 0;
var padding_y = 0;

class GameOfLifeRaylib < GameOfLife {
    init(width, height) {
        super.init(width, height);
        this.clock = 0;
        this.paused = false;
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
            this.width * cell_size,
            this.height * cell_size,
            BACKGROUND);

        {
            var label = "Click to toggle cell, Space to play/pause";
            rl.draw_text(
                label,
                (width - rl.measure_text(label, FONT_SIZE)) / 2,
                this.height * cell_size + FONT_SIZE * 0.1,
                FONT_SIZE,
                FOREGROUND);
        }

        for i in 0, this.width + 1 {
            var x = padding_x + i * cell_size;
            rl.draw_line(x, 0, x, this.height * cell_size, GRID);
        }

        for i in 0, this.height + 1 {
            var y = padding_y + i * cell_size;
            rl.draw_line(padding_x, y, padding_x + this.width * cell_size, y, GRID);
        }

        for y in 0, this.height {
            for x in 0, this.width {
                if this.get(x, y) {
                    rl.draw_rectangle(
                        padding_x + x * cell_size,
                        padding_y + y * cell_size,
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

    if !gol.paused {
        gol.clock += rl.get_frame_time();
        if gol.clock >= INTERVAL {
            gol.clock = 0;
            gol.step();
        }
    }

    if rl.is_key_pressed(ascii.code(" ")) {
        gol.paused = !gol.paused;
    }

    if rl.is_mouse_button_released(rl.MOUSE_BUTTON_LEFT) {
        var x = ((rl.get_mouse_x() - padding_x) / cell_size).floor();
        var y = ((rl.get_mouse_y() - padding_y) / cell_size).floor();
        gol.set(x, y, !gol.get(x, y));
    }

    rl.end_drawing();
}

rl.close_window();
```

Run the program.

```console
$ bs game_of_life_raylib.bs
```

## CLI Task Management APP

```bs
# tasks.bs

var TASKS_PATH = os.getenv("HOME") ++ "/.tasks";

fn date(d) {
    if !d {
        return "";
    }

    var year = "\(d[0])".lpad("0", 4);
    var month = "\(d[1])".lpad("0", 2);
    var day = "\(d[2])".lpad("0", 2);
    return "\(year)-\(month)-\(day)";
}

fn flags(start, template) {
    var parsed = {};

    var i = start;
    while i < len(os.args) {
        var arg = os.args[i]; i += 1;
        if !arg.prefix("-") || !(arg.slice(1) in template) {
            io.eprintln("Error: unexpected argument '\(arg)'");
            io.eprintln("Expected:");

            var length = 0;
            for flag, _ in template {
                length = length.max(len(flag));
            }

            for flag, desc in template {
                io.eprintln("    -\(flag.rpad(" ", length))    \(desc)");
            }

            os.exit(1);
        }
        var flag = arg.slice(1);

        if i >= len(os.args) {
            io.eprintln("Error: value not provided for flag '\(arg)'");
            io.eprintln("Usage:");
            io.eprintln("    \(arg) <value>    \(template[flag])");
            os.exit(1);
        }
        var value = os.args[i]; i += 1;

        parsed[flag] = value;
    }

    return parsed;
}

fn currently(n, label) {
    if n == 1 {
        io.eprintln("Note: there is currently \(n) \(label)");
    } else {
        io.eprintln("Note: there are currently \(n) \(label)s");
    }
}

var FLAGS = {
    due = "The due date for the completion of the task. Expected format is 'YYYY-MM-DD'",
    tag = "The tags associated with the task. Expects a comma separated list of names",
    priority = "The priority of the task. Can be 'low', 'medium', or 'high' (Default: 'medium')",
};

var PRIORITIES = ["low", "medium", "high"];

class Task {
    init(title) {
        this.title = title;
        this.due = nil;
        this.tags = [];
        this.priority = 1;
    }

    set_due(date) {
        if len(date) == 0 {
            this.due = nil;
            return;
        }

        var format = "YYYY-MM-DD";
        if len(date) != len(format) {
            io.eprintln("Error: invalid due date '\(date)'. Expected format '\(format)'");
            os.exit(1);
        }

        var year = date.slice(0, 4).tonumber();
        var month = date.slice(5, 7).tonumber();
        var day = date.slice(8, 10).tonumber();

        if !year || !month || !day {
            io.eprintln("Error: invalid due date '\(date)'. Expected format '\(format)'");
            os.exit(1);
        }

        this.due = [year, month, day];
    }

    set_tag(labels) {
        for _, tag in labels.split(",") {
            this.tags.push(tag.trim(" "));
        }
    }

    set_priority(value) {
        this.priority = PRIORITIES.find(value);
        if !this.priority {
            io.eprintln("Error: invalid priority '\(value)'");
            io.eprintln("Expected:");
            io.eprintln("    low");
            io.eprintln("    medium    (Default)");
            io.eprintln("    high");
            os.exit(1);
        }
    }

    show() {
        var final = this.title;

        if this.due {
            final ++= " (Due: \(date(this.due)))";
        }

        final ++= " [Priority: \(PRIORITIES[this.priority])]";

        if len(this.tags) > 0 {
            final ++= " [Tags: \(this.tags.join(", "))]";
        }

        return final;
    }
}

class Tasks {
    init(path) {
        this.path = path;
        this.pending = [];
        this.completed = [];

        var f = io.Reader(path);
        if !f {
            return;
        }

        var lines = f.read().split("\n");
        var i = 0;
        while i < len(lines) {
            var line = lines[i]; i += 1;

            var task;
            if line.prefix("PENDING:") {
                task = Task(line.slice(len("PENDING:")));
                this.pending.push(task);
            } else if line.prefix("COMPLETED:") {
                task = Task(line.slice(len("COMPLETED:")));
                this.completed.push(task);
            }

            fn expect(expected) {
                if i >= len(lines) {
                    io.eprintln("Error: corrupted '\(path)'. Expected '\(expected)' on line \(i + 1), got EOF");
                    os.exit(1);
                }

                line = lines[i]; i += 1;
                if !line.prefix(expected) {
                    io.eprintln("Error: corrupted '\(path)'. Expected '\(expected)' on line \(i + 1), got '\(line)'");
                    os.exit(1);
                }
            }

            expect("DUE:");
            task.set_due(line.slice(len("DUE:")));

            expect("TAGS:");
            task.set_tag(line.slice(len("TAGS:")));

            expect("PRIORITY:");
            task.set_priority(line.slice(len("PRIORITY:")));
        }

        f.close();
    }

    add(title, flags) {
        var task = Task(title);

        if "due" in flags {
            task.set_due(flags.due);
        }

        if "tag" in flags {
            task.set_tag(flags.tag);
        }

        if "priority" in flags {
            task.set_priority(flags.priority);
        }

        this.pending.push(task);
        io.println("Added: \(task.show())");
    }

    list(flags) {
        var pending = false;

        var match = Task("");
        if "due" in flags {
            match.set_due(flags.due);
        }

        if "tag" in flags {
            match.set_tag(flags.tag);
        }

        if "priority" in flags {
            match.set_priority(flags.priority);
        }

        fn skip(task) {
            if "due" in flags && (!task.due || !task.due.equal(match.due)) {
                return true;
            }

            if "tag" in flags {
                for _, tag in match.tags {
                    if !task.tags.find(tag) {
                        return true;
                    }
                }
            }

            if "priority" in flags && task.priority != match.priority {
                return true;
            }

            return false;
        }

        for i, t in this.pending {
            if skip(t) {
                continue;
            }

            if !pending {
                pending = true;
                io.println("Pending Tasks:");
            }

            io.println("[\(i)]", t.show());
        }

        var completed = false;

        for i, t in this.completed {
            if skip(t) {
                continue;
            }

            if !completed {
                completed = true;

                if pending {
                    io.println();
                }
                io.println("Completed Tasks:");
            }

            io.println("[\(i)]", t.show());
        }
    }

    done(index) {
        if index >= len(this.pending) {
            io.eprintln("Error: invalid task index '\(index)'");
            currently(len(this.pending), "pending task");
            os.exit(1);
        }

        var task = this.pending.remove(index);
        this.completed.push(task);

        io.println("Completed: \(task.show())");
    }

    remove(index) {
        if index >= len(this.completed) {
            io.eprintln("Error: invalid task index '\(index)'");
            currently(len(this.completed), "completed task");
            os.exit(1);
        }

        var task = this.completed.remove(index);
        io.println("Removed: \(task.show())");
    }

    search(query) {
        var pending = false;

        for i, t in this.pending {
            if !t.title.find(query) {
                continue;
            }

            if !pending {
                pending = true;
                io.println("Pending Tasks:");
            }

            io.println("[\(i)]", t.show());
        }

        var completed = false;

        for i, t in this.completed {
            if !t.title.find(query) {
                continue;
            }

            if !completed {
                completed = true;

                if pending {
                    io.println();
                }
                io.println("Completed Tasks:");
            }

            io.println("[\(i)]", t.show());
        }
    }

    save() {
        var f = io.Writer(this.path);
        if !f {
            io.eprintln("Error: could not save tasks to file '\(this.path)'");
            os.exit(1);
        }

        var i = 0;
        fn save(task, prefix) {
            f.writeln("\(prefix):\(task.title)");
            f.writeln("DUE:\(date(task.due))");
            f.writeln("TAGS:\(task.tags.join(","))");
            f.writeln("PRIORITY:\(PRIORITIES[task.priority])");
        }

        i = 0;
        while i < len(this.pending) {
            var task = this.pending[i]; i += 1;
            save(task, "PENDING");
        }

        i = 0;
        while i < len(this.completed) {
            var task = this.completed[i]; i += 1;
            save(task, "COMPLETED");
        }

        f.close();
    }
}

if is_main_module {
    var tasks = Tasks(TASKS_PATH);

    if len(os.args) < 2 {
        io.eprintln("Error: command not provided");
        io.eprintln("Usage: \(os.args[0]) <command> [args...]");
        io.eprintln("Commands:");
        io.eprintln("    add");
        io.eprintln("    list");
        io.eprintln("    done");
        io.eprintln("    remove");
        io.eprintln("    search");
        os.exit(1);
    }
    var command = os.args[1];

    if command == "add" {
        if len(os.args) < 3 {
            io.eprintln("Error: task title not provided");
            io.eprintln("Usage: \(os.args[0]) add <title> [flags...]");
            os.exit(1);
        }

        tasks.add(os.args[2], flags(3, FLAGS));
        tasks.save();
        return;
    }

    if command == "list" {
        tasks.list(flags(2, FLAGS));
        return;
    }

    if command == "done" {
        if len(os.args) < 3 {
            io.eprintln("Error: task index not provided");
            io.eprintln("Usage: \(os.args[0]) done <index>");
            os.exit(1);
        }

        var index = os.args[2].tonumber();
        if !index {
            io.eprintln("Error: invalid task index '\(os.args[2])'");
            os.exit(1);
        }

        tasks.done(index);
        tasks.save();
        return;
    }

    if command == "remove" {
        if len(os.args) < 3 {
            io.eprintln("Error: task index not provided");
            io.eprintln("Usage: \(os.args[0]) remove <index>");
            os.exit(1);
        }

        var index = os.args[2].tonumber();
        if !index {
            io.eprintln("Error: invalid task index '\(os.args[2])'");
            os.exit(1);
        }

        tasks.remove(index);
        tasks.save();
        return;
    }

    if command == "search" {
        if len(os.args) < 3 {
            io.eprintln("Error: search query not provided");
            io.eprintln("Usage: \(os.args[0]) search <query>");
            os.exit(1);
        }

        var query = Regex(os.args[2]);
        if !query {
            io.eprintln("Error: invalid search query '\(os.args[2])'");
            os.exit(1);
        }

        tasks.search(query);
        return;
    }

    io.eprintln("Error: invalid command '\(command)'");
    io.eprintln("Available commands:");
    io.eprintln("    add");
    io.eprintln("    list");
    io.eprintln("    done");
    io.eprintln("    remove");
    io.eprintln("    search");
    os.exit(1);
}
```

```console
## Adding a new task (Default priority is medium)
$ bs tasks.bs add "Finish reading book"
Added: Finish reading book [Priority: medium]

## Adding a task with a high priority and tag
$ bs tasks.bs add "Submit project report" -due "2024-10-22" -priority high -tag work
Added: Submit project report (Due: 2024-10-22) [Priority: high] [Tags: work]

## Listing all tasks
$ bs tasks.bs list
Pending Tasks:
[0] Finish reading book [Priority: medium]
[1] Submit project report (Due: 2024-10-22) [Priority: high] [Tags: work]

## Marking a task as complete
$ bs tasks.bs done 1
Completed: Submit project report (Due: 2024-10-22) [Priority: high] [Tags: work]

## Listing all tasks
$ bs tasks.bs list
Pending Tasks:
[0] Finish reading book [Priority: medium]

Completed Tasks:
[0] Submit project report (Due: 2024-10-22) [Priority: high] [Tags: work]

## Filtering tasks by tag
$ bs tasks.bs list -tag work
Completed Tasks:
[0] Submit project report (Due: 2024-10-22) [Priority: high] [Tags: work]

## Remove a completed task
$ bs tasks.bs remove 0
Removed: Submit project report (Due: 2024-10-22) [Priority: high] [Tags: work]

$ bs tasks.bs add Finish
Added: Finish [Priority: medium]

$ bs tasks.bs add foo
Added: foo [Priority: medium]

$ bs tasks.bs add bar
Added: bar [Priority: medium]

$ bs tasks.bs done 2
Completed: Finish [Priority: medium]

## Search content of tasks using Regex
$ bs tasks.bs search '[fF]'
Pending Tasks:
[0] Finish reading book [Priority: medium]
[1] foo [Priority: medium]

Completed Tasks:
[0] Finish [Priority: medium]
```
