# [Conway's Game Of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life)

## Console Version

```console
$ ./game_of_life_tui.bs
```

## Raylib Version
Compile the raylib module.

```console
$ cc -o raylib.so -fPIC -shared raylib.c -lbs -lraylib    # On Linux
$ cc -o raylib.dylib -fPIC -shared raylib.c -lbs -lraylib # On macOS
$ cl /LD /Fe:raylib.dll raylib.c bs.lib raylib.lib        # On Windows
```

Run the program.

```console
$ ./game_of_life_raylib.bs
```
