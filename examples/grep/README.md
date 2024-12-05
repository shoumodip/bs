# `grep`
The UNIX `grep` coreutil.

<!-- embed: examples/grep/grep.bs -->

```console
$ bs grep.bs '\.[A-z]+\(' grep.bs
grep.bs:3:13:     while !f.eof() {
grep.bs:4:21:         var line = f.readln()
grep.bs:6:23:         var col = line.find(pattern)
grep.bs:8:15:             io.println("{path}:{row + 1}:{col + 1}: {line}")
grep.bs:16:7:     io.eprintln("Usage: {os.args[0]} <pattern> [file...]")
grep.bs:17:7:     io.eprintln("Error: pattern not provided")
grep.bs:18:7:     os.exit(1)
grep.bs:23:7:     io.eprintln("Error: invalid pattern")
grep.bs:24:7:     os.exit(1)
grep.bs:36:15:     var f = io.Reader(path)
grep.bs:38:11:         io.println("Error: could not open file '{path}'")
grep.bs:44:6:     f.close()
grep.bs:47:3: os.exit(code)

$ cat grep.bs | bs grep.bs '\.[A-z]+\(' # DON'T CAT INTO GREP!!!
<stdin>:3:13:     while !f.eof() {
<stdin>:4:21:         var line = f.readln()
<stdin>:6:23:         var col = line.find(pattern)
<stdin>:8:15:             io.println("{path}:{row + 1}:{col + 1}: {line}")
<stdin>:16:7:     io.eprintln("Usage: {os.args[0]} <pattern> [file...]")
<stdin>:17:7:     io.eprintln("Error: pattern not provided")
<stdin>:18:7:     os.exit(1)
<stdin>:23:7:     io.eprintln("Error: invalid pattern")
<stdin>:24:7:     os.exit(1)
<stdin>:36:15:     var f = io.Reader(path)
<stdin>:38:11:         io.println("Error: could not open file '{path}'")
<stdin>:44:6:     f.close()
<stdin>:47:3: os.exit(code)
```
