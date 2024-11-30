# Tutorial

<!-- home-icon -->

## Introduction
Let's start with a simple "Hello World" program.

```bs
io.println("Hello, world!")
```

Save this to a file named `hello.bs` and run it as follows.

```console
$ bs hello.bs
Hello, world!
```

## Basic Types
### Nil
```bs
io.println(nil) # Output: nil
```

### Numbers
BS supports 64-bit floating point numbers only.

```bs
io.println(69)     # Output: 69
io.println(420.69) # Output: 420.69
io.println(0xff)   # Output: 255
```

Standard arithmetic as well as bitwise operations are supported.

```bs
io.println(34 + 35)        # Output: 69
io.println(500 - 80)       # Output: 420
io.println(23 * 3)         # Output: 69
io.println(840 / 2)        # Output: 420
io.println(209 % 70)       # Output: 69
io.println(-420)           # Output: -420

io.println(17 * 2 + 35)    # Output: 69
io.println((900 - 60) / 2) # Output: 420

io.println(276 >> 2)       # Output: 69
io.println(105 << 2)       # Output: 420
io.println(~-70)           # Output: 69
io.println(160 | 260)      # Output: 420
io.println(77 & 103)       # Output: 69
io.println(~-419 + 10 & 3) # Output: 420
io.println(69 ^ 1404)      # Output: 1337
```

### Booleans
```bs
io.println(true)  # Output: true
io.println(false) # Output: false
```

Standard logical operations are supported.

```bs
io.println(!true)          # Output: false
io.println(!false)         # Output: true
io.println(!nil)           # Output: true

# All numbers are considered "true"
io.println(!0)             # Output: false
io.println(!1)             # Output: false

# Ordering
io.println(69 > 420)       # Output: false
io.println(69 >= 420)      # Output: false
io.println(69 < 420)       # Output: true
io.println(69 <= 420)      # Output: true
io.println(69 == 420)      # Output: false
io.println(69 != 420)      # Output: true

# BS is strongly typed
io.println(nil == nil)     # Output: true
io.println(nil == true)    # Output: false
io.println(nil == 0)       # Output: false
io.println(false == 0)     # Output: false

# Logical AND, OR
io.println(true && true)   # Output: true
io.println(true && false)  # Output: false
io.println(true || false)  # Output: true
io.println(false || false) # Output: false

# Short circuiting
io.println(nil && true)    # Output: nil
io.println(0 || false)     # Output: 0
```

### Strings
```bs
# Output: Hello!
io.println("Hello!")

# Output:
# Say "Hello"!
# Here, a	 tab!
io.println("Say \"Hello\"!\nHere, a\t tab!")

# Output: Interpolation! 69
io.println("Interpolation! \(34 + 35)")

# Output: Nested interpolation! 420
io.println("Nested \("interpolation! \(420)")")

# Output: 6
io.println(len("Hello!"))

# Output: b
io.println("foobar"[3])

# Output: Let's go!
io.println('Let\'s go!') # Single quotes can also be used
```

#### (`$`)
```bs
# Unary ($) converts any value to a string
io.println(len($(21 * 20)))            # Output: 3

# Binary ($) performs string concatenation
io.println("Hello, " $ "world! " $ 69) # Output: Hello, world! 69
```

<blockquote>
<b>Q.</b> Why (<code>$</code>) of all things? Couldn't you have chosen a more sensible operator?
<br>
<b>A.</b> In my defense, I am just a silly goose.
</blockquote>

