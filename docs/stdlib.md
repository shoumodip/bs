# Stdlib Documentation
BS has a very minimal standard library which just has the bare essentials. This keeps the core runtime simple and easy to embed.

## IO
Contains simple I/O primitives.

### Function `io.input(prompt?: string) -> string`
Read a line from standard input, printing the optional argument `prompt` if provided.

### Function `io.print(...)`
Print all the arguments to standard output.

### Function `io.println(...)`
Print all the arguments to standard output, with a following newline.

### Function `io.eprint(...)`
Print all the arguments to standard error.

### Function `io.eprintln(...)`
Print all the arguments to standard error, with a following newline.

### Class `io.Reader(path: string)`
Native C class that opens `path` in readable mode.

Returns `nil` if failed.

#### Method `io.Reader.close()`
Close the file.

This is done automatically by the garbage collector, so can be omitted for
short programs.

#### Method `io.Reader.read(count?: number) -> string | nil`
Read `count` bytes from the current position. If the argument `bytes` is not
provided, it reads all of the available bytes.

Returns `nil` if failed.

#### Method `io.Reader.readln() -> string`
Read a line.

#### Method `io.Reader.seek(offset: number, whence: number) -> boolean`
Change the read position of the file.

Any of the following values can be used for `whence`.

- `io.SEEK_SET` - Seek from beginning of file
- `io.SEEK_CUR` - Seek from current position
- `io.SEEK_END` - Seek from end of file

Returns `true` if succeeded and `false` if failed

#### Method `io.Reader.tell() -> number`
Get the current position of the file.

Returns `nil` if failed

### Class `io.Writer(path: string)`
Native C class that opens `path` in writeable mode.

Returns `nil` if failed.

#### Method `io.Writer.close()`
Close the file.

This is done automatically by the garbage collector, so can be omitted for
short programs.

#### Method `io.Writer.flush()`
Flush the contents of the file, since IO is buffered in C.

#### Method `io.Writer.write(...) -> boolean`
Write all the arguments into the file.

Returns `false` if any errors were encountered, else `true`.

#### Method `io.Writer.writeln(...) -> boolean`
Write all the arguments into the file, with a following newline.

Returns `false` if any errors were encountered, else `true`.

### Instance `io.stdin`
`io.Reader` for standard input.

### Instance `io.stdout`
`io.Writer` for standard output.

### Instance `io.stderr`
`io.Writer` for standard error.

## OS
Contains simple primitives for OS functions.

### Function `os.exit(code: number)`
Halt the BS runtime with exit code `code`.

This doesn't actually exit the process itself in embedded usecase.
It just halts the BS interpreter, and the caller of the virtual
machine can decide what to do.

### Functin `os.clock() -> number`
Get the monotonic time passed since boot.

This function provides time with nanosecond level of precision but
in the unit of seconds.

### Function `os.sleep(seconds: number)`
Sleep for `seconds` interval, with nanosecond level of precision.

### Function `os.getenv(name: string) -> string | nil`
Get the environment variable `name`.

Returns `nil` if it doesn't exist.

### Function `os.setenv(name: string, value: string) -> boolean`
Set the environment variable `name` to `value`.

Returns `true` if successful, else `false`.

### Array `os.args`
Array of command line arguments. First element is the program
name.

### Class `os.Process(args: [string])`
Native C class that spawns a process. Expects `args` to be an
array of strings that represent the command line arguments.

Returns `nil` if failed.

#### Method `os.Process.kill(signal: number) -> boolean`
Kill the process with `signal`.

Returns `false` if any errors were encountered, else `true`.

#### Method `os.Process.wait() -> number | nil`
Wait for the process to exit, and return its exit code.

Returns `nil` if failed.

## Regex
### Class `Regex(pattern: string)`
Native C class that compiles a regular expression.

Returns `nil` if failed.

## String
Methods for the builtin string value.

### Method `string.slice(start: number, end: number) -> string`
Slice a string from `start` (inclusive) to `end` (exclusive).

### Method `string.reverse() -> string`
Reverse a string.

### Method `string.toupper() -> string`
Convert a string to uppercase.

### Method `string.tolower() -> string`
Convert a string to lowercase.

### Method `string.tonumber() -> number | nil`
Convert a string to a number.

### Method `string.find(pattern: string | Regex, start?: number) -> number | nil`
Find `pattern` within a string starting from position `start` (which defaults to `0`).

Returns the position if found, else `nil`.

### Method `string.split(pattern: string | Regex) -> [string]`
Split string by `pattern`.

### Method `string.replace(pattern: string | Regex, replacement: string) -> string`
Replace `pattern` with `replacement`.

### Method `string.ltrim(pattern: string) -> string`
Trim `pattern` from the left of a string.

### Method `string.rtrim(pattern: string) -> string`
Trim `pattern` from the right of a string.

### Method `string.trim(pattern: string) -> string`
Trim `pattern` from both sides of a string.

### Method `string.lpad(pattern: string, count: number) -> string`
Pad string with `pattern` on the left side, till the total length reaches `count`.

### Method `string.rpad(pattern: string, count: number) -> string`
Pad string with `pattern` on the right side, till the total length reaches `count`.

## Bit
Contains bitwise operations which are not frequent enough to warrant a
dedicated operator, but which still finds occasional uses.

### Function `bit.ceil(n: number) -> number`
Return the bitwise ceiling of a number.

### Function `bit.floor(n: number) -> number`
Return the bitwise floor of a number.

