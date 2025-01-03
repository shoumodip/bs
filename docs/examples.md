# Examples

<!-- home-icon -->

## `cat`
The UNIX `cat` coreutil.

```bs
// cat.bs

var code = 0

for i in 1..len(os.args) {
    const path = os.args[i]
    const contents = io.readfile(path)
    if !contents {
        io.eprintln("Error: could not read file '{path}'")
        code = 1
        continue
    }

    io.print(contents)
}

os.exit(code)
```

```console
$ bs cat.bs cat.bs # Poor man's quine
var code = 0

for i in 1..len(os.args) {
    const path = os.args[i]
    const contents = io.readfile(path)
    if !contents {
        io.eprintln("Error: could not read file '{path}'")
        code = 1
        continue
    }

    io.print(contents)
}

os.exit(code)
```

## `ls`
The UNIX `ls` coreutil.

```bs
// ls.bs

fn compare(a, b) {
    if a.suffix("/") && !b.suffix("/") {
        return true
    }

    if !a.suffix("/") && b.suffix("/") {
        return false
    }

    return a.compare(b) < 0
}

io.readdir(if len(os.args) >= 2 then os.args[1] else ".")
    .map(fn (e) -> e.name() $ if e.isdir() then "/" else "")
    .sort(compare)
    .map(io.println)
```

```console
$ bs ls.bs
README.md
ls.bs
```

## `grep`
The UNIX `grep` coreutil.

```bs
// grep.bs

fn grep(f, path, pattern) {
    var row = 0
    while !f.eof() {
        const line = f.readln()

        const col = line.find(pattern)
        if col {
            io.println("{path}:{row + 1}:{col + 1}: {line}")
        }

        row += 1
    }
}

if len(os.args) < 2 {
    io.eprintln("Usage: {os.args[0]} <pattern> [file...]")
    io.eprintln("Error: pattern not provided")
    os.exit(1)
}

const pattern = Regex(os.args[1])
if !pattern {
    io.eprintln("Error: invalid pattern")
    os.exit(1)
}

if len(os.args) == 2 {
    return grep(io.stdin, "<stdin>", pattern)
}

var code = 0

for i in 2..len(os.args) {
    const path = os.args[i]

    const f = io.Reader(path)
    if !f {
        io.println("Error: could not open file '{path}'")
        code = 1
        continue
    }

    grep(f, path, pattern)
    f.close()
}

os.exit(code)
```

```console
$ bs grep.bs '\.[A-z]+\(' grep.bs
grep.bs:3:13:     while !f.eof() {
grep.bs:4:23:         const line = f.readln()
grep.bs:6:25:         const col = line.find(pattern)
grep.bs:8:15:             io.println("{path}:{row + 1}:{col + 1}: {line}")
grep.bs:16:7:     io.eprintln("Usage: {os.args[0]} <pattern> [file...]")
grep.bs:17:7:     io.eprintln("Error: pattern not provided")
grep.bs:18:7:     os.exit(1)
grep.bs:23:7:     io.eprintln("Error: invalid pattern")
grep.bs:24:7:     os.exit(1)
grep.bs:33:12: for i in 2..len(os.args) {
grep.bs:36:17:     const f = io.Reader(path)
grep.bs:38:11:         io.println("Error: could not open file '{path}'")
grep.bs:44:6:     f.close()
grep.bs:47:3: os.exit(code)

$ cat grep.bs | bs grep.bs '\.[A-z]+\(' # DON'T CAT INTO GREP!!!
<stdin>:3:13:     while !f.eof() {
<stdin>:4:23:         const line = f.readln()
<stdin>:6:25:         const col = line.find(pattern)
<stdin>:8:15:             io.println("{path}:{row + 1}:{col + 1}: {line}")
<stdin>:16:7:     io.eprintln("Usage: {os.args[0]} <pattern> [file...]")
<stdin>:17:7:     io.eprintln("Error: pattern not provided")
<stdin>:18:7:     os.exit(1)
<stdin>:23:7:     io.eprintln("Error: invalid pattern")
<stdin>:24:7:     os.exit(1)
<stdin>:33:12: for i in 2..len(os.args) {
<stdin>:36:17:     const f = io.Reader(path)
<stdin>:38:11:         io.println("Error: could not open file '{path}'")
<stdin>:44:6:     f.close()
<stdin>:47:3: os.exit(code)
```

## Shell
A simple UNIX shell.

