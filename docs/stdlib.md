# Stdlib Documentation
BS has a very minimal standard library which just has the bare essentials. This keeps the core runtime simple and easy to embed.

## IO
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
