# Stdlib

## I/O
Contains simple I/O primitives.

Note that I/O is binary in BS. On *nix systems it doesn't matter, but on the
video game OS it should be kept in mind.

### input(prompt?) @function
Read a line from standard input, printing the optional argument `prompt` if
provided.

```bs
var name = io.input("Enter your name> ");
io.println("Hello, \(name)!");
```
```bsx
mf name = io.input("Enter your name> ") fr
io.println("Hello, \(name)!") fr
```

```console
$ bs demo.bs
Enter your name> Batman
Hello, Batman!
```

### print(...) @function
Print all the arguments to standard output.

```bs
io.print("Hello", "world", 69);
```
```bsx
io.print("Hello", "world", 69) fr
```

```console
$ bs demo.bs
Hello, world 69⏎
```

The arguments are separated with spaces.

### println(...) @function
Print all the arguments to standard output, with a following newline.

### eprint(...) @function
Print all the arguments to standard error.

### eprintln(...) @function
Print all the arguments to standard error, with a following newline.

### Reader(path) @class
Native C class that opens `path` in readable mode.

Returns `nil` if failed.

```bs
var f = io.Reader("input.txt");
if !f {
    io.eprintln("Error: could not read file!");
    os.exit(1);
}

var contents = f.read();
io.print(contents);
```
```bsx
mf f = io.Reader("input.txt") fr
ayo nah f {
    io.eprintln("Error: could not read file!") fr
    os.exit(1) fr
}

mf contents = f.read() fr
io.print(contents) fr
```

Create a file `input.txt` with some sample text and run it.

```console
$ bs demo.bs
Just a test file
Nothing to see here
Foo
Bar
Baz
People's dreams have no end! ~ Blackbeard
```

#### Reader.close() @method
Close the file.

This is done automatically by the garbage collector, so can be omitted for
short programs.

#### Reader.read(count?) @method
Read `count` bytes from the current position. If the argument `count` is not
provided, it reads all of the available bytes.

Returns `nil` if failed.

```bs
var f = io.Reader("input.txt");
if !f {
    io.eprintln("Error: could not read file!");
    os.exit(1);
}

io.print("The first 16 bytes: [\(f.read(16))]\n");
io.print("The rest:", f.read());
```
```bsx
mf f = io.Reader("input.txt") fr
ayo nah f {
    io.eprintln("Error: could not read file!") fr
    os.exit(1) fr
}

io.print("The first 16 bytes: [\(f.read(16))]\n") fr
io.print("The rest:", f.read()) fr
```

```console
$ bs demo.bs
The first 16 bytes: [Just a test file]
The rest:
Nothing to see here
Foo
Bar
Baz
People's dreams have no end! ~ Blackbeard
```

#### Reader.readln() @method
Read a line.

```bs
var f = io.Reader("input.txt");
if !f {
    io.eprintln("Error: could not read file!");
    os.exit(1);
}

while !f.eof() {
    var line = f.readln();
    io.println("Line:", line);
}
```
```bsx
mf f = io.Reader("input.txt") fr
ayo nah f {
    io.eprintln("Error: could not read file!") fr
    os.exit(1) fr
}

yolo nah f.eof() {
    mf line = f.readln() fr
    io.println("Line:", line) fr
}
```

```console
$ bs demo.bs
Line: Just a test file
Line: Nothing to see here
Line: Foo
Line: Bar
Line: Baz
Line: People's dreams have no end! ~ Blackbeard
Line:
```

Note that on windows, the line will have a trailing `\r`, since I/O is binary
in BS. It is up to the user to decide whether to strip it or not.

#### Reader.eof() @method
Return whether the end of file has been reached.

#### Reader.seek(offset, whence) @method
Change the read position of the file.

Any of the following values can be used for `whence`.

- `SEEK_SET` - Seek from beginning of file
- `SEEK_CUR` - Seek from current position
- `SEEK_END` - Seek from end of file

Returns `true` if succeeded and `false` if failed

```bs
var f = io.Reader("input.txt");
if !f {
    io.eprintln("Error: could not read file!");
    os.exit(1);
}

io.print("The first 16 bytes: [\(f.read(16))]\n");

f.seek(5, io.SEEK_SET);

io.println("The full content offset by 5:");
io.print(f.read());
```
```bsx
mf f = io.Reader("input.txt") fr
ayo nah f {
    io.eprintln("Error: could not read file!") fr
    os.exit(1) fr
}

io.print("The first 16 bytes: [\(f.read(16))]\n") fr

f.seek(5, io.SEEK_SET) fr

io.println("The full content offset by 5:") fr
io.print(f.read()) fr
```

