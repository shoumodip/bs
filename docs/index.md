# BS

<a href='https://github.com/shoumodip/bs' class='github-icon'>
<svg viewBox='0 0 1024 1024' width='40' height='40'>
<path fill='#EEEEEE' d='M512 0C229.2 0 0 229.2 0 512c0 226.5 146.9 418.2 350.2 485.6 25.6 4.7 35-11.1 35-24.7v-86.8c-142.5 31-172.5-61.7-172.5-61.7-23.3-59.2-57-74.9-57-74.9-46.7-31.9 3.6-31.2 3.6-31.2 51.6 3.6 78.7 52.9 78.7 52.9 45.9 78.6 120.3 55.9 149.6 42.7 4.7-33.2 17.9-56 32.7-68.9-113.9-12.9-233.6-57-233.6-253.5 0-56 20-101.8 52.9-137.6-5.1-13-22.9-64.8 5.1-135.1 0 0 43.1-13.7 141.4 52.9 41-11.4 85.3-17 129.4-17 44.1 0 88.4 5.6 129.4 17 98.4-66.6 141.4-52.9 141.4-52.9 28.1 70.3 10.2 122.1 5.1 135.1 33 35.8 52.9 81.6 52.9 137.6 0 196.9-120 240.4-234.1 253.2 18.3 15.6 34.7 46.4 34.7 93.6v138.8c0 13.7 9.4 29.4 35 24.7C877.1 930.2 1024 738.5 1024 512 1024 229.2 794.8 0 512 0z'/>
</svg>
</a>

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
<a href="https://github.com/shoumodip/bs/releases/latest/download/bs-macos-x86_64.zip" download>MacOS<br>(x86_64)</a>
</div>
<div class="link">
<a href="https://github.com/shoumodip/bs/releases/latest/download/bs-windows-x86_64.zip" download>Windows<br>(x86_64)</a>
</div>
<div class="link">
<a href="https://github.com/shoumodip/bs/releases/latest/download/bs-linux-arm64.zip" download>Linux<br>(ARM64)</a>
</div>
<div class="link">
<a href="https://github.com/shoumodip/bs/releases/latest/download/bs-macos-arm64.zip" download>MacOS<br>(ARM64)</a>
</div>
<div class="link">
<a href="https://github.com/shoumodip/bs/releases/latest/download/bs-windows-arm64.zip" download>Windows<br>(ARM64)</a>
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

<!-- no-navigation -->