### Arrays
```bs
# Variables will be introduced later
var array = [69, 420]

# Pretty printing by default!
io.println(array)        # Output: [69, 420]

# Array access
io.println(array[0])     # Output: 69

# Array access out of bounds is an error
io.println(array[2])     # Error!

# Array assignment
array[1] = "nice!"
io.println(array)        # Output: [69, "nice!"]

# Array assignment out of bounds is NOT an error
array[3] = "Are you serious?"
io.println(array)        # Output: [69, "nice!", nil, "Are you serious?"]

# Array length
io.println(len(array))   # Output: 4

# Due to the assignment semantics, appending to arrays is quite easy
array[len(array)] = 420
io.println(array)        # Output: [69, "nice!", nil, "Are you serious?", 420]

# Of course, you can also use the push() method of arrays
array.push(1337)

# Output:
# [
#     69,
#     "nice!",
#     nil,
#     "Are you serious?",
#     420,
#     1337
# ]
io.println(array)

# Arrays are compared by reference, not value
var xs = [1, 2, 3]
var ys = [1, 2, 3]
var zs = xs
io.println(xs == ys)     # Output: false
io.println(xs == zs)     # Output: true

# To compare by value, use the equal() method of arrays
io.println(xs.equal(ys)) # Output: true
```

### Tables
```bs
var table = {
    foo = 69,
    [34 + 35] = 420
}

# Output:
# {
#     foo = 69,
#     [69] = 420
# }
io.println(table)

# Key access
io.println(table.foo)             # Output: 69
io.println(table[69])             # Output: 420

# Key assignment
table.key = "value"
table["bar" $ 69] = "eh"

# Output:
# {
#     bar69 = "eh",
#     foo = 69,
#     key = "value",
#     [69] = 420
# }
io.println(table)

# Undefined key access is an error
io.println(table.something)       # Error!

# Check if key exists in a table
io.println("foo" in table)        # Output: true
io.println("something" in table)  # Output: false

# Check if key doesn't exist in a table
io.println("foo" !in table)       # Output: false
io.println("something" !in table) # Output: true

# Table length
io.println(len(table))            # Output: 4

# Delete keys
io.println(delete(table[69]))     # Output: true

# Output:
# {
#     bar69 = "eh",
#     foo = 69,
#     key = "value",
# }
io.println(table)

# Deletion of non existent key
io.println(delete(table.wrong))   # Output: false

# Tables are compared by reference, not value
var xs = {a = 1, b = 2}
var ys = {a = 1, b = 2}
var zs = xs
io.println(xs == ys)              # Output: false
io.println(xs == zs)              # Output: true

# To compare by value, use the equal() method of tables
io.println(xs.equal(ys))          # Output: true
```

### Typeof
```bs
io.println(typeof(nil))      # Output: nil
io.println(typeof(true))     # Output: boolean
io.println(typeof(69))       # Output: number
io.println(typeof("deez"))   # Output: string
io.println(typeof([]))       # Output: array
io.println(typeof({}))       # Output: table

# Functions will be introduced later
io.println(typeof(fn () {})) # Output: function
```

#### Is
```bs
io.println(nil is "nil")           # Output: true
io.println(true is "boolean")      # Output: true
io.println(69 is "number")         # Output: true
io.println("deez" is "string")     # Output: true
io.println([] is "array")          # Output: true
io.println({} is "table")          # Output: true
io.println(fn () {} is "function") # Output: true

io.println(420 is "nil")           # Output: false
io.println(420 !is "nil")          # Output: true
```

## Semicolons
Semicolons can be used to mark the end of an expression optionally. This is not
necessary though, since BS performs automatic semicolon insertion based on
newlines similarly to Go.


```bs
# Output:
# 69
# 420
io.println(69); io.println(420)
```

This does mean, however, that placement of binary operators matter.

```bs
# => 100 - 31;
100 - 31

# => 100 - 31;
100 -
31

# => 100; -31;
100
- 31
```

Basically binary operators cannot start on a newline. If you wish to split an
expression across multiple lines, the operators have to kept on the same line
if you wish the next line to be part of that expression.

The field access operator (`.`) is the only exception to this rule.

```bs
# Considered part of the same expression, even though the binary operator (.)
# starts on a new line.
something
    .foo
    .bar(deez, nuts)
    .baz

# The above expression is equivalent to this.
something.foo.bar(deez, nuts).baz
```

## Conditions
### If Statement
```bs
var age = 18

if age >= 18 {
    io.println("You are an adult")
} else {
    io.println("Stay away from Drake")
}

if age == 18 {
    io.println("Get a life")
}

if age != 18 {
    io.println("Go to school")
}
```

