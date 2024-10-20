# `cat`
The UNIX `cat` coreutil.

<!-- embed: examples/cat/cat.bs -->

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
