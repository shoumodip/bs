# Stdlib

<!-- home-icon -->

## I/O
Contains simple I/O primitives.

Note that I/O is binary in BS. On *nix systems it doesn't matter, but on the
video game OS it should be kept in mind.

### input(prompt?) @function
Read a line from standard input, printing the optional argument `prompt` if
provided.

```bs
var name = io.input("Enter your name> ")
io.println("Hello, \(name)!")
```

```console
$ bs demo.bs
Enter your name> Batman
Hello, Batman!
```

### print(...) @function
Print all the arguments to standard output.

```bs
io.print("Hello", "world", 69)
```

```console
$ bs demo.bs
Hello world 69⏎
```

The arguments are separated with spaces.

### println(...) @function
Print all the arguments to standard output, with a following newline.

### eprint(...) @function
Print all the arguments to standard error.

### eprintln(...) @function
Print all the arguments to standard error, with a following newline.

### readdir(path) @function
Read a directory into an array of `DirEntry`.

Returns `nil` if failed.

```bs
var entries = assert(io.readdir("."))

for _, e in entries {
    io.println(e.name(), e.isdir())
}
```

```console
$ bs demo.bs
.git true
include true
demo.bs false
compile_flags.txt false
lib true
tests true
.gitattributes false
README.md false
bin true
docs true
editor true
build true
.gitignore false
examples true
src true
LICENSE false
.github true
```

### Reader(path) @class
Native C class that opens `path` in readable mode.

Returns `nil` if failed.

```bs
var f = io.Reader("input.txt")
if !f {
    io.eprintln("Error: could not read file!")
    os.exit(1)
}

var contents = f.read()
io.print(contents)
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
var f = io.Reader("input.txt")
if !f {
    io.eprintln("Error: could not read file!")
    os.exit(1)
}

io.print("The first 16 bytes: [\(f.read(16))]\n")
io.print("The rest:", f.read())
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
var f = io.Reader("input.txt")
if !f {
    io.eprintln("Error: could not read file!")
    os.exit(1)
}

while !f.eof() {
    var line = f.readln()
    io.println("Line:", line)
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
var f = io.Reader("input.txt")
if !f {
    io.eprintln("Error: could not read file!")
    os.exit(1)
}

io.print("The first 16 bytes: [\(f.read(16))]\n")

f.seek(5, io.SEEK_SET)

io.println("The full content offset by 5:")
io.print(f.read())
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
var f = io.Reader("input.txt")
if !f {
    io.eprintln("Error: could not read file!")
    os.exit(1)
}

io.println("The first 3 lines:")
for i in 0, 3 {
    io.println(f.readln())
}

io.println()
io.println("Read \(f.tell()) bytes so far.")
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
var f = io.Writer("output.txt")
if !f {
    io.eprintln("Error: could not create file!")
    os.exit(1)
}

f.writeln("Hello there!")
f.writeln("General Kenobi!")
f.writeln(69, "Nice!")
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

### DirEntry(name, isdir) @class
Native C class that wraps over a directory entry. This doesn't really do
anything, and only serves as an implementation detail for `readdir()`.

Example usecase:

```bs
var entries = assert(io.readdir("."))

for _, e in entries {
    io.println(e.name(), e.isdir())
}
```

```console
$ bs demo.bs
.git true
include true
demo.bs false
compile_flags.txt false
lib true
tests true
.gitattributes false
README.md false
bin true
docs true
editor true
build true
.gitignore false
examples true
src true
LICENSE false
.github true
```

#### DirEntry.name() @method
Get the name of the entry.

#### DirEntry.isdir() @method
Get whether the entry is a directory.

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
os.exit(69)
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
        return n
    }

    return fib(n - 1) + fib(n - 2)
}

var start = os.clock()
io.println(fib(30))

var elapsed = os.clock() - start
io.println("Elapsed: \(elapsed)")
```

