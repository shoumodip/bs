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

<div class="only-mobile">

## Getting Started

<center>

[Tutorial](tutorial.html)
[Stdlib](stdlib.html)
[GitHub](https://github.com/shoumodip/bs)

</center>
</div>