```bs
// shell.bs

const home = os.getenv("HOME")

// Return a pretty current working directory
fn pwd() {
    const cwd = os.getcwd()
    if cwd.prefix(home) {
        return "~" $ cwd.slice(len(home))
    }

    return cwd
}

// Rawdogging shell lexing with regex any%
const delim = Regex("[ \n\t]+")

// Previous working directory
var previous = nil

while !io.stdin.eof() {
    const args = io.input("{pwd()} $ ").split(delim)
    if len(args) == 0 {
        continue
    }

    const cmd = args[0]
    match cmd {
        // Builtin 'exit'
        // Usage:
        //   exit         -> Exits with 0
        //   exit <CODE>  -> Exits with CODE
        "exit" -> {
            if len(args) > 2 {
                io.eprintln("Error: too many arguments to command '{cmd}'")
                continue
            }

            const code = if len(args) == 2 then args[1].tonumber() else 0
            if !code {
                io.eprintln("Error: invalid exit code '{args[1]}'")
                continue
            }

            os.exit(code)
        }

        // Builtin 'cd'
        // Usage:
        //   cd        -> Go to HOME
        //   cd <DIR>  -> Go to DIR
        //   cd -      -> Go to previous location
        "cd" -> {
            if len(args) > 2 {
                io.eprintln("Error: too many arguments to command '{cmd}'")
                continue
            }

            var path = if len(args) == 2 then args[1] else "~"
            if path == "-" {
                if !previous {
                    io.eprintln("Error: no previous directory to go into")
                    continue
                }

                path = previous
            }

            if path.prefix("~") {
                path = home $ path.slice(1)
            }

            const current = os.getcwd()
            if !os.setcwd(path) {
                io.eprintln("Error: the directory '{path}' does not exist")
            }

            previous = current
        }
    } else {
        const p = os.Process(args)
        if !p {
            io.eprintln("Error: unknown command '{cmd}'")
            continue
        }
        p.wait()
    }
}

// In case of CTRL-d
io.println()
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
// rule110.bs

const board = [].resize(30).fill(0)
board[len(board) - 2] = 1

for i in 0..len(board) - 2 {
    for _, j in board {
        io.print(if j != 0 then "*" else " ")
    }
    io.println()

    var pattern = (board[0] << 1) | board[1]
    for j in 1..len(board) - 1 {
        pattern = ((pattern << 1) & 7) | board[j + 1]
        board[j] = (110 >> pattern) & 1
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
// GameOfLife.bs

class GameOfLife {
    init(width, height) {
        this.width = width
        this.height = height

        this.board = []
        this.buffer = []

        this.board.resize(width * height)
        this.buffer.resize(width * height)
    }

    get(x, y) {
        x %= this.width
        y %= this.height
        return this.board[y * this.width + x]
    }

    set(x, y, v) {
        x %= this.width
        y %= this.height
        this.board[y * this.width + x] = v
    }

    step() {
        fn set(x, y, v) {
            x %= this.width
            y %= this.height
            this.buffer[y * this.width + x] = v
        }

        fn nbors(x, y) {
            var count = 0
            for dy in -1..2 {
                for dx in -1..2 {
                    if dx == 0 && dy == 0 {
                        continue
                    }

                    if this.get(x + dx, y + dy) {
                        count += 1
                    }
                }
            }
            return count
        }

        for y in 0..this.height {
            for x in 0..this.width {
                const n = nbors(x, y)
                if n == 2 {
                    set(x, y, this.get(x, y))
                } else {
                    set(x, y, n == 3)
                }
            }
        }

        // Swap buffers
        const t = this.board
        this.board = this.buffer
        this.buffer = t
    }

    // (X, Y) is the center of the glider
    glider(x, y) {
        this.set(x + 0, y - 1, true)
        this.set(x + 1, y + 0, true)
        this.set(x - 1, y + 1, true)
        this.set(x + 0, y + 1, true)
        this.set(x + 1, y + 1, true)
    }
}

return GameOfLife
```

### Console