```console
$ bs demo.bs
832040
Elapsed: 0.137143968999226
```

### sleep(seconds) @function
Sleep for `seconds` interval, with nanosecond level of precision.

```bs
var start = os.clock()
os.sleep(0.69)

var elapsed = os.clock() - start
io.println("Elapsed: \(elapsed)")
```

```console
$ bs demo.bs
Elapsed: 0.690292477000185
```

### getenv(name) @function
Get the environment variable `name`.

Returns `nil` if it doesn't exist.

```bs
io.println(os.getenv("FOO"))
io.println(os.getenv("BAR"))
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
os.setenv("FOO", "lmao")
io.println(os.getenv("FOO"))
```

```console
$ bs demo.bs
lmao
```

### getcwd() @function
Get the current working directory.

```bs
io.println(os.getcwd())
```

```console
$ bs demo.bs
/home/sk/Git/bs
```

### setcwd(dir) @function
Set the current working directory to `dir`.

Returns `true` if successful, else `false`.

```bs
os.setcwd("/usr")
io.println(os.getcwd())
```

```console
$ bs demo.bs
/usr
```

### args @constant
Array of command line arguments. First element is the program
name.

```bs
io.println(os.args)
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
var p = os.Process(["ls", "-l"])
if !p {
    io.eprintln("ERROR: could not start process")
    os.exit(1)
}

p.wait()
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
var p = os.Process(["ls", "-l"], true)
if !p {
    io.eprintln("ERROR: could not start process")
    os.exit(1)
}

var stdout = p.stdout()
while !stdout.eof() {
    var line = stdout.readln()
    io.println("Line:", line)
}

p.wait()
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

```bs
var p = os.Process(["grep", "foobar"], false, false, true)
if !p {
    io.eprintln("ERROR: could not start process")
    os.exit(1)
}

var stdin = p.stdin()
stdin.writeln("First line foobar lmao")
stdin.writeln("Second line lmao")
stdin.writeln("Third line lets goooo foobar")
stdin.close()
p.wait()
```

```console
$ bs demo.bs
First line foobar lmao
Third line lets goooo foobar
```

#### Process.kill(signal) @method
Kill the process with `signal`.

On windows the process is just killed, since there is no concept of kill
signals there.

Returns `false` if any errors were encountered, else `true`.

#### Process.wait() @method
Wait for the process to exit, and return its exit code.

Returns `nil` if failed.

## Regex(pattern) @class
POSIX compatible regular expressions.

Returns `nil` if failed.

```bs
var r = Regex("([0-9]+) ([a-z]+)")
io.println("69 apples, 420 oranges".replace(r, "{fruit: '\\2', count: \\1}"))
```

```console
$ bs demo.bs
{fruit: 'apples', count: 69}, {fruit: 'oranges', count: 420}
```

## String
Methods for the builtin string value.

### string.slice(start, end?) @method
Slice a string from `start` (inclusive) to `end` (exclusive).

```bs
io.println("deez nuts".slice(0, 4))
io.println("deez nuts".slice(5, 9))
io.println("deez nuts".slice(5)) # No end defaults to string end
```

```console
$ bs demo.bs
deez
nuts
nuts
```

### string.reverse() @method
Reverse a string.

```bs
io.println("sllab amgil".reverse())
```

```console
$ bs demo.bs
ligma balls
```

### string.repeat(count) @method
Repeat a string `count` times.

```bs
io.println("foo".repeat(6))
```

```console
$ bs demo.bs
foofoofoofoofoofoo
```

### string.toupper() @method
Convert a string to uppercase.

```bs
io.println("Urmom".toupper())
```

```console
$ bs demo.bs
URMOM
```

### string.tolower() @method
Convert a string to lowercase.

```bs
io.println("Urmom".tolower())
```

```console
$ bs demo.bs
urmom
```

### string.tonumber() @method
Convert a string to a number.

```bs
io.println("69".tonumber())
io.println("420.69".tonumber())
io.println("0xff".tonumber())
io.println("69e3".tonumber())
io.println("nah".tonumber())
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
io.println("foo bar baz".find("ba"))
io.println("foo bar baz".find("ba", 5))
io.println("foo bar baz".find("ba", 9))
io.println("foo bar baz".find("lmao"))