```console
$ bs demo.bs
The first 16 bytes: [Just a test file]
The full content offset by 5:
a test file
Nothing to see here
Foo
Bar
Baz
People's dreams have no end! ~ Blackbeard
```

#### Reader.tell() @method
Get the current position of the file.

Returns `nil` if failed

```bs
var f = io.Reader("input.txt");
if !f {
    io.eprintln("Error: could not read file!");
    os.exit(1);
}

io.println("The first 3 lines:");
for i in 0, 3 {
    io.println(f.readln());
}

io.println();
io.println("Read \(f.tell()) bytes so far.");
```
```bsx
mf f = io.Reader("input.txt") fr
ayo nah f {
    io.eprintln("Error: could not read file!") fr
    os.exit(1) fr
}

io.println("The first 3 lines:") fr
yall i amongus 0, 3 {
    io.println(f.readln()) fr
}

io.println() fr
io.println("Read \(f.tell()) bytes so far.") fr
```

```console
$ bs demo.bs
The first 3 lines:
Just a test file
Nothing to see here
Foo

Read 41 bytes so far.
```

### Writer(path) @class
Native C class that opens `path` in writeable mode.

Returns `nil` if failed.

```bs
var f = io.Writer("output.txt");
if !f {
    io.eprintln("Error: could not create file!");
    os.exit(1);
}

f.writeln("Hello there!");
f.writeln("General Kenobi!");
f.writeln(69, "Nice!");
```
```bsx
mf f = io.Writer("output.txt") fr
ayo nah f {
    io.eprintln("Error: could not create file!") fr
    os.exit(1) fr
}

f.writeln("Hello there!") fr
f.writeln("General Kenobi!") fr
f.writeln(69, "Nice!") fr
```

```console
$ bs demo.bs
$ cat output.txt # `type output.txt` on windows
Hello there!
General Kenobi!
69 Nice!
```

#### Writer.close() @method
Close the file.

This is done automatically by the garbage collector, so can be omitted for
short programs.

#### Writer.flush() @method
Flush the contents of the file, since I/O is buffered in C, which is the
language BS is written in.

#### Writer.write(...) @method
Write all the arguments into the file.

Returns `false` if any errors were encountered, else `true`.

#### Writer.writeln(...) @method
Write all the arguments into the file, with a following newline.

Returns `false` if any errors were encountered, else `true`.

### stdin @constant
`Reader` for standard input.

### stdout @constant
`Writer` for standard output.

### stderr @constant
`Writer` for standard error.

## OS
Contains simple primitives for OS functions.

### exit(code) @function
Halt the BS runtime with exit code `code`.

This doesn't actually exit the process itself in embedded usecase.
It just halts the BS interpreter, and the caller of the virtual
machine can decide what to do.

```bs
os.exit(69);
```
```bsx
os.exit(69) fr
```

```console
$ bs demo.bs
$ echo $? # `echo %errorlevel%` on windows
69
```

### clock() @function
Get the monotonic time passed since boot.

This function provides time with nanosecond level of precision but
in the unit of seconds.

```bs
fn fib(n) {
    if n < 2 {
        return n;
    }

    return fib(n - 1) + fib(n - 2);
}

var start = os.clock();
io.println(fib(30));

var elapsed = os.clock() - start;
io.println("Elapsed: \(elapsed)");
```
```bsx
lit fib(n) {
    ayo n < 2 {
        bet n fr
    }

    bet fib(n - 1) + fib(n - 2) fr
}

mf start = os.clock() fr
io.println(fib(30)) fr

mf elapsed = os.clock() - start fr
io.println("Elapsed: \(elapsed)") fr
```

```console
$ bs demo.bs
832040
Elapsed: 0.137143968999226
```

### sleep(seconds) @function
Sleep for `seconds` interval, with nanosecond level of precision.

```bs
var start = os.clock();
os.sleep(0.69);

var elapsed = os.clock() - start;
io.println("Elapsed: \(elapsed)");
```
```bsx
mf start = os.clock() fr
os.sleep(0.69) fr

mf elapsed = os.clock() - start fr
io.println("Elapsed: \(elapsed)") fr
```

```console
$ bs demo.bs
Elapsed: 0.690292477000185
```

### getenv(name) @function
Get the environment variable `name`.

Returns `nil` if it doesn't exist.