```bs
// game_of_life_tui.bs

const GameOfLife = import("GameOfLife")

const INTERVAL = 0.1

class GameOfLifeTUI < GameOfLife {
    init(width, height) {
        super.init(width, height)
    }

    show() {
        for y in 0..this.height {
            for x in 0..this.width {
                io.print(if this.get(x, y) then "#" else ".")
            }
            io.println()
        }
    }
}

const gol = GameOfLifeTUI(20, 10)
gol.glider(1, 1)

while true {
    gol.show()
    gol.step()
    io.print("\e[{gol.height}A\e[{gol.width}D")
    os.sleep(INTERVAL)
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

    // BS strings are null terminated by default for FFI convenience
    InitWindow(width, height, title->data);
    return bs_value_nil;
}

static Bs_Value rl_close_window(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    CloseWindow();
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

static Bs_Value rl_set_exit_key(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);
    SetExitKey(args[0].as.number);
    return bs_value_nil;
}

static Bs_Value rl_set_config_flags(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);
    SetConfigFlags(args[0].as.number);
    return bs_value_nil;
}

static Bs_Value rl_draw_line(Bs *bs, Bs_Value *args, size_t arity) {
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

static Bs_Value rl_draw_text(Bs *bs, Bs_Value *args, size_t arity) {
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

static Bs_Value rl_draw_rectangle(Bs *bs, Bs_Value *args, size_t arity) {
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

static Bs_Value rl_get_screen_width(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(GetScreenWidth());
}

static Bs_Value rl_get_screen_height(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(GetScreenHeight());
}

static Bs_Value rl_get_frame_time(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(GetFrameTime());
}

static Bs_Value rl_get_mouse_x(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(GetMouseX());
}

static Bs_Value rl_get_mouse_y(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_num(GetMouseY());
}

static Bs_Value rl_is_mouse_button_released(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);
    return bs_value_bool(IsMouseButtonReleased(args[0].as.number));
}

static Bs_Value rl_is_key_pressed(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);
    return bs_value_bool(IsKeyPressed(args[0].as.number));
}

static Bs_Value rl_measure_text(Bs *bs, Bs_Value *args, size_t arity) {
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

<blockquote>
Make sure to provide the compiler and linker flags as required.
</blockquote>

```bs
// game_of_life_raylib.bs

const rl = import("raylib")
const GameOfLife = import("GameOfLife")

const GRID = 0x5A524CFF
const BACKGROUND = 0x282828FF
const FOREGROUND = 0x89B482FF

const INTERVAL = 0.1
const FONT_SIZE = 30

var width = 800
var height = 600

var cell_size = 0
var padding_x = 0
var padding_y = 0

class GameOfLifeRaylib < GameOfLife {
    init(width, height) {
        super.init(width, height)
        this.clock = 0
        this.paused = false
    }

    show() {
        width = rl.get_screen_width()
        height = rl.get_screen_height() - FONT_SIZE * 1.2

        const cw = width / this.width
        const ch = height / this.height
        cell_size = cw.min(ch)

        padding_x = (width - this.width * cell_size) / 2
        padding_y = (height - this.height * cell_size) / 2

        rl.draw_rectangle(
            padding_x,
            padding_y,
            this.width * cell_size,
            this.height * cell_size,
            BACKGROUND)

        {
            const label = "Click to toggle cell, Space to play/pause"
            rl.draw_text(
                label,
                (width - rl.measure_text(label, FONT_SIZE)) / 2,
                this.height * cell_size + FONT_SIZE * 0.1,
                FONT_SIZE,
                FOREGROUND)
        }

        for i in 0..=this.width {
            const x = padding_x + i * cell_size
            rl.draw_line(x, 0, x, this.height * cell_size, GRID)
        }

        for i in 0..=this.height {
            const y = padding_y + i * cell_size
            rl.draw_line(padding_x, y, padding_x + this.width * cell_size, y, GRID)
        }

        for y in 0..this.height {
            for x in 0..this.width {
                if this.get(x, y) {
                    rl.draw_rectangle(
                        padding_x + x * cell_size,
                        padding_y + y * cell_size,
                        cell_size,
                        cell_size,
                        FOREGROUND)
                }
            }
        }
    }
}

rl.init_window(width, height, "Game Of Life")
rl.set_exit_key(ascii.code("Q"))
rl.set_config_flags(rl.FLAG_WINDOW_RESIZABLE)

const gol = GameOfLifeRaylib(20, 20)
gol.glider(2, 2)