```console
$ bs conditions.bs
You are an adult
Get a life
```

### If Expression
```bs
var age = 17
io.println(if age >= 18 then "Adult" else "Minor")
```

```console
$ bs conditions.bs
Minor
```

### Match Statement
```bs
# Output: A
match 69 {
    69 => io.println("A")
    420 => {
        io.println("B")
    }
}

# Output: B
match 420 {
    69 => io.println("A")
    420 => {
        io.println("B")
    }
}

match 1337 {
    69 => io.println("A")
    420 => {
        io.println("B")
    }

    # No output in this case, since none of the cases match
}

# Output: C
match 1337 {
    69 => io.println("A")
    420 => {
        io.println("B")
    }
} else {
    # Executed when none of the cases match
    io.println("C")
}

# Output: C
match 42 {
    # Multiple cases for the same branch
    0, 1 => io.println("A")
    69, 420, 1337 => {
        io.println("B")
    }
    42 => io.println("C")
} else {
    io.println("D")
}

# Output: A
match 0 {} else {
    # Why would you do this though? :/
    io.println("A")
}

var x = "foo"
var y = "bar"

# Output:
# Side effect!
# x
match "foobar".slice(0, 3) { # All values can be matched
    y => io.println("y")

    # Expressions are allowed
    22 + 10 => io.println("Deez")

    # Arbritary runtime code in general is allowed
    os.clock() => panic("What?")

    # And you thought JS was bad
    (fn () {
        # 1. Functions will be described later
        # 2. The order of operations matter. If a matching case was encountered
        #    before this, then this side effect would not have occured
        io.println("Side effect!")
    })() => {
        io.println("Why?")
    }

    x => io.println("x")
}
```

## Loops
### While Loop
```bs
var i = 0
while i < 5 {
    io.println(i)
    i = i + 1
}
```

```console
$ bs loops.bs
0
1
2
3
4
```

### For Loop
Iterate over range of numbers.

```bs
for i in 0, 5 {
    io.println(i)
}
```

```console
$ bs loops.bs
0
1
2
3
4
```

Iterate over range of numbers with custom step.

```bs
for i in 0, 5, 2 {
    io.println(i)
}
```

```console
$ bs loops.bs
0
2
4
```

Direction of the range iteration is automatically selected.

```bs
for i in 5, 0 {
    io.println(i)
}
```

```console
$ bs loops.bs
5
4
3
2
1
```

Iterate over a string.

```bs
var str = "foobar"

for i, c in str {
    io.println("Index: \(i) Char: \(c)")
}
```

```console
$ bs loops.bs
Index: 0 Char: f
Index: 1 Char: o
Index: 2 Char: o
Index: 3 Char: b
Index: 4 Char: a
Index: 5 Char: r
```

Iterate over an array.

```bs
var xs = [2, 4, 6, 8, 10]

for i, v in xs {
    io.println("Index: \(i) Value: \(v)")
}
```

```console
$ bs loops.bs
Index: 0 Value: 2
Index: 1 Value: 4
Index: 2 Value: 6
Index: 3 Value: 8
Index: 4 Value: 10
```

Iterate over a table.

```bs
var xs = {
    foo = 69,
    bar = 420,
    [42] = 1337
}

for k, v in xs {
    io.println("Key: \(k) Value: \(v)")
}
```

```console
$ bs loops.bs
Key: 42 Value: 1337
Key: bar Value: 420
Key: foo Value: 69
```

### Break and Continue

```bs
var i = 0
while i < 10 {
    if i == 5 {
        break
    }

    io.println(i)
    i = i + 1
}

io.println()

for i in 0, 5 {
    if i == 3 {
        continue
    }
    io.println(i)
}
```

```console
$ bs loops.bs
0
1
2
3
4

0
1
2
4
```

## Functions
```bs
fn greet(name) {
    io.println("Hello, \(name)!")
}

greet("world")
```

```console
$ bs functions.bs
Hello, world!
```