```bs
io.println(os.getenv("FOO"));
io.println(os.getenv("BAR"));
```
```bsx
io.println(os.getenv("FOO")) fr
io.println(os.getenv("BAR")) fr
```

```console
$ export FOO=lmao # `set FOO=lmao` on windows
$ bs demo.bs
lmao
nil
```

### setenv(name, value) @function
Set the environment variable `name` to `value`.

Returns `true` if successful, else `false`.

```bs
os.setenv("FOO", "lmao");
io.println(os.getenv("FOO"));
```
```bsx
os.setenv("FOO", "lmao") fr
io.println(os.getenv("FOO")) fr
```

```console
$ bs demo.bs
lmao
```

### args @constant
Array of command line arguments. First element is the program
name.

```bs
io.println(os.args);
```
```bsx
io.println(os.args) fr
```

```console
$ bs demo.bs foo bar baz
["demo.bs", "foo", "bar", "baz"]
```

### Process(args, capture_stdout?, capture_stderr?, capture_stdin?) @class
Native C class that spawns a process. Expects `args` to be an
array of strings that represent the command line arguments.

If any of the optional capture arguments are provided to be true, then that
corresponding file of the created process will be captured.

Returns `nil` if failed.

```bs
var p = os.Process(["ls", "-l"]);
if !p {
    io.eprintln("ERROR: could not start process");
    os.exit(1);
}

p.wait();
```
```bsx
mf p = os.Process(["ls", "-l"]) fr
ayo nah p {
    io.eprintln("ERROR: could not start process") fr
    os.exit(1) fr
}

p.wait() fr
```

```console
$ bs demo.bs
total 4
-rw-r--r-- 1 shoumodip shoumodip 122 Oct  6 15:11 demo.bs
```

#### Process.stdout() @method
Get the standard output of the process as an `io.Reader` instance.

Returns `nil` if the process was spawned without capturing stdout.

```bs
var p = os.Process(["ls", "-l"], true);
if !p {
    io.eprintln("ERROR: could not start process");
    os.exit(1);
}

var stdout = p.stdout();
while !stdout.eof() {
    var line = stdout.readln();
    io.println("Line:", line);
}

p.wait();
```
```bsx
mf p = os.Process(["ls", "-l"], nocap) fr
ayo nah p {
    io.eprintln("ERROR: could not start process") fr
    os.exit(1) fr
}

mf stdout = p.stdout() fr
yolo nah stdout.eof() {
    mf line = stdout.readln() fr
    io.println("Line:", line) fr
}

p.wait() fr
```

```console
$ bs demo.bs
Line: total 4
Line: -rw-r--r-- 1 shoumodip shoumodip 241 Oct  6 15:44 demo.bs
Line:
```

#### Process.stderr() @method
Get the standard error of the process as an `io.Reader` instance.

Returns `nil` if the process was spawned without capturing stderr.

#### Process.stdin() @method
Get the standard input of the process as an `io.Writer` instance.

Returns `nil` if the process was spawned without capturing stdin.

#### Process.kill(signal) @method
Kill the process with `signal`.

On windows the process is just killed, since there is no concept of kill
signals there.

Returns `false` if any errors were encountered, else `true`.

#### Process.wait() @method
Wait for the process to exit, and return its exit code.

Returns `nil` if failed.

## Regular Expressions
POSIX compatible regular expressions.

### Regex(pattern) @class
Native C class that compiles a regular expression.

Returns `nil` if failed.

```bs
var r = Regex("([0-9]+) ([a-z]+)");
io.println("69 apples, 420 oranges".replace(r, "{fruit: '\\2', count: \\1}"));
```
```bsx
mf r = Regex("([0-9]+) ([a-z]+)") fr
io.println("69 apples, 420 oranges".replace(r, "{fruit: '\\2', count: \\1}")) fr
```

```console
$ bs demo.bs
{fruit: 'apples', count: 69}, {fruit: 'oranges', count: 420}
```

## String
Methods for the builtin string value.

### string.slice(start, end) @method
Slice a string from `start` (inclusive) to `end` (exclusive).

```bs
io.println("deez nuts".slice(0, 4));
io.println("deez nuts".slice(5, 9));
```
```bsx
io.println("deez nuts".slice(0, 4)) fr
io.println("deez nuts".slice(5, 9)) fr
```

```console
$ bs demo.bs
deez
nuts
```

### string.reverse() @method
Reverse a string.

```bs
io.println("sllab amgil".reverse());
```
```bsx
io.println("sllab amgil".reverse()) fr
```