while !rl.window_should_close() {
    rl.begin_drawing()
    rl.clear_background(0x181818FF)

    gol.show()

    if !gol.paused {
        gol.clock += rl.get_frame_time()
        if gol.clock >= INTERVAL {
            gol.clock = 0
            gol.step()
        }
    }

    if rl.is_key_pressed(ascii.code(" ")) {
        gol.paused = !gol.paused
    }

    if rl.is_mouse_button_released(rl.MOUSE_BUTTON_LEFT) {
        const x = ((rl.get_mouse_x() - padding_x) / cell_size).floor()
        const y = ((rl.get_mouse_y() - padding_y) / cell_size).floor()
        gol.set(x, y, !gol.get(x, y))
    }

    rl.end_drawing()
}

rl.close_window()
```

Run the program.

```console
$ bs game_of_life_raylib.bs
```

## CLI Task Management APP

```bs
// tasks.bs

const TASKS_PATH = os.getenv("HOME") $ "/.tasks"

class Tasks {
    init(path) {
        this.path = path
        this.tasks = (io.readfile(this.path) || "").split("\n")
    }

    add(title) {
        this.tasks.push(title)
        this.save()
        io.println("Added: {title}")
    }

    verify(index) {
        const total = len(this.tasks)
        if index >= total {
            io.eprintln("Error: invalid index {index} (total is {total})")
            os.exit(1)
        }
    }

    done(index) {
        this.verify(index)
        const title = delete(this.tasks[index])
        this.save()
        io.println("Done {index}: {title}")
    }

    edit(index, title) {
        this.verify(index)
        this.tasks[index] = title
        this.save()
        io.println("Edit {index}: {title}")
    }

    list(query) {
        for i, t in this.tasks {
            if query && !t.find(query) {
                continue
            }

            io.println("{i}: {t}")
        }
    }

    save() {
        const f = io.Writer(this.path)
        if !f {
            io.eprintln("Error: could not save tasks to '{this.path}'")
            os.exit(1)
        }

        f.write(this.tasks.join("\n"))
        f.close()
    }
}

fn usage(f) {
    f.writeln("Usage: tasks <command> [args...]")
    f.writeln("Commands:")
    f.writeln("    help                    Show this message and exit")
    f.writeln("    add  <title>            Add a task")
    f.writeln("    done <index>            Mark task as done")
    f.writeln("    edit <index> <title>    Edit a task")
    f.writeln("    list [query]            List tasks, with optional query")
}

if len(os.args) < 2 {
    return Tasks(TASKS_PATH).list(nil)
}

const command = os.args[1]
match command {
    "help" -> usage(io.stdout)

    "add" -> {
        if len(os.args) < 3 {
            io.eprintln("Error: task title not provided")
            io.eprintln("Usage: tasks add <title>")
            os.exit(1)
        }

        Tasks(TASKS_PATH).add(os.args[2])
    }

    "done" -> {
        if len(os.args) < 3 {
            io.eprintln("Error: task index not provided")
            io.eprintln("Usage: tasks done <index>")
            os.exit(1)
        }

        const index = os.args[2].tonumber()
        if !index {
            io.eprintln("Error: invalid index '{os.args[2]}'")
            os.exit(1)
        }

        Tasks(TASKS_PATH).done(index)
    }

    "edit" -> {
        if len(os.args) < 3 {
            io.eprintln("Error: task index not provided")
            io.eprintln("Usage: tasks edit <index> <title>")
            os.exit(1)
        }

        const index = os.args[2].tonumber()
        if !index {
            io.eprintln("Error: invalid index '{os.args[2]}'")
            os.exit(1)
        }

        if len(os.args) < 4 {
            io.eprintln("Error: task title not provided")
            io.eprintln("Usage: tasks edit <index> <title>")
            os.exit(1)
        }

        Tasks(TASKS_PATH).edit(index, os.args[3])
    }

    "list" -> {
        var query = nil
        if len(os.args) > 2 {
            query = Regex(os.args[2])
            if !query {
                io.eprintln("Error: invalid query '{os.args[2]}'")
                os.exit(1)
            }
        }

        Tasks(TASKS_PATH).list(query)
    }
} else {
    io.eprintln("Error: invalid command '{command}'")
    usage(io.stderr)
    os.exit(1)
}
```

```console
$ bs tasks.bs add "Finish reading book"
Added: Finish reading book

$ bs tasks.bs add "Submit project report"
Added: Submit project report

$ bs tasks.bs add "Submit homework"
Added: Submit homework

$ bs tasks.bs list
[0] Finish reading book
[1] Submit project report
[2] Submit homework

$ bs tasks.bs list 'Submit.*report'
[1] Submit project report

$ bs tasks.bs done 0
Done: Finish reading book