### Return
```bs
fn factorial(n) {
    if n < 2 {
        return 1
    }

    return n * factorial(n - 1)
}

io.println(factorial(6))

fn f() {}
fn g() {
    return

    io.println("HERE!")
}

# Functions implicitly return 'nil'
io.println(f())
io.println(g())
```

```console
$ bs functions.bs
720
nil
nil
```

### First Class Functions
```bs
fn add(x, y) {
    return x + y
}

fn combine(f, x, y) {
    return f(x, y)
}

io.println(combine(add, 34, 35))
```

```console
$ bs functions.bs
69
```

### Anonymous Functions
```bs
fn combine(f, x, y) {
    return f(x, y)
}

io.println(combine(fn (x, y) {
    return x + y
}, 34, 35))
```

```console
$ bs functions.bs
69
```

### Single Expression Function
Functions support a shorthand syntax for a single expression body.

```bs
fn combine(f, x, y) => f(x, y)

io.println(combine(fn (x, y) => x + y, 34, 35))
```

```console
$ bs functions.bs
69
```

### Closures

```bs
fn outer(x) {
    return fn () {
        return x * 2
    }
}

var a = outer(34.5)
var b = outer(210)

io.println(a())
io.println(b())
```

```console
$ bs functions.bs
69
420
```

### Looped Closures
The variables defined in the body of a loop, as well as the iterators, are
captured as unique values in the nested closure on each iteration of the loop.

```bs
var closures = []

for i in 0, 5 {
    var z = i * 2
    closures[len(closures)] = fn () {
        io.println(i, z)
    }
}

for _, f in closures {
    f()
}
```

```console
$ bs functions.bs
0 0
1 2
2 4
3 6
4 8
```

## Variables
```bs
var a = 34
var b = 35
io.println(a + b) # Output: 69
```

### Assignment
```bs
var a = 17
var b             # Assigned to 'nil'

a = a * 2
b = 35

io.println(a + b) # Output: 69
```

Shorthand assignment operators also work.

```bs
var a = 17

# All arithmetic operators are supported
a *= 2
a += 35

var b = "Nice! "

# String concatenation is also supported
b $= a

io.println(b) # Output: Nice! 69
```

### Scoped Variables
```bs
var x = 69
io.println(x)     # Output: 69

{
    var x = 420
    io.println(x) # Output: 420
}

io.println(x)     # Output: 69
```

### Variable Shadowing
```bs
var x = 69
io.println(x) # Output: 69

var x = "x used to be \(x)"
io.println(x) # Output: x used to be 69
```

### Local Variables
```bs
var z = 420 # Local to the scope of the file

fn add(x, y) {
    var z = x + y # Local to the scope of the function
    io.println(z)
}

add(34, 35)
io.println(z)
```

```console
$ bs variables.bs
69
420
```

### File Local Variables
Here's a pitfall you might run into

```bs
fn f() {
    io.println(x) # Refers to the variable 'x'
}

var x = 69        # Variable 'x' now defined at the toplevel

f()               # "Should" print 69
```

```console
$ bs variables.bs
variables.bs:2:16: error: undefined identifier 'x'

    2 |     io.println(x) # Refers to the variable 'x'
      |                ^

variables.bs:7:2: in f()

    7 | f()               # "Should" print 69
      |  ^
```

What is going on?

### Global Variables
Variables defined at the toplevel are local to the scope of the file, which
behaves as a function itself. To put simply, variables cannot be used before
they are declared. This is where global variables come into play.

```bs
fn f() {
    io.println(x) # Refers to the variable 'x'
}

pub var x = 69    # Global variable 'x' now defined

f()
```

```console
$ bs variables.bs
69
```

## Import
```bs
# one.bs
var M = {}

M.inc = fn (n) => n + 1
M.dec = fn (n) => n - 1

return M # Any arbitrary value can be returned, this is just the usual pattern
```

```bs
# main.bs
var one = import("one") # Note that the extension is omitted

io.println(one.inc(68))
io.println(one.dec(421))
```