## ASCII
Contains functions for dealing with ASCII codes and characters.

### Function `ascii.char(code: number) -> string`
Return the character associated with an ASCII code.

### Function `ascii.code(char: string) -> number`
Return the ASCII code associated with a character.

Expects `char` to be a string of length `1`.

### Function `ascii.isalnum(char: string) -> boolean`
Return whether a character is an alphabet or a digit.

Expects `char` to be a string of length `1`.

### Function `ascii.isalpha(char: string) -> boolean`
Return whether a character is an alphabet.

Expects `char` to be a string of length `1`.

### Function `ascii.iscntrl(char: string) -> boolean`
Return whether a character is a control character.

Expects `char` to be a string of length `1`.

### Function `ascii.isdigit(char: string) -> boolean`
Return whether a character is a digit.

Expects `char` to be a string of length `1`.

### Function `ascii.islower(char: string) -> boolean`
Return whether a character is lowercase.

Expects `char` to be a string of length `1`.

### Function `ascii.isupper(char: string) -> boolean`
Return whether a character is uppercase.

Expects `char` to be a string of length `1`.

### Function `ascii.isgraph(char: string) -> boolean`
Return whether a character is graphable.

Expects `char` to be a string of length `1`.

### Function `ascii.isprint(char: string) -> boolean`
Return whether a character is printable.

Expects `char` to be a string of length `1`.

### Function `ascii.ispunct(char: string) -> boolean`
Return whether a character is a punctuation.

Expects `char` to be a string of length `1`.

### Function `ascii.isspace(char: string) -> boolean`
Return whether a character is whitespace.

Expects `char` to be a string of length `1`.

## Bytes
Strings are immutable in BS. The native class `Bytes` provides a mutable string builder for optimized string modification operations.

### Class `Bytes()`
Create a string builder.

### Method `Bytes.count() -> number`
Return the current number of bytes written.

### Method `Bytes.reset(position: number) -> string`
Move back the writer head to `position`.

Also create a string from `position` to the end of the `Bytes` instance and return it.

### Method `Bytes.slice(start: number, end: number) -> string`
Return a slice from `start` (inclusive) to `end` (exclusive).

### Method `Bytes.write(value: string)`
Write `value` to the end.

### Method `Bytes.insert(position: number, value: string)`
Write `value` at `position`.

## Array
Methods for the builtin array value.

### Method `array.map(f: function) -> array`
Functional map.

The provided function `f` must take a single argument.

### Method `array.filter(f: function) -> array`
Functional filter.

The provided function `f` must take a single argument.

### Method `array.reduce(f: function, accumulator?: any) -> any`
Functional reduce.

The provided function `f` must take two arguments. The first
argument shall be the accumulator, and the second shall be the
current value.

### Method `array.copy() -> array`
Copy an array.

### Method `array.join(separator: string) -> string`
Join the elements of an array, separated by `separator` into a single string.

### Method `array.find(value: any, start?: number) -> number | nil`
Find `value` within an array starting from position `start` (which defaults to `0`).

Returns the position if found, else `nil`.

### Method `array.equal(that: array) -> boolean`
Compare the elements of two arrays.

### Method `array.push(value: any)`
Push `value` into an array.

This modifies the array.

### Method `array.insert(position: number, value: any)`
Insert `value` into an array at `position`.

This modifies the array.

### Method `array.sort(compare: function) -> array`
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

The compare function must take two arguments. If it returns `true`, then the left argument shall be considered "less than", and vice versa for `false`.

### Method `array.resize(size: number) -> array`
Resize an array to one having `size` elements.

If `size` is larger than the original size, the extra elements shall default to `nil`.

This modifies the array.

### Method `array.reverse() -> array`
Reverse an array.

This modifes the array.

## Table
Methods for the builtin table value.

### Method `table.copy() -> table`
Copy a table.

### Method `table.equal(that: table) -> boolean`
Compare the elements of two tables.

## Math
Contains simple mathematical primitives.

The methods defined here work on numbers.

### Method `number.sin() -> number`
Sine in radians.

### Method `number.cos() -> number`
Cosine in radians.

### Method `number.tan() -> number`
Tangent in radians.

### Method `number.asin() -> number`
Inverse sine in radians.

### Method `number.acos() -> number`
Inverse cosine in radians.

### Method `number.atan() -> number`
Inverse tangent in radians.

### Method `number.exp() -> number`
Return the exponential function of the number.

### Method `number.log() -> number`
Return the natural logarithm (base `e`) of the number.

### Method `number.log10() -> number`
Return the common logarithm (base `10`) of the number.

### Method `number.pow(exponent: number) -> number`
Raise the number to `exponent`.

### Method `number.sqrt() -> number`
Square root.

### Method `number.ceil() -> number`
Ceiling.

### Method `number.floor() -> number`
Floor.

### Method `number.round() -> number`
Return the nearest integer.

### Method `number.max(those ...number) -> number`
Return the maximum between the method receiver and the provided arguments.

### Method `number.min(those ...number) -> number`
Return the minimum between the method receiver and the provided arguments.

### Method `number.clamp(low: number, high: number) -> number`
Clamp the number between `low` and `high`.

### Method `number.lerp(a: number, b: number, t: number) -> number`
Linear interpolation.

### Number `math.E`
Euler's constant.

### Number `math.PI`
PI.

### Function `math.random() -> number`
Return a pseudorandom number between `0` and `1`.