$ bs tasks.bs list
[0] Submit project report
[1] Submit homework
```

## Flappy Bird

Flappy Bird using Raylib.

```c
// raylib.c

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
```

Compile the raylib module.

```console
$ cc -o raylib.so -fPIC -shared raylib.c -lbs -lraylib    # On Linux
$ cc -o raylib.dylib -fPIC -shared raylib.c -lbs -lraylib # On macOS
$ cl /LD /Fe:raylib.dll raylib.c bs.lib raylib.lib        # On Windows
```

<blockquote>
Make sure to provide the compiler and linker flags as required.
</blockquote>

```bs
// flappy_bird.bs

const rl = import("raylib")
const game = {}

const WIDTH = 800
const HEIGHT = 600

const GRAVITY = 0.5
const IMPULSE = -7

const BIRD_TILT = 20
const BIRD_STARTOFF_SPEED = 7
const BIRD_ANIMATION_SPEED = 0.2

const BIRD_MIN_X = 100
const BIRD_MIN_Y = 50

const PIPE_GAP = 120
const PIPE_SPEED = -4
const PIPE_SPAWN_DELAY = 1.5
const PIPE_SPAWN_RANGE = HEIGHT / 5

const BACKGROUND_SCROLL_SPEED = 0.5

const MESSAGE_PADDING = 30
const OVER_MESSAGE_SCALE = 1.2
const TITLE_MESSAGE_SCALE = 1.2
const CONTINUE_MESSAGE_SCALE = 0.7

const BACKGROUND_MUSIC_DELAY = 1.2

const DEBUG_COLOR = 0xFF0000FF
const DEBUG_HITBOX = false

fn exit(code) {
    game.assets.unload()
    rl.close_audio_device()
    rl.close_window()
    os.exit(code)
}

class Assets {
    init() {
        this.sounds = {}
        this.textures = {}
    }

    sound(path, loader) {
        path = "assets/sounds/" $ path
        if path in this.sounds {
            return this.sounds[path]
        }

        const sound = loader(path)
        if !sound {
            io.eprintln("Error: could not load sound '{path}'")
            exit(1)
        }

        this.sounds[path] = sound
        return sound
    }

    texture(path) {
        path = "assets/images/" $ path
        if path in this.textures {
            return this.textures[path]
        }

        const texture = rl.Texture(path)
        if !texture {
            io.eprintln("Error: could not load image '{path}'")
            exit(1)
        }

        this.textures[path] = texture
        return texture
    }

    unload() {
        for _, sound in this.sounds {
            sound.unload()
        }

        for _, texture in this.textures {
            texture.unload()
        }
    }
}

class Hitbox {
    init(x, y, width, height) {
        this.x = x
        this.y = y
        this.width = width
        this.height = height
    }

    move(x, y) {
        this.x = x
        this.y = y
    }

    debug() {
        if DEBUG_HITBOX {
            rl.draw_rectangle(this.x, this.y, this.width, this.height, DEBUG_COLOR)
        }
    }

    collides(that) {
        return this.x < that.x + that.width && this.x + this.width > that.x &&
            this.y < that.y + that.height && this.y + this.height > that.y
    }
}

class Bird {
    init() {
        this.dy = 0
        this.current = 0
        this.textures = [
            game.assets.texture("bird0.png"),
            game.assets.texture("bird1.png"),
            game.assets.texture("bird2.png"),
        ]

        const w = this.textures[0].width() * game.scale
        const h = this.textures[0].height() * game.scale
        this.hitbox = Hitbox((WIDTH - w) / 2, HEIGHT / 3, w, h)
    }

    update() {
        if game.started {
            if !game.over && this.hitbox.x > BIRD_MIN_X {
                this.hitbox.x -= BIRD_STARTOFF_SPEED
                this.dy -= 0.3 * GRAVITY
            }

            const needs_dy = !game.over || this.hitbox.y + this.hitbox.height * game.scale < game.base.y
            if needs_dy {
                this.dy += GRAVITY
            }

            if !game.over && rl.is_key_pressed(32) {
                this.dy = IMPULSE
            }

            if needs_dy {
                this.hitbox.y += this.dy
                if this.hitbox.y < BIRD_MIN_Y {
                    this.hitbox.y = BIRD_MIN_Y
                }
            }

            if this.hitbox.y + this.hitbox.height >= game.base.y {
                game.die()
            }
        }

        if !game.over {
            this.current += rl.get_frame_time()
            if this.current >= BIRD_ANIMATION_SPEED * len(this.textures) {
                this.current %= BIRD_ANIMATION_SPEED * len(this.textures)
            }
        }

        this.textures[(this.current / BIRD_ANIMATION_SPEED).floor()].draw(
            this.hitbox.x,
            this.hitbox.y,
            this.dy.sign() * BIRD_TILT,
            game.scale,
            0xFFFFFFFF)

        this.hitbox.debug()
    }
}

