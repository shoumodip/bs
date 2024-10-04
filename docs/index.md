<header>

# BS

<div class="only-desktop">

[Tutorial](tutorial.html)
[Stdlib](stdlib.html)
[GitHub](https://github.com/shoumodip/bs)

</div>
</header>

A minimal embeddable scripting language with sane defaults and strong typing

```bs
fn factorial(n) {
    if n < 2 {
        return 1;
    }

    return n * factorial(n - 1);
}

io.println("5! = \(factorial(5))");
```
```bsx
lit factorial(n) {
    ayo n < 2 {
        bet 1 fr
    }

    bet n * factorial(n - 1) fr
}

io.println("5! = \(factorial(5))") fr
```

```console
$ bs factorial.bs # or .bsx
5! = 120
```

## Download

```console
$ git clone https://github.com/shoumodip/bs
$ cd bs
$ ./build/linux.sh # If on Linux
$ ./build/macos.sh # If on macOS
$ build\windows.bat # If on Windows
```

<div class="only-mobile">

## Further Links

<center>

[Tutorial](tutorial.html)
[Stdlib](stdlib.html)
[GitHub](https://github.com/shoumodip/bs)

</center>
</div>