var r = Regex("[0-9]")
io.println("69a".find(r))
io.println("69a".find(r, 1))
io.println("69a".find(r, 2))
io.println("yolol".find(r))
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
io.println("foo bar baz".split(""))
io.println("foo bar baz".split(" "))
io.println("foo bar baz".split("  "))

var r = Regex(" +")
io.println("foobar".split(r))
io.println("foo bar".split(r))
io.println("foo bar   baz".split(r))
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
io.println("foo bar baz".replace("", "-"))
io.println("foo bar baz".replace(" ", "---"))
io.println("foo bar baz".replace("  ", "-"))

var r = Regex("([0-9]+) ([a-z]+)")
io.println("69 apples, 420 oranges".replace(r, "{type: \\2, count: \\1}"))
io.println("69 apples, 420  oranges".replace(r, "{type: \\2, count: \\1}"))
io.println("ayo noice!".replace(r, "{type: \\2, count: \\1}"))
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

### string.compare(other) @method
Compare two strings together.

- `0` means the string is equal to the other string
- `1` means the string is "greater than" the other string
- `-1` means the string is "less than" the other string

```bs
io.println("foo".compare("bar"))
io.println("foo".compare("lol"))
io.println("foo".compare("foo"))
io.println("foo".compare("food"))
io.println("food".compare("foo"))
```

```console
$ bs demo.bs
1
-1
0
-1
1
```

### string.ltrim(pattern) @method
Trim `pattern` from the left of a string.

```bs
io.println("[" $ "   foo bar baz  ".ltrim(" ") $ "]")
```

```console
$ bs demo.bs
[foo bar baz  ]
```

### string.rtrim(pattern) @method
Trim `pattern` from the right of a string.

```bs
io.println("[" $ "   foo bar baz  ".rtrim(" ") $ "]")
```

```console
$ bs demo.bs
[   foo bar baz]
```

### string.trim(pattern) @method
Trim `pattern` from both sides of a string.

```bs
io.println("[" $ "   foo bar baz  ".trim(" ") $ "]")
```

```console
$ bs demo.bs
[foo bar baz]
```

### string.lpad(pattern, count) @method
Pad string with `pattern` on the left side, till the total length reaches
`count`.

```bs
io.println("foo".lpad("69", 0))
io.println("foo".lpad("69", 3))
io.println("foo".lpad("69", 4))
io.println("foo".lpad("69", 5))
io.println("foo".lpad("69", 6))
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
io.println("foo".rpad("69", 0))
io.println("foo".rpad("69", 3))
io.println("foo".rpad("69", 4))
io.println("foo".rpad("69", 5))
io.println("foo".rpad("69", 6))
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
io.println("foobar".prefix("foo"))
io.println("foobar".prefix("Foo"))
io.println("foobar".prefix("deez nuts"))
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
io.println("foobar".suffix("bar"))
io.println("foobar".suffix("Bar"))
io.println("foobar".suffix("deez nuts"))
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
io.println(bit.ceil(7))
io.println(bit.ceil(8))
io.println(bit.ceil(9))
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
io.println(bit.floor(7))
io.println(bit.floor(8))
io.println(bit.floor(9))
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

## Bytes(str?) @class
Strings are immutable in BS. The native class `Bytes` provides a mutable string
builder for optimized string modification operations.

```bs
var b = Bytes()
b.push("Hello, ")
b.push("world!")
io.println(b)

var nice = Bytes("69 Hehe")
io.println(nice)
```

```console
$ bs demo.bs
Hello, world!
69 Hehe
```

### Bytes.count() @method
Return the current number of bytes written.

```bs
var b = Bytes()
b.push("Hello")
b.push(" world!")
io.println(b)