```console
$ bs main.bs
69
420
```

### Singular Load
Modules are loaded only once.

```bs
# one.bs
var M = {}

io.println("Loading module 'one'")

M.inc = fn (n) => n + 1
M.dec = fn (n) => n - 1

return M
```

```bs
# main.bs
io.println(import("one").inc(68))
io.println(import("one").dec(421))
```

```console
$ bs main.bs
Loading module 'one'
69
420
```

### Main Module
```bs
# one.bs
var M = {}

if is_main_module {
    io.println("Loading module 'one'")
}

M.inc = fn (n) => n + 1
M.dec = fn (n) => n - 1

return M
```

```bs
# main.bs
io.println(import("one").inc(68))
io.println(import("one").dec(421))
```

```console
$ bs main.bs
69
420
```

But if the module `one` is executed directly...

```console
$ bs one.bs
Loading module 'one'
```

### Extended Modules
Extended modules can be imported within normal ones, and vice versa. If there
is an extended as well as a normal module with the same name, the normal one
will be imported.

## OOP
I refuse to showcase the `Animal > Dog, Cat` nonsense, so here's a
more applicable, albeit involved, example.

```bs
class Sprite {
    # Constructor
    init(name, health, attack_power) {
        this.name = name
        this.health = health
        this.attack_power = attack_power
    }

    attack(target) {
        io.println("\(this.name) attacks \(target.name) for \(this.attack_power) damage!")
        target.take_damage(this.attack_power)
    }

    take_damage(amount) {
        this.health -= amount
        if this.is_alive() {
            io.println("\(this.name) has \(this.health) health remaining.")
        } else {
            io.println("\(this.name) has been defeated!")
        }
    }

    is_alive() {
        return this.health > 0
    }
}

# Inheritance
class Hero < Sprite {
    init(name, health, attack_power) {
        super.init(name, health, attack_power)
        this.poisoned = false
        this.defending = false
    }

    # Special ability of hero: defending
    defend() {
        io.println("\(this.name) is defending!")
        this.defending = true
    }

    take_damage(amount) {
        if this.defending {
            if this.poisoned {
                # Hero will not be poisoned if defending
                this.poisoned = false
            } else {
                # Hero will take half damage for a normal attack if defending
                amount /= 2
            }

            this.defending = false
        }

        super.take_damage(amount)
    }
}

class Enemy < Sprite {
    init(name, health, attack_power) {
        super.init(name, health, attack_power)
    }

    # Special ability of enemy: poisoning
    poison(target) {
        io.println("\(this.name) poisons \(target.name)!")
        target.poisoned = true
        target.take_damage(this.attack_power)
    }
}

# io.input() is a core library function which optionally prints a prompt and
# reads a line from standard input
var hero = Hero(io.input("Enter hero's name> "), 30, 10)
var enemy = Enemy(io.input("Enter enemy's name> "), 25, 8)

# Create a random number generator
var rng = math.Random()

while hero.is_alive() && enemy.is_alive() {
    io.println()

    # Hero cannot choose while poisoned
    if hero.poisoned {
        io.println("\(hero.name) has been poisoned! skipping turn.")

        # Prevent consecutive poisoning
        hero.poisoned = false
        enemy.attack(hero)
    } else {
        var choice = io.input("Enter \(hero.name)'s choice (A: attack, D: defend)> ").toupper()

        if choice == "A" {
            hero.attack(enemy)
            if !enemy.is_alive() {
                break
            }
        } else if choice == "D" {
            hero.defend()
        } else {
            io.println("ERROR: invalid choice! skipping \(hero.name)'s turn.")
        }

        # Enemy will either attack or poison the hero based on a 50-50 chance
        if rng.number() >= 0.5 {
            enemy.poison(hero)
        } else {
            enemy.attack(hero)
        }
    }
}

if hero.is_alive() {
    io.println("\(hero.name) won!")
} else {
    io.println("\(hero.name) lost :(")
}
```