class Background {
    init() {
        this.x = 0
        this.texture = game.assets.texture("background.png")
        this.width = this.texture.width()
        game.scale = HEIGHT / this.texture.height()
    }

    update() {
        if !game.over {
            this.x -= BACKGROUND_SCROLL_SPEED
            if this.x < -this.width {
                this.x = 0
            }
        }

        var i = 0
        while this.x + this.width * i < WIDTH {
            this.texture.draw(this.x + this.width * i, 0, 0, game.scale, 0xFFFFFFFF)
            i += 1
        }
    }
}

class Base {
    init() {
        this.x = 0
        this.texture = game.assets.texture("base.png")
        this.width = this.texture.width()
        this.y = HEIGHT - this.texture.height() * game.scale * 0.9
    }

    update() {
        if !game.over {
            this.x += PIPE_SPEED
            if this.x < -this.width {
                this.x = 0
            }
        }

        var i = 0
        while this.x + this.width * i < WIDTH {
            this.texture.draw(
                this.x + this.width * i,
                this.y,
                0,
                game.scale,
                0xFFFFFFFF)

            i += 1
        }
    }
}

class Pipe {
    init(y) {
        this.dx = PIPE_SPEED
        this.texture = game.assets.texture("pipe.png")
        this.hitbox = Hitbox(
            WIDTH,
            y,
            this.texture.width() * game.scale,
            this.texture.height() * game.scale)

        this.scored = false
    }

    top() {
        return Hitbox(
            this.hitbox.x,
            this.hitbox.y - this.hitbox.height / 2,
            this.hitbox.width,
            this.hitbox.height)
    }

    bottom() {
        return Hitbox(
            this.hitbox.x,
            this.hitbox.y + this.hitbox.height / 2 + PIPE_GAP,
            this.hitbox.width,
            this.hitbox.height)
    }

    update() {
        if !game.over {
            this.hitbox.x += this.dx
            if this.hitbox.x + this.hitbox.width < BIRD_MIN_X && !this.scored {
                game.point()
                this.scored = true
            }
        }

        const top = this.top()
        this.texture.draw(top.x, top.y, 180, game.scale, 0xFFFFFFFF)

        const bottom = this.bottom()
        this.texture.draw(bottom.x, bottom.y, 0, game.scale, 0xFFFFFFFF)

        top.debug()
        bottom.debug()
    }
}

class Pipes {
    init() {
        this.items = []
        this.clock = 0
    }

    update() {
        if !game.started {
            return true
        }

        if !game.over {
            this.clock += rl.get_frame_time()
            if this.clock >= PIPE_SPAWN_DELAY {
                this.items.push(Pipe(game.rng.number(-PIPE_SPAWN_RANGE, PIPE_SPAWN_RANGE)))
                this.clock %= PIPE_SPAWN_DELAY
            }

            this.items = this.items.filter(fn (p) -> p.hitbox.x >= -p.hitbox.width)
        }

        for _, pipe in this.items {
            pipe.update()
            if !game.over &&
                (pipe.top().collides(game.bird.hitbox) ||
                 pipe.bottom().collides(game.bird.hitbox)) {
                return false
            }
        }

        return true
    }
}

class Score {
    init() {
        this.best = nil
        this.current = 0
        this.textures = [
            game.assets.texture("0.png"),
            game.assets.texture("1.png"),
            game.assets.texture("2.png"),
            game.assets.texture("3.png"),
            game.assets.texture("4.png"),
            game.assets.texture("5.png"),
            game.assets.texture("6.png"),
            game.assets.texture("7.png"),
            game.assets.texture("8.png"),
            game.assets.texture("9.png"),
        ]
    }

    reset() {
        this.current = 0
    }