```console
$ bs demo.bs
ligma balls
```

### string.toupper() @method
Convert a string to uppercase.

```bs
io.println("Urmom".toupper());
```
```bsx
io.println("Urmom".toupper()) fr
```

```console
$ bs demo.bs
URMOM
```

### string.tolower() @method
Convert a string to lowercase.

```bs
io.println("Urmom".tolower());
```
```bsx
io.println("Urmom".tolower()) fr
```

```console
$ bs demo.bs
urmom
```

### string.tonumber() @method
Convert a string to a number.

```bs
io.println("69".tonumber());
io.println("420.69".tonumber());
io.println("0xff".tonumber());
io.println("69e3".tonumber());
io.println("nah".tonumber());
```
```bsx
io.println("69".tonumber()) fr
io.println("420.69".tonumber()) fr
io.println("0xff".tonumber()) fr
io.println("69e3".tonumber()) fr
io.println("nah".tonumber()) fr
```

```console
$ bs demo.bs
69
420.69
255
69000
nil
```

### string.find(pattern, start?) @method
Find `pattern` within a string starting from position `start` (which defaults
to `0`).

Returns the position if found, else `nil`.

```bs
io.println("foo bar baz".find("ba"));
io.println("foo bar baz".find("ba", 5));
io.println("foo bar baz".find("ba", 9));
io.println("foo bar baz".find("lmao"));

var r = Regex("[0-9]");
io.println("69a".find(r));
io.println("69a".find(r, 1));
io.println("69a".find(r, 2));
io.println("yolol".find(r));
```
```bsx
io.println("foo bar baz".find("ba")) fr
io.println("foo bar baz".find("ba", 5)) fr
io.println("foo bar baz".find("ba", 9)) fr
io.println("foo bar baz".find("lmao")) fr

mf r = Regex("[0-9]") fr
io.println("69a".find(r)) fr
io.println("69a".find(r, 1)) fr
io.println("69a".find(r, 2)) fr
io.println("yolol".find(r)) fr
```

```console
$ bs demo.bs
4
8
nil
nil
0
1
nil
nil
```

### string.split(pattern) @method
Split string by `pattern`.

```bs
io.println("foo bar baz".split(""));
io.println("foo bar baz".split(" "));
io.println("foo bar baz".split("  "));

var r = Regex(" +");
io.println("foobar".split(r));
io.println("foo bar".split(r));
io.println("foo bar   baz".split(r));
```
```bsx
io.println("foo bar baz".split("")) fr
io.println("foo bar baz".split(" ")) fr
io.println("foo bar baz".split("  ")) fr

mf r = Regex(" +") fr
io.println("foobar".split(r)) fr
io.println("foo bar".split(r)) fr
io.println("foo bar   baz".split(r)) fr
```

```console
$ bs demo.bs
["foo bar baz"]
["foo", "bar", "baz"]
["foo bar baz"]
["foobar"]
["foo", "bar"]
["foo", "bar", "baz"]
```

### string.replace(pattern, replacement) @method
Replace `pattern` with `replacement`.

```bs
io.println("foo bar baz".replace("", "-"));
io.println("foo bar baz".replace(" ", "---"));
io.println("foo bar baz".replace("  ", "-"));

var r = Regex("([0-9]+) ([a-z]+)");
io.println("69 apples, 420 oranges".replace(r, "{type: \\2, count: \\1}"));
io.println("69 apples, 420  oranges".replace(r, "{type: \\2, count: \\1}"));
io.println("ayo noice!".replace(r, "{type: \\2, count: \\1}"));
```
```bsx
io.println("foo bar baz".replace("", "-")) fr
io.println("foo bar baz".replace(" ", "---")) fr
io.println("foo bar baz".replace("  ", "-")) fr

mf r = Regex("([0-9]+) ([a-z]+)") fr
io.println("69 apples, 420 oranges".replace(r, "{type: \\2, count: \\1}")) fr
io.println("69 apples, 420  oranges".replace(r, "{type: \\2, count: \\1}")) fr
io.println("ayo noice!".replace(r, "{type: \\2, count: \\1}")) fr
```

```console
$ bs demo.bs
foo bar baz
foo---bar---baz
foo bar ba
{type: apples, count: 69}, {type: oranges, count: 420}
{type: apples, count: 69}, 420  oranges
ayo noice!
```

### string.ltrim(pattern) @method
Trim `pattern` from the left of a string.