var n = b.count()
io.println("\(n) bytes written.")
```

```console
$ bs demo.bs
Hello world!
12 bytes written.
```

### Bytes.reset(position) @method
Move back the writer head to `position`.

```bs
var b = Bytes()
b.push("Hello")

var p = b.count()
b.push(" world!")

io.println(b)
b.reset(p)
io.println(b)
```

```console
$ bs demo.bs
Hello world!
Hello
```

### Bytes.slice(start?, end?) @method
Return a slice from `start` (inclusive) to `end` (exclusive).

If no arguments are provided to this function, the whole builder is returned as
a string.

```bs
var b = Bytes()
b.push("Hello world!")

io.println(b.slice())
io.println(b.slice(0, 5))
io.println(b.slice(6, 12))
```

```console
$ bs demo.bs
Hello world!
Hello
world!
```

### Bytes.push(value) @method
Push `value` to the end.

```bs
var buffer = Bytes()

# Operations can be chained
buffer
    .push("Hello")           # A String
    .push(Bytes(", world!")) # Another Bytes instance
    .push(32)                # An ASCII code, in this case ' '
    .push($69)               # To push the string representation, a string must be provided

io.println(buffer)
```

```console
$ bs demo.bs
Hello, world! 69
```

### Bytes.insert(position, value) @method
Insert `value` at `position`.

```bs
var buffer = Bytes("world!")

# Operations can be chained, just like Bytes.push()
buffer
    .insert(0, "Hell")      # A String
    .insert(4, Bytes(", ")) # Another Bytes instance
    .insert(4, 111)         # An ASCII code, in this case 'o'

io.println(buffer)
```

```console
$ bs demo.bs
Hello, world!
```

### Bytes.get(position) @method
Get the byte at `position` as a number.

```bs
var b = Bytes()
b.push("Hello")

for i in 0, b.count() {
    var c = b.get(i)
    io.println(ascii.char(c), c)
}
```

```console
$ bs demo.bs
H 72
e 101
l 108
l 108
o 111
```

### Bytes.set(position, value) @method
Set the byte at `position` to `value`.

The argument `value` has to be a number.

```bs
var b = Bytes()

b.push("Cello")
io.println(b)

b.set(0, ascii.code("H"))
io.println(b)
```

```console
$ bs demo.bs
Cello
Hello
```

## Array
Methods for the builtin array value.

### array.map(f) @method
Functional map.

The provided function `f` must take a single argument.

```bs
var xs = [1, 2, 3, 4, 5]
var ys = xs.map(fn (x) => x * 2)
io.println(xs)
io.println(ys)
```

```console
$ bs demo.bs
[1, 2, 3, 4, 5]
[2, 4, 6, 8, 10]
```

### array.filter(f) @method
Functional filter.

The provided function `f` must take a single argument.

```bs
var xs = [1, 2, 3, 4, 5]
var ys = xs.filter(fn (x) => x % 2 == 0)
io.println(xs)
io.println(ys)
```

```console
$ bs demo.bs
[1, 2, 3, 4, 5]
[2, 4]
```

### array.reduce(f, accumulator?) @method
Functional reduce.

The provided function `f` must take two arguments. The first
argument shall be the accumulator, and the second shall be the
current value.

```bs
var xs = [1, 2, 3, 4, 5]

var a = xs.reduce(fn (x, y) => x + y)
io.println(a)

var b = xs.reduce(fn (x, y) => x + y, 10)
io.println(b)
```

```console
$ bs demo.bs
15
25
```

### array.copy() @method
Copy an array.

```bs
var xs = [1, 2, 3, 4, 5]
var ys = xs
var zs = xs.copy()

xs[0] = 69