    display() {
        if game.started {
            fn show(n, y, scale, tint) {
                const digits = []
                if n < 10 {
                    digits.push(n)
                } else {
                    while n != 0 {
                        digits.push(n % 10)
                        n = (n / 10).floor()
                    }
                }
                digits.reverse()

                scale *= game.scale
                const width = digits.reduce(fn (a, b) -> a + this.textures[b].width() * scale, 0)
                var x = (WIDTH - width) / 2
                for _, n in digits {
                    this.textures[n].draw(x, y, 0, scale, tint)
                    x += this.textures[n].width() * scale
                }
            }

            show(this.current, HEIGHT / 3.6, 1.0, 0xFFFFFFFF)
            if this.best {
                show(this.best, HEIGHT / 2.8, 0.7, 0xDDDDDDFF)
            }
        }
    }
}

rl.init_window(WIDTH, HEIGHT, "Hello from BS!")
rl.init_audio_device()
rl.set_target_fps(60)

game.assets = Assets()
game.started = false
game.over = false
game.rng = math.Random()

fn game.die() {
    if game.over {
        return
    }

    game.over = true
    if !game.score.best || game.score.current > game.score.best {
        game.score.best = game.score.current
    }

    game.die_sound.play()
    game.background_music_delay = BACKGROUND_MUSIC_DELAY
}

fn game.point() {
    game.score.current += 1
    game.point_sound.play()
}

fn game.start() {
    game.start_sound.play()
    game.background_music.toggle()
}

game.background = Background()
game.base = Base()
game.bird = Bird()
game.pipes = Pipes()
game.score = Score()

game.over_message = game.assets.texture("gameover.png")
game.title_message = game.assets.texture("title.png")
game.continue_message = game.assets.texture("continue.png")

game.die_sound = game.assets.sound("die.wav", rl.Sound)
game.point_sound = game.assets.sound("point.wav", rl.Sound)
game.start_sound = game.assets.sound("start.wav", rl.Sound)
game.background_music = game.assets.sound("background.mp3", rl.Music)

game.background_music_delay = 0

while !rl.window_should_close() {
    rl.begin_drawing()
    game.background.update()

    if game.background_music_delay > 0 {
        game.background_music_delay -= rl.get_frame_time()
        if game.background_music_delay <= 0 {
            game.background_music.toggle()
        }
    }

    game.background_music.update()

    if !game.pipes.update() {
        game.die()
    }

    game.base.update()
    game.bird.update()

    if !game.started {
        const x = (WIDTH - game.title_message.width() * TITLE_MESSAGE_SCALE) / 2
        game.title_message.draw(x, HEIGHT / 7, 0, TITLE_MESSAGE_SCALE, 0xFFFFFFFF)

        const x = (WIDTH - game.continue_message.width() * CONTINUE_MESSAGE_SCALE) / 2
        game.continue_message.draw(
            x,
            HEIGHT / 3 + game.continue_message.height() * CONTINUE_MESSAGE_SCALE + MESSAGE_PADDING,
            0,
            CONTINUE_MESSAGE_SCALE,
            0xFFFFFFFF)

        if rl.is_key_pressed(32) {
            game.started = true
            game.bird.dy = IMPULSE
            game.start()
        }
    }

    if game.over {
        const x = (WIDTH - game.over_message.width() * OVER_MESSAGE_SCALE) / 2
        game.over_message.draw(x, HEIGHT / 7, 0, OVER_MESSAGE_SCALE, 0xFFFFFFFF)

        if game.background_music_delay <= 0 {
            const x = (WIDTH - game.continue_message.width() * CONTINUE_MESSAGE_SCALE) / 2
            game.continue_message.draw(
                x,
                HEIGHT / 3 + game.continue_message.height() * CONTINUE_MESSAGE_SCALE + MESSAGE_PADDING,
                0,
                CONTINUE_MESSAGE_SCALE,
                0xFFFFFFFF)

            if rl.is_key_pressed(32) {
                game.over = false
                game.score.reset()

                game.bird = Bird()
                game.pipes = Pipes()

                game.bird.dy = IMPULSE
                game.start()
            }
        }
    }

    game.score.display()
    rl.end_drawing()
}

exit(0)
```

Download and extract the
<a href="https://raw.githubusercontent.com/shoumodip/bs/refs/heads/main/examples/flappy_bird/assets.zip">assets</a>

Run the program.

```console
$ bs flappy_bird.bs
```

### Asset Sources (Reference)
<ul>
<li><a href="https://github.com/samuelcust/flappy-bird-assets">Images and Sound Effects</a></li>
<li><a href="https://pixabay.com/music/video-games-retro-game-arcade-short-236130/">Background Music</a></li>
</ul>