```bs
io.println("[" ++ "   foo bar baz  ".ltrim(" ") ++ "]");
```
```bsx
io.println("[" ++ "   foo bar baz  ".ltrim(" ") ++ "]") fr
```

```console
$ bs demo.bs
[foo bar baz  ]
```

### string.rtrim(pattern) @method
Trim `pattern` from the right of a string.

```bs
io.println("[" ++ "   foo bar baz  ".rtrim(" ") ++ "]");
```
```bsx
io.println("[" ++ "   foo bar baz  ".rtrim(" ") ++ "]") fr
```

```console
$ bs demo.bs
[   foo bar baz]
```

### string.trim(pattern) @method
Trim `pattern` from both sides of a string.

```bs
io.println("[" ++ "   foo bar baz  ".trim(" ") ++ "]");
```
```bsx
io.println("[" ++ "   foo bar baz  ".trim(" ") ++ "]") fr
```

```console
$ bs demo.bs
[foo bar baz]
```

### string.lpad(pattern, count) @method
Pad string with `pattern` on the left side, till the total length reaches
`count`.

```bs
io.println("foo".lpad("69", 0));
io.println("foo".lpad("69", 3));
io.println("foo".lpad("69", 4));
io.println("foo".lpad("69", 5));
io.println("foo".lpad("69", 6));
```
```bsx
io.println("foo".lpad("69", 0)) fr
io.println("foo".lpad("69", 3)) fr
io.println("foo".lpad("69", 4)) fr
io.println("foo".lpad("69", 5)) fr
io.println("foo".lpad("69", 6)) fr
```

```console
$ bs demo.bs
foo
foo
6foo
69foo
696foo
```

### string.rpad(pattern, count) @method
Pad string with `pattern` on the right side, till the total length reaches
`count`.

```bs
io.println("foo".rpad("69", 0));
io.println("foo".rpad("69", 3));
io.println("foo".rpad("69", 4));
io.println("foo".rpad("69", 5));
io.println("foo".rpad("69", 6));
```
```bsx
io.println("foo".rpad("69", 0)) fr
io.println("foo".rpad("69", 3)) fr
io.println("foo".rpad("69", 4)) fr
io.println("foo".rpad("69", 5)) fr
io.println("foo".rpad("69", 6)) fr
```

```console
$ bs demo.bs
foo
foo
foo6
foo69
foo696
```

### string.prefix(pattern) @method
Check whether string starts with `pattern`.

```bs
io.println("foobar".prefix("foo"));
io.println("foobar".prefix("Foo"));
io.println("foobar".prefix("deez nuts"));
```
```bsx
io.println("foobar".prefix("foo")) fr
io.println("foobar".prefix("Foo")) fr
io.println("foobar".prefix("deez nuts")) fr
```

```console
$ bs demo.bs
true
false
false
```

### string.suffix(pattern) @method
Check whether string ends with `pattern`.

```bs
io.println("foobar".suffix("bar"));
io.println("foobar".suffix("Bar"));
io.println("foobar".suffix("deez nuts"));
```
```bsx
io.println("foobar".suffix("bar")) fr
io.println("foobar".suffix("Bar")) fr
io.println("foobar".suffix("deez nuts")) fr
```

```console
$ bs demo.bs
true
false
false
```

## Bit
Contains bitwise operations which are not frequent enough to warrant a
dedicated operator, but which still finds occasional uses.

### ceil(n) @function
Return the bitwise ceiling of a number.

```bs
io.println(bit.ceil(7));
io.println(bit.ceil(8));
io.println(bit.ceil(9));
```
```bsx
io.println(bit.ceil(7)) fr
io.println(bit.ceil(8)) fr
io.println(bit.ceil(9)) fr
```

```console
$ bs demo.bs
8
8
16
```

### floor(n) @function
Return the bitwise floor of a number.

```bs
io.println(bit.floor(7));
io.println(bit.floor(8));
io.println(bit.floor(9));
```
```bsx
io.println(bit.floor(7)) fr
io.println(bit.floor(8)) fr
io.println(bit.floor(9)) fr
```

```console
$ bs demo.bs
4
8
8
```

## ASCII
Contains functions for dealing with ASCII codes and characters.

### char(code) @function
Return the character associated with an ASCII code.

### code(char) @function
Return the ASCII code associated with a character.

Expects `char` to be a string of length `1`.

### isalnum(char) @function
Return whether a character is an alphabet or a digit.

Expects `char` to be a string of length `1`.

### isalpha(char) @function
Return whether a character is an alphabet.

