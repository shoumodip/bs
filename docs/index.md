# BS

<!-- github-icon -->

A minimal embeddable scripting language with sane defaults and strong typing

```bs
fn factorial(n) {
    if n < 2 {
        return 1
    }

    return n * factorial(n - 1)
}

io.println("5! = {factorial(5)}")
```

```console
$ bs factorial.bs
5! = 120
```

## Download

<div class="links">
<div class="link">
<a href="https://github.com/shoumodip/bs/releases/latest/download/bs-linux-x86_64.zip" download>Linux<br>(x86_64)</a>
</div>
<div class="link">
<a href="https://github.com/shoumodip/bs/releases/latest/download/bs-windows-x86_64.zip" download>Windows<br>(x86_64)</a>
</div>
<div class="link">
<a href="https://github.com/shoumodip/bs/releases/latest/download/bs-macos-arm64.zip" download>MacOS<br>(ARM64)</a>
</div>
</div>

## Further Links

<div class="links">
<div class="link">
<a href="tutorial.html">Tutorial</a>
</div>
<div class="link">
<a href="stdlib.html">Stdlib</a>
</div>
<div class="link">
<a href="examples.html">Examples</a>
</div>
</div>

## Editor Support

<ul>
<li><a href="https://github.com/shoumodip/bs.vim">Vim</a></li>
</ul>

<!-- no-navigation -->