```console
$ bs oop.bs
Enter hero's name> Luffy
Enter enemy's name> Magellan

Enter Luffy's choice (A: attack, D: defend)> a
Luffy attacks Magellan for 10 damage!
Magellan has 15 health remaining.
Magellan poisons Luffy!
Luffy has 22 health remaining.

Luffy has been poisoned! skipping turn.
Magellan attacks Luffy for 8 damage!
Luffy has 14 health remaining.

Enter Luffy's choice (A: attack, D: defend)> a
Luffy attacks Magellan for 10 damage!
Magellan has 5 health remaining.
Magellan poisons Luffy!
Luffy has 6 health remaining.

Luffy has been poisoned! skipping turn.
Magellan attacks Luffy for 8 damage!
Luffy has been defeated!
Luffy lost :(
```

### Constructor Failure
Typical OOP languages mandate that constructors must return the instance under
all circumstances, even at the cost of error handling. In BS however,
constructors may return `nil` in order to indicate that the
constructor failed. Any other return value is strictly forbidden, however.

```bs
class Logger {
    init(path) {
        # io.Writer is a core C class that handles writeable files. It can fail
        this.file = io.Writer(path)
        if !this.file {
            # Could not create logger file, return 'nil' to indicate failure
            return nil
        }
    }

    write(s) {
        # os.clock() returns the current monotonic time in seconds as a
        # floating point number
        this.file.writeln("\(os.clock()): \(s)")
    }
}

var logger = Logger("log.txt")
if !logger {
    # io.eprintln() prints to standard error
    io.eprintln("Error: could not create logger")

    # os.exit() exits the program with the provided exit code
    os.exit(1)
}

logger.write("Hello, world!")
```

### How to know if a class can fail?
Just print the class.

```bs
class Foo {
    init(x) {
        if x == 69 {
            return nil
        }

        this.x = x
    }
}

class Bar {
    init(x) {
        this.x = x
    }
}

io.println(Foo)
io.println(Bar)
```

```console
$ bs oop.bs
class Foo {
    # Can fail
}
class Bar {}
```

### How to know the methods in a class?
Just print the class.

```bs
class Lol {
    foo() {}
    bar() {}
    baz() {}
}

io.println(Lol)
```

```console
$ bs oop.bs
class Lol {
    baz = <fn>,
    bar = <fn>,
    foo = <fn>
}
```

## Panic
```bs
panic()
```

```console
$ bs panic.bs
panic/panic.bs:1:1: panic
```

An optional message can also be provided.

```bs
panic("TODO")
```

```console
$ bs panic.bs
panic/panic.bs:1:1: TODO
```

## Assert
```bs
assert(true)
assert(false)
```

```console
$ bs assert.bs
assert.bs:2:1: assertion failed
```

An optional message can also be provided.

```bs
assert(true, "Ligma")
assert(false, "Ligma")
```

```console
$ bs assert.bs
assert.bs:2:1: Ligma
```

Assert returns the value being checked.

```bs
var x = assert(69, "Ligma")
io.println(x)
```

```console
$ bs assert.bs
69
```

## FFI
BS supports loading of native modules at runtime.

### Compiling a native module
Create a file `arithmetic.c` with the following content

```c
// arithmetic.c

#include <bs/object.h>

Bs_Value arithmetic_add(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);
    return bs_value_num(args[0].as.number + args[1].as.number);
}

Bs_Value arithmetic_sub(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 2);
    bs_arg_check_value_type(bs, args, 0, BS_VALUE_NUM);
    bs_arg_check_value_type(bs, args, 1, BS_VALUE_NUM);
    return bs_value_num(args[0].as.number - args[1].as.number);
}

// This is the "entry point" of the library. This function will be called when
// the native library is loaded into memory. Prepare the FFI here
BS_LIBRARY_INIT void bs_library_init(Bs *bs, Bs_C_Lib *library) {
    static const Bs_FFI ffi[] = {
        {"add", arithmetic_add},
        {"sub", arithmetic_sub},
    };
    bs_c_lib_ffi(bs, library, ffi, bs_c_array_size(ffi));
}
```