io.println(xs)
io.println(ys)
io.println(zs)
```

```console
$ bs demo.bs
[69, 2, 3, 4, 5]
[69, 2, 3, 4, 5]
[1, 2, 3, 4, 5]
```

### array.join(separator) @method
Join the elements of an array, separated by `separator` into a single string.

```bs
io.println([1, 2, 3, 4, 5].join(" -> "))
```

```console
$ bs demo.bs
1 -> 2 -> 3 -> 4 -> 5
```

### array.find(value, start?) @method
Find `value` within an array starting from position `start` (which defaults to
`0`).

Returns the position if found, else `nil`.

```bs
var xs = [1, 2, 3, 4, 5, 3]
io.println(xs.find(3))
io.println(xs.find(3, 3))
io.println(xs.find(3, 6))
io.println(xs.find(true, 6))
```

```console
$ bs demo.bs
2
5
nil
nil
```

### array.equal(that) @method
Compare the elements of two arrays.

```bs
var xs = [1, 2, 3, 4, 5]
var ys = [1, 2, 3, 4, 5]

io.println(xs == ys)
io.println(xs.equal(ys))
```

```console
$ bs demo.bs
false
true
```

The first expression returns `false` because the `==` operator compares by
reference.

### array.push(value) @method
Push `value` into an array.

This modifies the array.

```bs
var xs = []

for i in 0, 5 {
    xs.push(i * 2)
}

io.println(xs)
```

```console
$ bs demo.bs
[0, 2, 4, 6, 8]
```

Operations can be chained.

```bs
io.println(
    ["Nice!"]
        .push(69)
        .push(420))
```

```console
$ bs demo.bs
["Nice!", 69, 420]
```

### array.insert(position, value) @method
Insert `value` into an array at `position`.

This modifies the array.

```bs
var xs = []

for i in 0, 5 {
    if i == 2 {
        continue
    }

    xs.push(i * 2)
}
io.println(xs)

xs.insert(2, 4)
io.println(xs)
```

```console
$ bs demo.bs
[0, 2, 6, 8]
[0, 2, 4, 6, 6]
```

Operations can be chained, like `array.push()`

```bs
io.println(
    ["Nice!"]
        .insert(0, 69)
        .insert(1, 420))
```

```console
$ bs demo.bs
[69, 420, "Nice!"]
```

### array.sort(compare) @method
Sort an array inplace with `compare`, and return itself.

```bs
var xs = [4, 2, 5, 1, 3]
xs.sort(fn (x, y) => x < y) # This also returns the array so you can chain operations

io.println(xs)
```

```console
$ bs demo.bs
[1, 2, 3, 4, 5]
```

The compare function must take two arguments. If it returns `true`, then the
left argument shall be considered "less than", and vice versa for `false`.

### array.resize(size) @method
Resize an array to one having `size` elements, and return itself.

If `size` is larger than the original size, the extra elements shall default to
`nil`.

This modifies the array.

```bs
var xs = [1, 2, 3, 4, 5]
io.println(xs)

io.println(xs.resize(3))
io.println(xs)
```

```console
$ bs demo.bs
[1, 2, 3, 4, 5]
[1, 2, 3]
[1, 2, 3]
```

### array.remove(index) @method
Removes the item at `index` and returns it.

This modifies the array.

```bs
var xs = [1, 2, 3, 4, 5]
io.println(xs)

io.println(xs.remove(2))
io.println(xs)
```

```console
$ bs demo.bs
[1, 2, 3, 4, 5]
3
[1, 2, 4, 5]
```

### array.reverse() @method
Reverse an array.

This modifies the array.

```bs
var xs = [1, 2, 3, 4, 5]
io.println(xs)

xs.reverse() # This also returns the array so you can chain operations
io.println(xs)
```

```console
$ bs demo.bs
[1, 2, 3, 4, 5]
[5, 4, 3, 2, 1]
```

### array.fill(value) @method
Fill an array with `value`.

This modifies the array.

```bs
var xs = [1, 2, 3, 4, 5]
io.println(xs)

