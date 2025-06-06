# Flappy Bird

Flappy Bird using Raylib.

<!-- embed: examples/flappy_bird/raylib.c -->

Compile the raylib module.

```console
$ cc -o raylib.so -fPIC -shared raylib.c -lbs -lraylib    # On Linux
$ cc -o raylib.dylib -fPIC -shared raylib.c -lbs -lraylib # On macOS
$ cl /LD /Fe:raylib.dll raylib.c bs.lib raylib.lib        # On Windows
```

<blockquote>
Make sure to provide the compiler and linker flags as required.
</blockquote>

<!-- embed: examples/flappy_bird/flappy_bird.bs -->

Download and extract the
<a href="https://raw.githubusercontent.com/shoumodip/bs/refs/heads/main/examples/flappy_bird/assets.zip">assets</a>

Run the program.

```console
$ bs flappy_bird.bs
```

## Asset Sources (Reference)
<ul>
<li><a href="https://github.com/samuelcust/flappy-bird-assets">Images and Sound Effects</a></li>
<li><a href="https://pixabay.com/music/video-games-retro-game-arcade-short-236130/">Background Music</a></li>
</ul>
