# `cat`
The UNIX `cat` coreutil.

<!-- embed: examples/cat/cat.bs -->

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