xs.fill(69) # This also returns the array so you can chain operations
io.println(xs)
```

```console
$ bs demo.bs
[1, 2, 3, 4, 5]
[69, 69, 69, 69, 69]
```

Use this with `array.resize()` to create an array with a preset size and value.

```bs
var xs = [].resize(5).fill("foo")
io.println(xs)
```

```console
$ bs demo.bs
["foo", "foo", "foo", "foo", "foo"]
```

Here's a 2D version.

```bs
var width = 5
var height = 6

var board = []
    .resize(height)
    .map(fn (x) => [].resize(width).fill(0))

board[2][2] = 1

io.println(board)
```

```console
$ bs demo.bs
[
    [0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0],
    [0, 0, 1, 0, 0],
    [0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0]
]
```

### array.slice(start, end?) @method
Slice an array from `start` (inclusive) to `end` (exclusive).

```bs
var xs = [1, 2, 3, 4, 5]
io.println(xs)
io.println(xs.slice(2))
io.println(xs.slice(1, 3))
```

```console
$ bs demo.bs
[1, 2, 3, 4, 5]
[3, 4, 5]
[2, 3]
```

### array.append(other) @method
Append an array.

This modifies the array.

```bs
var xs = [1, 2, 3, 4, 5]
var ys = [6, 7, 8, 9, 10]
io.println(xs)
io.println(xs.append(ys))
```

```console
$ bs demo.bs
[1, 2, 3, 4, 5]
[
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10
]
```

## Table
Methods for the builtin table value.

### table.copy() @method
Copy a table.

```bs
var xs = {
    foo = 69,
    bar = 420
}

var ys = xs
var zs = xs.copy()

xs.bar = 1337

io.println(xs)
io.println(ys)
io.println(zs)
```

```console
$ bs demo.bs
{
    bar = 1337,
    foo = 69
}
{
    bar = 1337,
    foo = 69
}
{
    bar = 420,
    foo = 69
}
```

### table.equal(that) @method
Compare the elements of two tables.

```bs
var xs = { foo = 69, bar = 420 }
var ys = { foo = 69, bar = 420 }

io.println(xs == ys)
io.println(xs.equal(ys))
```

```console
$ bs demo.bs
false
true
```

Just like arrays, tables are compared by reference with the `==` operator.

## Math
Contains simple mathematical primitives.

### number.sin() @method
Sine in radians.

```bs
var theta = 0.5
io.println(theta.sin())
```

```console
$ bs demo.bs
0.479425538604203
```

Note that the precision of the mathematical functions may vary depending on the
compiler and the platform. This is inherent to computing in general.

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

```bs
io.println(2.exp()) # Basically e^2
```

```console
$ bs demo.bs
7.38905609893065
```

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

### number.abs() @method
Return the absolute value (Basically make a number positive).

### number.sign() @method
Return the sign of the number.

```bs
io.println((0).sign())
io.println((69).sign())
io.println((-420).sign())
```

```console
$ bs demo.bs
0
1
-1
```

### number.max(...) @method
Return the maximum between the method receiver and the provided arguments.

```bs
io.println(1.max(2, 3))
io.println(3.max(0, 1, 2))
```

```console
$ bs demo.bs
3
3
```

### number.min(...) @method
Return the minimum between the method receiver and the provided arguments.

```bs
io.println(1.min(2, 3))
io.println(3.min(1, 2, 3))
```

```console
$ bs demo.bs
1
1
```

### number.clamp(low, high) @method
Clamp the number between `low` and `high`.

### number.lerp(a, b, t) @method
Linear interpolation.

### number.precise(level) @method
Set the precision (number of decimal digits).

```bs
var n = 69.1337
io.println(n)
io.println(n.precise(0))
io.println(n.precise(3))
```

```console
$ bs demo.bs
69.1337
69
69.134
```

### Random(seed?) @class
Random number generator using the `xoroshiro128+` algorithm.

If the `seed` argument is not provided, then a random seed is chosen at
runtime.

```bs
var a = math.Random()
var b = math.Random(1337)