Compile it into a dynamic library, linking against `bs`

```console
$ cc -o arithmetic.so -fPIC -shared arithmetic.c -lbs # On Linux
$ cc -o arithmetic.dylib -fPIC -shared arithmetic.c -lbs # On macOS
$ cl /LD /Fe:arithmetic.dll arithmetic.c bs.lib # On Windows
```

Finally, load it from BS.

```bs
# ffi.bs

var arith = import("arithmetic")
io.println(arith.add(34, 35))
io.println(arith.sub(500, 80))
```

```console
$ bs ffi.bs
69
420
```

### Wrapping an existing C library
As an example let's create a simple wrapper around the
<a href="https://www.raylib.com/"><code>raylib</code></a>
library.

```c
// raylib.c

#include "raylib.h"
#include "bs/object.h"

Bs_Value rl_init_window(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 3);
    bs_arg_check_whole_number(bs, args, 0);
    bs_arg_check_whole_number(bs, args, 1);
    bs_arg_check_object_type(bs, args, 2, BS_OBJECT_STR);

    const int width = args[0].as.number;
    const int height = args[1].as.number;
    const Bs_Str *title = (const Bs_Str *)args[2].as.object;

    // BS strings are null terminated by default for FFI convenience
    InitWindow(width, height, title->data);
    return bs_value_nil;
}

Bs_Value rl_close_window(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    CloseWindow();
    return bs_value_nil;
}

Bs_Value rl_window_should_close(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    return bs_value_bool(WindowShouldClose());
}

Bs_Value rl_begin_drawing(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    BeginDrawing();
    return bs_value_nil;
}

Bs_Value rl_end_drawing(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 0);
    EndDrawing();
    return bs_value_nil;
}

Bs_Value rl_clear_background(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 1);
    bs_arg_check_whole_number(bs, args, 0);
    ClearBackground(GetColor(args[0].as.number));
    return bs_value_nil;
}

Bs_Value rl_draw_text(Bs *bs, Bs_Value *args, size_t arity) {
    bs_check_arity(bs, arity, 5);
    bs_arg_check_object_type(bs, args, 0, BS_OBJECT_STR);
    bs_arg_check_whole_number(bs, args, 1);
    bs_arg_check_whole_number(bs, args, 2);
    bs_arg_check_whole_number(bs, args, 3);
    bs_arg_check_whole_number(bs, args, 4);

    const Bs_Str *text = (const Bs_Str *)args[0].as.object;
    const int x = args[1].as.number;
    const int y = args[2].as.number;
    const int size = args[3].as.number;
    const Color color = GetColor(args[4].as.number);

    DrawText(text->data, x, y, size, color);
    return bs_value_nil;
}

BS_LIBRARY_INIT void bs_library_init(Bs *bs, Bs_C_Lib *library) {
    static const Bs_FFI ffi[] = {
        {"init_window", rl_init_window},
        {"close_window", rl_close_window},
        {"window_should_close", rl_window_should_close},
        {"begin_drawing", rl_begin_drawing},
        {"end_drawing", rl_end_drawing},
        {"clear_background", rl_clear_background},
        {"draw_text", rl_draw_text},
    };
    bs_c_lib_ffi(bs, library, ffi, bs_c_array_size(ffi));
}
```

Compile it into a dynamic library, linking against `bs` and `raylib`

```console
$ cc -o raylib.so -fPIC -shared raylib.c -lbs -lraylib    # On Linux
$ cc -o raylib.dylib -fPIC -shared raylib.c -lbs -lraylib # On macOS
$ cl /LD /Fe:raylib.dll raylib.c bs.lib raylib.lib        # On Windows
```

Finally, load it from BS.

```bs
var rl = import("raylib")

rl.init_window(800, 600, "Hello from BS!")
while !rl.window_should_close() {
    rl.begin_drawing()
    rl.clear_background(0x282828FF)
    rl.draw_text("Hello world!", 50, 50, 50, 0xD4BE98FF)
    rl.end_drawing()
}
rl.close_window()
```
