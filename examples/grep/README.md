# `grep`
The UNIX `grep` coreutil.

<!-- embed: examples/grep/grep.bs -->

```console
$ bs grep.bs '\.[A-z]+\(' grep.bs
grep.bs:3:13:     while !f.eof() {
grep.bs:4:21:         var line = f.readln();
grep.bs:6:23:         var col = line.find(pattern);
grep.bs:8:15:             io.println("\(path):\(row + 1):\(col + 1): \(line)");
grep.bs:14:6:     f.close();
grep.bs:18:7:     io.eprintln("Usage: \(os.args[0]) <pattern> [file...]");
grep.bs:19:7:     io.eprintln("Error: pattern not provided");
grep.bs:20:7:     os.exit(1);
grep.bs:25:7:     io.eprintln("Error: invalid pattern");
grep.bs:26:7:     os.exit(1);
grep.bs:38:15:     var f = io.Reader(path);
grep.bs:40:11:         io.println("Error: could not open file '\(path)'");
grep.bs:48:3: os.exit(code);

$ bs cat.bs cat.bs | bs grep.bs '\.[A-z]+\(' # DON'T CAT INTO GREP!!!
<stdin>:5:15:     var f = io.Reader(path);
<stdin>:7:11:         io.eprintln("Error: could not read file '\(path)'");
<stdin>:12:7:     io.print(f.read());
<stdin>:14:6:     f.close();
<stdin>:17:3: os.exit(code);
```