io.println(a.number())
io.println(a.number())
io.println(a.number(69, 420))
io.println(a.number(69, 420))

io.println()
io.println("==== Constant ====")
io.println()

io.println(b.number())
io.println(b.number())
io.println(b.number(69, 420))
io.println(b.number(69, 420))
```

```console
$ bs demo.bs
0.279107155961776
0.73815524476827
225.323320231653
148.438554063034

==== Constant ====

0.0725452046400308
0.811773795162954
416.950161924657
182.365867621578
```

#### Random.number(min?, max?) @method
Return a random number between `low` and `high`.

If no arguments are provided, a number between `0` and `1` is returned.

```bs
var r = math.Random()
io.println(r.number())
io.println(r.number())
io.println(r.number(69, 420))
io.println(r.number(69, 420))
```

```console
$ bs demo.bs
0.279107155961776
0.73815524476827
225.323320231653
148.438554063034
```

#### Random.bytes(count) @method
Return a random sequence of bytes.

```bs
var seq = math.Random().bytes(9)
io.println(seq.slice())
```

```console
$ bs demo.bs
{03ah
"7f
```

### range(begin, end, step?) @function
Return an array containing a range.

If `step` is not provided, it is automatically selected.

```bs
io.println(math.range(1, 6))
io.println(math.range(6, 1))

io.println(math.range(1, 6, 2))
io.println(math.range(6, 1, -2))
```

```console
$ bs demo.bs
[1, 2, 3, 4, 5]
[6, 5, 4, 3, 2]
[1, 3, 5]
[6, 4, 2]
```

If `step` is provided such that it would run indefinitely, an error will be
raised.

```bs
io.println(math.range(1, 6, -1))
```

```console
$ bs demo.bs
[C]: error: a step of -1 in an ascending range would run indefinitely
demo.bs:1:29: in range()
```

### E @constant
Euler's constant.

### PI @constant
PI.

## Meta
Contains simple metaprogramming primitives.

### compile(str) @function
Compile a string into a function.

```bs
var f = meta.compile("34 + 35")
io.println(f())
```

```console
$ bs demo.bs
69
```

If any errors were encountered while compiling the string, a table is returned
instead of a function.

```bs
var f = meta.compile("Oops@")
io.println(f)
```

```console
$ bs demo.bs
{
    explanation = nil,
    example = nil,
    line = "Oops@",
    message = "invalid character '@' (64)",
    col = 5,
    row = 1
}
```

So the usage becomes as straight forward as:

```bs
var f = meta.compile(...)
if f is "table" {
    io.eprintln("Error")
    # Error handling...
}

f() # Or whatever you want to do
```

The function is compiled such that the last expression in the body is returned,
otherwise defaulting to `nil`.

```bs
var f = meta.compile("
    for i in 0, 5 {
        io.println('Nice!')
    }
")

assert(f is "function")
io.println(f())

var g = meta.compile("
    for i in 0, 5 {
        io.println('Hehe!')
    }

    69
")

assert(g is "function")
io.println(g())

var h = meta.compile("
    for i in 0, 5 {
        io.println('Bruh!')
    }

    return 420
")

assert(h is "function")
io.println(h())
```

```console
$ bs demo.bs
Nice!
Nice!
Nice!
Nice!
Nice!
nil
Hehe!
Hehe!
Hehe!
Hehe!
Hehe!
69
Bruh!
Bruh!
Bruh!
Bruh!
Bruh!
420
```

### eval(str) @function
Evaluate a string.

```bs
io.println(meta.eval("34 + 35"))
io.println(meta.eval("
    for i in 0, 5 {
        io.println('Nice!')
    }
"))
```

```console
$ bs demo.bs
69
Nice!
Nice!
Nice!
Nice!
Nice!
nil
```