Expects `char` to be a string of length `1`.

### iscntrl(char) @function
Return whether a character is a control character.

Expects `char` to be a string of length `1`.

### isdigit(char) @function
Return whether a character is a digit.

Expects `char` to be a string of length `1`.

### islower(char) @function
Return whether a character is lowercase.

Expects `char` to be a string of length `1`.

### isupper(char) @function
Return whether a character is uppercase.

Expects `char` to be a string of length `1`.

### isgraph(char) @function
Return whether a character is graphable.

Expects `char` to be a string of length `1`.

### isprint(char) @function
Return whether a character is printable.

Expects `char` to be a string of length `1`.

### ispunct(char) @function
Return whether a character is a punctuation.

Expects `char` to be a string of length `1`.

### isspace(char) @function
Return whether a character is whitespace.

Expects `char` to be a string of length `1`.

## Bytes
Strings are immutable in BS. The native class `Bytes` provides a mutable string
builder for optimized string modification operations.

### Bytes() @class
Create a string builder.

### Bytes.count() @method
Return the current number of bytes written.

### Bytes.reset(position) @method
Move back the writer head to `position`.

Also create a string from `position` to the end of the `Bytes` instance and return it.

### Bytes.slice(start, end) @method
Return a slice from `start` (inclusive) to `end` (exclusive).

### Bytes.write(value) @method
Write `value` to the end.

### Bytes.insert(position, value) @method
Write `value` at `position`.

## Array
Methods for the builtin array value.

### array.map(f) @method
Functional map.

The provided function `f` must take a single argument.

### array.filter(f) @method
Functional filter.

The provided function `f` must take a single argument.

### array.reduce(f, accumulator?) @method
Functional reduce.

The provided function `f` must take two arguments. The first
argument shall be the accumulator, and the second shall be the
current value.

### array.copy() @method
Copy an array.

### array.join(separator) @method
Join the elements of an array, separated by `separator` into a single string.

### array.find(value, start?) @method
Find `value` within an array starting from position `start` (which defaults to
`0`).

Returns the position if found, else `nil`.

### array.equal(that) @method
Compare the elements of two arrays.

### array.push(value) @method
Push `value` into an array.

This modifies the array.

### array.insert(position, value) @method
Insert `value` into an array at `position`.

This modifies the array.

### array.sort(compare) @method
Sort an array inplace with `compare`, and return itself.

```bs
var xs = [4, 2, 5, 1, 3];
xs.sort(fn (x, y) => x < y);

io.println(xs); # Output: [1, 2, 3, 4, 5]
```
```bsx
mf xs = [4, 2, 5, 1, 3] fr
xs.sort(lit (x, y) => x < y) fr

io.println(xs) fr # Output: [1, 2, 3, 4, 5]
```

The compare function must take two arguments. If it returns `true`, then the
left argument shall be considered "less than", and vice versa for `false`.

### array.resize(size) @method
Resize an array to one having `size` elements.

If `size` is larger than the original size, the extra elements shall default to
`nil`.

This modifies the array.

### array.reverse() @method
Reverse an array.

This modifes the array.

## Table
Methods for the builtin table value.

### table.copy() @method
Copy a table.

### table.equal(that) @method
Compare the elements of two tables.

## Math
Contains simple mathematical primitives.

The methods defined here work on numbers.

### number.sin() @method
Sine in radians.

### number.cos() @method
Cosine in radians.

### number.tan() @method
Tangent in radians.

### number.asin() @method
Inverse sine in radians.

### number.acos() @method
Inverse cosine in radians.

### number.atan() @method
Inverse tangent in radians.

### number.exp() @method
Return the exponential function of the number.

### number.log() @method
Return the natural logarithm (base `e`) of the number.

### number.log10() @method
Return the common logarithm (base `10`) of the number.

### number.pow(exponent) @method
Raise the number to `exponent`.

### number.sqrt() @method
Square root.

### number.ceil() @method
Ceiling.

### number.floor() @method
Floor.

### number.round() @method
Return the nearest integer.

### number.max(those ...) @method
Return the maximum between the method receiver and the provided arguments.

### number.min(those ...) @method
Return the minimum between the method receiver and the provided arguments.

### number.clamp(low, high) @method
Clamp the number between `low` and `high`.

### number.lerp(a, b, t) @method
Linear interpolation.

### E @constant
Euler's constant.

### PI @constant
PI.

### random() @function
Return a pseudorandom number between `0` and `1`.
