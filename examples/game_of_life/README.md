# Game Of Life

An implementation of
<a href="https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life">Conway's Game Of Life</a>

<!-- embed: examples/game_of_life/GameOfLife.bs -->

## Console

<!-- embed: examples/game_of_life/game_of_life_tui.bs -->

Run the program.

```console
$ bs game_of_life_tui.bs
```

## Raylib

<!-- embed: examples/game_of_life/raylib.c -->

Compile the raylib module.

```console
$ cc -o raylib.so -fPIC -shared raylib.c -lbs -lraylib    # On Linux
$ cc -o raylib.dylib -fPIC -shared raylib.c -lbs -lraylib # On macOS
$ cl /LD /Fe:raylib.dll raylib.c bs.lib raylib.lib        # On Windows
```

Load it from BS.

<!-- embed: examples/game_of_life/game_of_life_raylib.bs -->

Run the program.

```console
$ bs game_of_life_raylib.bs
```
