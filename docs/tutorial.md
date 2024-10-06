# Tutorial

## Introduction
Let's start with a simple "Hello World" program.

```bs
io.println("Hello, world!");
```
```bsx
io.println("Hello, world!") fr
```

Save this to a file named `hello.bs` and run it as follows.

```console
$ bs hello.bs
Hello, world!
```

## BS and BSX
BS is more "mainstream" in its syntax, whereas BSX is a parody of Gen Z
brainrot. There are subtle differences between the two, even besides syntax
alone, which will be introduced later in appropriate places in the tutorial.

You can choose whether to use BS or BSX based on the extension of your file.
Save the BSX version of the previous example to `hello.bsx` and run
it as before.

```console
$ bs hello.bsx
Hello, world!
```

## Basic Types
### Nil
```bs
io.println(nil); # Output: nil
```
```bsx
io.println(bruh) fr # Output: bruh
```

Note that the output differs based on whether you are using BS or BSX.

### Numbers
BS supports 64-bit floating point numbers only.

```bs
io.println(69);     # Output: 69
io.println(420.69); # Output: 420.69
io.println(0xff);   # Output: 255
```
```bsx
io.println(69) fr     # Output: 69
io.println(420.69) fr # Output: 420.69
io.println(0xff) fr   # Output: 255
```

Standard arithmetic as well as bitwise operations are supported.

```bs
io.println(34 + 35);        # Output: 69
io.println(500 - 80);       # Output: 420
io.println(23 * 3);         # Output: 69
io.println(840 / 2);        # Output: 420
io.println(209 % 70);       # Output: 69
io.println(-420);           # Output: -420

io.println(17 * 2 + 35);    # Output: 69
io.println((900 - 60) / 2); # Output: 420

io.println(276 >> 2);       # Output: 69
io.println(105 << 2);       # Output: 420
io.println(~-70);           # Output: 69
io.println(160 | 260);      # Output: 420
io.println(77 & 103);       # Output: 69
io.println(~-419 + 10 & 3); # Output: 420
io.println(69 ^ 1404);      # Output: 1337
```
```bsx
io.println(34 + 35) fr        # Output: 69
io.println(500 - 80) fr       # Output: 420
io.println(23 * 3) fr         # Output: 69
io.println(840 / 2) fr        # Output: 420
io.println(209 % 70) fr       # Output: 69
io.println(-420) fr           # Output: -420

io.println(17 * 2 + 35) fr    # Output: 69
io.println((900 - 60) / 2) fr # Output: 420

io.println(276 >> 2) fr       # Output: 69
io.println(105 << 2) fr       # Output: 420
io.println(~-70) fr           # Output: 69
io.println(160 | 260) fr      # Output: 420
io.println(77 & 103) fr       # Output: 69
io.println(~-419 + 10 & 3) fr # Output: 420
io.println(69 ^ 1404) fr      # Output: 1337
```

### Booleans
```bs
io.println(true);  # Output: true
io.println(false); # Output: false
```
```bsx
io.println(nocap) fr # Output: nocap
io.println(cap) fr   # Output: cap
```

Again, note that the output differs depending on the mode.

```bs
io.println(!true);          # Output: false
io.println(!false);         # Output: true
io.println(!nil);           # Output: true

# All numbers are considered "true"
io.println(!0);             # Output: false
io.println(!1);             # Output: false

# Ordering
io.println(69 > 420);       # Output: false
io.println(69 >= 420);      # Output: false
io.println(69 < 420);       # Output: true
io.println(69 <= 420);      # Output: true
io.println(69 == 420);      # Output: false
io.println(69 != 420);      # Output: true

# BS is strongly typed
io.println(nil == nil);     # Output: true
io.println(nil == true);    # Output: false
io.println(nil == 0);       # Output: false
io.println(false == 0);     # Output: false

# Logical AND, OR
io.println(true && true);   # Output: true
io.println(true && false);  # Output: false
io.println(true || false);  # Output: true
io.println(false || false); # Output: false

# Short circuiting
io.println(nil && true);    # Output: nil
io.println(0 || false);     # Output: 0
```
```bsx
io.println(nah nocap) fr      # Output: cap
io.println(nah cap) fr        # Output: nocap
io.println(nah bruh) fr       # Output: nocap

# All numbers are considered "nocap"
io.println(nah 0) fr          # Output: cap
io.println(nah 1) fr          # Output: cap

# Ordering
io.println(69 > 420) fr       # Output: cap
io.println(69 >= 420) fr      # Output: cap
io.println(69 < 420) fr       # Output: nocap
io.println(69 <= 420) fr      # Output: nocap
io.println(69 == 420) fr      # Output: cap
io.println(69 != 420) fr      # Output: nocap

# BSX is strongly typed
io.println(bruh == bruh) fr   # Output: nocap
io.println(bruh == nocap) fr  # Output: cap
io.println(bruh == 0) fr      # Output: cap
io.println(cap == 0) fr       # Output: cap

# Logical AND, OR
io.println(nocap && nocap) fr # Output: nocap
io.println(nocap && cap) fr   # Output: cap
io.println(nocap || cap) fr   # Output: nocap
io.println(cap || cap) fr     # Output: cap

# Short circuiting
io.println(bruh && nocap) fr  # Output: bruh
io.println(0 || cap) fr       # Output: 0
```

### Strings
```bs
# Output: Hello!
io.println("Hello!");

# Output:
# Say "Hello"!
# Here, a	 tab!
io.println("Say \"Hello\"!\nHere, a\t tab!");

# Output: Interpolation! 69
io.println("Interpolation! \(34 + 35)");

# Output: Nested interpolation! 420
io.println("Nested \("interpolation! \(420)")");

# Output: Hello, world! 69
io.println("Hello, " ++ "world! " ++ 69);

# Output: 6
io.println(len("Hello!"));
```
```bsx
# Output: Hello!
io.println("Hello!") fr

# Output:
# Say "Hello"!
# Here, a	 tab!
io.println("Say \"Hello\"!\nHere, a\t tab!") fr

# Output: Interpolation! 69
io.println("Interpolation! \(34 + 35)") fr

# Output: Nested interpolation! 420
io.println("Nested \("interpolation! \(420)")") fr

# Output: Hello, world! 69
io.println("Hello, " ++ "world! " ++ 69) fr

# Output: 6
io.println(thicc("Hello!")) fr
```

### Arrays
```bs
# Variables will be introduced later
var array = [69, 420];

# Pretty printing by default!
io.println(array);      # Output: [69, 420]

# Array access
io.println(array[0]);   # Output: 69

# Array access out of bounds is an error
io.println(array[2]);   # Error!

# Array assignment
array[1] = "nice!";
io.println(array);      # Output: [69, "nice!"]

# Array assignment out of bounds is NOT an error
array[3] = "Are you serious?";
io.println(array);      # Output: [69, "nice!", nil, "Are you serious?"]

# Array length
io.println(len(array)); # Output: 4

# Due to the assignment semantics, appending to arrays is quite easy
array[len(array)] = 420;
io.println(array);      # Output: [69, "nice!", nil, "Are you serious?", 420]

# Arrays are compared by reference, not value
var xs = [1, 2, 3];
var ys = [1, 2, 3];
var zs = xs;
io.println(xs == ys);   # Output: false
io.println(xs == zs);   # Output: true
```
```bsx
# Variables will be introduced later
mf array = [69, 420] fr

# Pretty printing by default!
io.println(array) fr        # Output: [69, 420]

# Array access
io.println(array[0]) fr     # Output: 69

# Array access out of bounds is an error
io.println(array[2]) fr     # Error!

# Array assignment
array[1] = "nice!" fr
io.println(array) fr        # Output: [69, "nice!"]

# Array assignment out of bounds is NOT an error
array[3] = "Are you serious?" fr
io.println(array) fr        # Output: [69, "nice!", bruh, "Are you serious?"]

# Array length
io.println(thicc(array)) fr # Output: 4

# Due to the assignment semantics, appending to arrays is quite easy
array[thicc(array)] = 420 fr
io.println(array) fr        # Output: [69, "nice!", bruh, "Are you serious?", 420]

# Arrays are compared by reference, not value
mf xs = [1, 2, 3] fr
mf ys = [1, 2, 3] fr
mf zs = xs fr
io.println(xs == ys) fr   # Output: cap
io.println(xs == zs) fr   # Output: nocap
```

### Tables
```bs
var table = {
    foo = 69,
    [34 + 35] = 420
};

# Output:
# {
#     foo = 69,
#     [69] = 420
# }
io.println(table);

# Key access
io.println(table.foo);            # Output: 69
io.println(table[69]);            # Output: 420

# Key assignment
table.key = "value";
table["bar" ++ 69] = "eh";

# Output:
# {
#     bar69 = "eh",
#     foo = 69,
#     key = "value",
#     [69] = 420
# }
io.println(table);

# Undefined key access is an error
io.println(table.something);      # Error!

# Check if key exists in a table
io.println("foo" in table);       # Output: true
io.println("something" in table); # Output: false

# Table length
io.println(len(table));           # Output: 4

# Delete keys
io.println(delete(table[69]));    # Output: true

# Output:
# {
#     bar69 = "eh",
#     foo = 69,
#     key = "value",
# }
io.println(table);

# Deletion of non existent key
io.println(delete(table.wrong));  # Output: false

# Tables are compared by reference, not value
var xs = {a = 1, b = 2};
var ys = {a = 1, b = 2};
var zs = xs;
io.println(xs == ys);             # Output: false
io.println(xs == zs);             # Output: true
```
```bsx
mf table = {
    foo = 69,
    [34 + 35] = 420
} fr

# Output:
# {
#     foo = 69,
#     [69] = 420
# }
io.println(table) fr

# Key access
io.println(table.foo) fr                 # Output: 69
io.println(table[69]) fr                 # Output: 420

# Key assignment
table.key = "value" fr
table["bar" ++ 69] = "eh" fr

# Output:
# {
#     bar69 = "eh",
#     foo = 69,
#     key = "value",
#     [69] = 420
# }
io.println(table) fr

# Undefined key access is an error
io.println(table.something) fr           # Error!

# Check if key exists in a table
io.println("foo" amongus table) fr       # Output: nocap
io.println("something" amongus table) fr # Output: cap

# Table length
io.println(thicc(table)) fr              # Output: 4

# Delete keys
io.println(ghost(table[69])) fr          # Output: nocap

# Output:
# {
#     bar69 = "eh",
#     foo = 69,
#     key = "value",
# }
io.println(table) fr

# Deletion of non existent key
io.println(ghost(table.wrong)) fr        # Output: cap

# Tables are compared by reference, not value
mf xs = {a = 1, b = 2} fr
mf ys = {a = 1, b = 2} fr
mf zs = xs fr
io.println(xs == ys) fr                  # Output: cap
io.println(xs == zs) fr                  # Output: nocap
```

### Typeof
```bs
io.println(typeof(nil));      # Output: nil
io.println(typeof(true));     # Output: boolean
io.println(typeof(69));       # Output: number
io.println(typeof("deez"));   # Output: string
io.println(typeof([]));       # Output: array
io.println(typeof({}));       # Output: table

# Functions will be introduced later
io.println(typeof(fn () {})); # Output: function
```
```bsx
io.println(vibeof(bruh)) fr      # Output: bruh
io.println(vibeof(nocap)) fr     # Output: capness
io.println(vibeof(69)) fr        # Output: number
io.println(vibeof("deez")) fr    # Output: string
io.println(vibeof([])) fr        # Output: array
io.println(vibeof({})) fr        # Output: table

# Functions will be introduced later
io.println(vibeof(lit () {})) fr # Output: function
```

## Conditions
### If Statement
```bs
var age = 18;

if age >= 18 {
    io.println("You are an adult");
} else {
    io.println("Stay away from Drake");
}

if age == 18 {
    io.println("Get a life");
}

if age != 18 {
    io.println("Go to school");
}
```
```bsx
mf age = 18 fr

ayo age >= 18 {
    io.println("You are an adult") fr
} sike {
    io.println("Stay away from Drake") fr
}

ayo age == 18 {
    io.println("Get a life") fr
}

ayo age != 18 {
    io.println("Go to school") fr
}
```

```console
$ bs conditions.bs # or.bsx
You are an adult
Get a life
```

### If Expression
```bs
var age = 17;
io.println(if age >= 18 then "Adult" else "Minor");
```
```bsx
mf age = 17 fr
io.println(ayo age >= 18 sayless "Adult" sike "Minor") fr
```

```console
$ bs conditions.bs # or.bsx
Minor
```

## Loops
### While Loop
```bs
var i = 0;
while i < 5 {
    io.println(i);
    i = i + 1;
}
```
```bsx
mf i = 0 fr
yolo i < 5 {
    io.println(i) fr
    i = i + 1 fr
}
```

```console
$ bs loops.bs # or .bsx
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
    io.println(i);
}
```
```bsx
yall i amongus 0, 5 {
    io.println(i) fr
}
```

```console
$ bs loops.bs # or .bsx
0
1
2
3
4
```

Iterate over range of numbers with custom step.

```bs
for i in 0, 5, 2 {
    io.println(i);
}
```
```bsx
yall i amongus 0, 5, 2 {
    io.println(i) fr
}
```

```console
$ bs loops.bs # or .bsx
0
2
4
```

Direction of the range iteration is automatically selected.

```bs
for i in 5, 0 {
    io.println(i);
}
```
```bsx
yall i amongus 5, 0 {
    io.println(i) fr
}
```

```console
$ bs loops.bs # or .bsx
5
4
3
2
1
```

Iterate over an array.

```bs
var xs = [2, 4, 6, 8, 10];

for i, v in xs {
    io.println("Index: \(i); Value: \(v)");
}
```
```bsx
mf xs = [2, 4, 6, 8, 10] fr

yall i, v amongus xs {
    io.println("Index: \(i); Value: \(v)") fr
}
```

```console
$ bs loops.bs # or .bsx
Index: 0; Value: 2
Index: 1; Value: 4
Index: 2; Value: 6
Index: 3; Value: 8
Index: 4; Value: 10
```

Iterate over a table.

```bs
var xs = {
    foo = 69,
    bar = 420,
    [42] = 1337
};

for k, v in xs {
    io.println("Key: \(k); Value: \(v)");
}
```
```bsx
mf xs = {
    foo = 69,
    bar = 420,
    [42] = 1337
} fr

yall k, v amongus xs {
    io.println("Key: \(k); Value: \(v)") fr
}
```

```console
$ bs loops.bs # or .bsx
Key: 42; Value: 1337
Key: bar; Value: 420
Key: foo; Value: 69
```

### Break and Continue

```bs
var i = 0;
while i < 10 {
    if i == 5 {
        break;
    }

    io.println(i);
    i = i + 1;
}

io.println();

for i in 0, 5 {
    if i == 3 {
        continue;
    }
    io.println(i);
}
```
```bsx
mf i = 0 fr
yolo i < 10 {
    ayo i == 5 {
        yeet fr
    }

    io.println(i) fr
    i = i + 1 fr
}

io.println() fr

yall i amongus 0, 5 {
    ayo i == 3 {
        slickback fr
    }
    io.println(i) fr
}
```

```console
$ bs loops.bs # or .bsx
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
    io.println("Hello, \(name)!");
}

greet("world");
```
```bsx
lit greet(name) {
    io.println("Hello, \(name)!") fr
}

greet("world") fr
```

```console
$ bs functions.bs # or .bsx
Hello, world!
```

### Return
```bs
fn factorial(n) {
    if n < 2 {
        return 1;
    }

    return n * factorial(n - 1);
}

io.println(factorial(6));
```
```bsx
lit factorial(n) {
    ayo n < 2 {
        bet 1 fr
    }

    bet n * factorial(n - 1) fr
}

io.println(factorial(6)) fr
```

```console
$ bs functions.bs # or .bsx
720
```

### First Class Functions
```bs
fn add(x, y) {
    return x + y;
}

fn combine(f, x, y) {
    return f(x, y);
}

io.println(combine(add, 34, 35));
```
```bsx
lit add(x, y) {
    bet x + y fr
}

lit combine(f, x, y) {
    bet f(x, y) fr
}

io.println(combine(add, 34, 35)) fr
```

```console
$ bs functions.bs # or .bsx
69
```

### Anonymous Functions
```bs
fn combine(f, x, y) {
    return f(x, y);
}

io.println(combine(fn (x, y) {
    return x + y;
}, 34, 35));
```
```bsx
lit combine(f, x, y) {
    bet f(x, y) fr
}

io.println(combine(lit (x, y) {
    bet x + y fr
}, 34, 35)) fr
```

```console
$ bs functions.bs # or .bsx
69
```

### Single Expression Function
Functions support a shorthand syntax for a single expression body.

```bs
fn combine(f, x, y) => f(x, y);

io.println(combine(fn (x, y) => x + y, 34, 35));
```
```bsx
lit combine(f, x, y) => f(x, y) fr

io.println(combine(lit (x, y) => x + y, 34, 35)) fr
```

```console
$ bs functions.bs # or .bsx
69
```

### Closures

```bs
fn outer(x) {
    return fn () {
        return x * 2;
    };
}

var a = outer(34.5);
var b = outer(210);

io.println(a());
io.println(b());
```
```bsx
lit outer(x) {
    bet lit () {
        bet x * 2 fr
    } fr
}

mf a = outer(34.5) fr
mf b = outer(210) fr

io.println(a()) fr
io.println(b()) fr
```

```console
$ bs functions.bs # or .bsx
69
420
```

### Looped Closures
The variables defined in the body of a loop, as well as the iterators, are
captured as unique values in the nested closure on each iteration of the loop.

```bs
var closures = [];

for i in 0, 5 {
    var z = i * 2;
    closures[len(closures)] = fn () {
        io.println(i, z);
    };
}

for _, f in closures {
    f();
}
```
```bsx
mf closures = [] fr

yall i amongus 0, 5 {
    mf z = i * 2 fr
    closures[thicc(closures)] = lit () {
        io.println(i, z) fr
    } fr
}

yall _, f amongus closures {
    f() fr
}
```

```console
$ bs functions.bs # or .bsx
0 0
1 2
2 4
3 6
4 8
```

## Variables
```bs
var a = 34;
var b = 35;
io.println(a + b); # Output: 69
```
```bsx
mf a = 34 fr
mf b = 35 fr
io.println(a + b) fr # Output: 69
```

### Assignment
```bs
var a = 17;
var b;             # Assigned to 'nil'

a = a * 2;
b = 35;

io.println(a + b); # Output: 69
```
```bsx
mf a = 17 fr
mf b fr              # Assigned to 'bruh'

a = a * 2 fr
b = 35 fr

io.println(a + b) fr # Output: 69
```

Shorthand assignment operators also work.

```bs
var a = 17;

# All arithmetic operators are supported
a *= 2;
a += 35;

var b = "Nice! ";

# String concatenation is also supported
b ++= a;

io.println(b); # Output: Nice! 69
```
```bsx
mf a = 17 fr

# All arithmetic operators are supported
a *= 2 fr
a += 35 fr

mf b = "Nice! " fr

# String concatenation is also supported
b ++= a fr

io.println(b) fr # Output: Nice! 69
```

### Scoped Variables
```bs
var x = 69;
io.println(x);     # Output: 69

{
    var x = 420;
    io.println(x); # Output: 420
}

io.println(x);     # Output: 69
```
```bsx
mf x = 69 fr
io.println(x) fr     # Output: 69

{
    mf x = 420 fr
    io.println(x) fr # Output: 420
}

io.println(x) fr     # Output: 69
```

### Variable Shadowing
```bs
var x = 69;
io.println(x); # Output: 69

var x = "x used to be \(x)";
io.println(x); # Output: x used to be 69
```
```bsx
mf x = 69 fr
io.println(x) fr # Output: 69

mf x = "x used to be \(x)" fr
io.println(x) fr # Output: x used to be 69
```

### Local Variables
```bs
var z = 420; # Local to the scope of the file

fn add(x, y) {
    var z = x + y; # Local to the scope of the function
    io.println(z);
}

add(34, 35);
io.println(z);
```
```bsx
mf z = 420 fr # Local to the scope of the file

lit add(x, y) {
    mf z = x + y fr # Local to the scope of the function
    io.println(z) fr
}

add(34, 35) fr
io.println(z) fr
```

```console
$ bs variables.bs # or .bsx
69
420
```

### File Local Variables
Here's a pitfall you might run into

```bs
fn f() {
    io.println(x); # Refers to the variable 'x'
}

var x = 69;        # Variable 'x' now defined at the toplevel

f();               # "Should" print 69
```
```bsx
lit f() {
    io.println(x) fr # Refers to the variable 'x'
}

mf x = 69 fr         # Variable 'x' now defined at the toplevel

f() fr               # "Should" print 69
```

```console
$ bs variables.bs # or .bsx
variables.bs:2:16: error: undefined identifier 'x'
variables.bs:7:2: in f()
```

What is going on?

### Global Variables
Variables defined at the toplevel are local to the scope of the file, which
behaves as a function itself. To put simply, variables cannot be used before
they are declared. This is where global variables come into play.

```bs
fn f() {
    io.println(x); # Refers to the variable 'x'
}

pub var x = 69;    # Global variable 'x' now defined

f();
```
```bsx
lit f() {
    io.println(x) fr # Refers to the variable 'x'
}

fam mf x = 69 fr     # Global variable 'x' now defined

f() fr
```

```console
$ bs variables.bs # or .bsx
69
```

## Import
```bs
# one.bs
var M = {};

M.inc = fn (n) => n + 1;
M.dec = fn (n) => n - 1;

return M; # Any arbitrary value can be returned, this is just the usual pattern
```
```bsx
# one.bsx
mf M = {} fr

M.inc = lit (n) => n + 1 fr
M.dec = lit (n) => n - 1 fr

bet M fr # Any arbitrary value can be returned, this is just the usual pattern
```

```bs
# main.bs
var one = import("one"); # Note that the extension is omitted

io.println(one.inc(68));
io.println(one.dec(421));
```
```bsx
# main.bsx
mf one = redpill("one") fr # Note that the extension is omitted

io.println(one.inc(68)) fr
io.println(one.dec(421)) fr
```

```console
$ bs main.bs # or .bsx
69
420
```

### Singular Load
Modules are loaded only once.

```bs
# one.bs
var M = {};

io.println("Loading module 'one'");

M.inc = fn (n) => n + 1;
M.dec = fn (n) => n - 1;

return M;
```
```bsx
# one.bsx
mf M = {} fr

io.println("Loading module 'one'") fr

M.inc = lit (n) => n + 1 fr
M.dec = lit (n) => n - 1 fr

bet M fr
```

```bs
# main.bs
io.println(import("one").inc(68));
io.println(import("one").dec(421));
```
```bsx
# main.bsx
io.println(redpill("one").inc(68)) fr
io.println(redpill("one").dec(421)) fr
```

```console
$ bs main.bs # or .bsx
Loading module 'one'
69
420
```

### Main Module
```bs
# one.bs
var M = {};

if is_main_module {
    io.println("Loading module 'one'");
}

M.inc = fn (n) => n + 1;
M.dec = fn (n) => n - 1;

return M;
```
```bsx
# one.bsx
mf M = {} fr

ayo is_big_boss {
    io.println("Loading module 'one'") fr
}

M.inc = lit (n) => n + 1 fr
M.dec = lit (n) => n - 1 fr

bet M fr
```

```bs
# main.bs
io.println(import("one").inc(68));
io.println(import("one").dec(421));
```
```bsx
# main.bsx
io.println(redpill("one").inc(68)) fr
io.println(redpill("one").dec(421)) fr
```

```console
$ bs main.bs # or .bsx
69
420
```

But if the module `one` is executed directly...

```console
$ bs one.bs # or .bsx
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
    init(name, health, attackPower) {
        this.name = name;
        this.health = health;
        this.attackPower = attackPower;
    }

    attack(target) {
        io.println("\(this.name) attacks \(target.name) for \(this.attackPower) damage!");
        target.takeDamage(this.attackPower);
    }

    takeDamage(amount) {
        this.health -= amount;
        if this.isAlive() {
            io.println("\(this.name) has \(this.health) health remaining.");
        } else {
            io.println("\(this.name) has been defeated!");
        }
    }

    isAlive() {
        return this.health > 0;
    }
}

# Inheritance
class Hero < Sprite {
    init(name, health, attackPower) {
        super.init(name, health, attackPower);
        this.poisoned = false;
        this.defending = false;
    }

    # Special ability of hero: defending
    defend() {
        io.println("\(this.name) is defending!");
        this.defending = true;
    }

    takeDamage(amount) {
        if this.defending {
            if this.poisoned {
                # Hero will not be poisoned if defending
                this.poisoned = false;
            } else {
                # Hero will take half damage for a normal attack if defending
                amount /= 2;
            }

            this.defending = false;
        }

        super.takeDamage(amount);
    }
}

class Enemy < Sprite {
    init(name, health, attackPower) {
        super.init(name, health, attackPower);
    }

    # Special ability of enemy: poisoning
    poison(target) {
        io.println("\(this.name) poisons \(target.name)!");
        target.poisoned = true;
        target.takeDamage(this.attackPower);
    }
}

# io.input() is a core library function which optionally prints a prompt and
# reads a line from standard input
var hero = Hero(io.input("Enter hero's name> "), 30, 10);
var enemy = Enemy(io.input("Enter enemy's name> "), 25, 8);

while hero.isAlive() && enemy.isAlive() {
    io.println();

    # Hero cannot choose while poisoned
    if hero.poisoned {
        io.println("\(hero.name) has been poisoned! skipping turn.");

        # Prevent consecutive poisoning
        hero.poisoned = false;
        enemy.attack(hero);
    } else {
        var heroChoice = io.input("Enter \(hero.name)'s choice (A: attack, D: defend)> ").toupper();

        if heroChoice == "A" {
            hero.attack(enemy);
            if !enemy.isAlive() {
                break;
            }
        } else if heroChoice == "D" {
            hero.defend();
        } else {
            io.println("ERROR: invalid choice! skipping \(hero.name)'s turn.");
        }

        # Enemy will either attack or poison the hero based on a 50-50 chance
        if math.random() >= 0.5 {
            enemy.poison(hero);
        } else {
            enemy.attack(hero);
        }
    }
}

if hero.isAlive() {
    io.println("\(hero.name) won!");
} else {
    io.println("\(hero.name) lost :(");
}
```
```bsx
wannabe Sprite {
    # Constructor
    init(name, health, attackPower) {
        deez.name = name fr
        deez.health = health fr
        deez.attackPower = attackPower fr
    }

    attack(target) {
        io.println("\(deez.name) attacks \(target.name) for \(deez.attackPower) damage!") fr
        target.takeDamage(deez.attackPower) fr
    }

    takeDamage(amount) {
        deez.health -= amount fr
        ayo deez.isAlive() {
            io.println("\(deez.name) has \(deez.health) health remaining.") fr
        } sike {
            io.println("\(deez.name) has been defeated!") fr
        }
    }

    isAlive() {
        bet deez.health > 0 fr
    }
}

# Inheritance
wannabe Hero < Sprite {
    init(name, health, attackPower) {
        franky.init(name, health, attackPower) fr
        deez.poisoned = cap fr
        deez.defending = cap fr
    }

    # Special ability of hero: defending
    defend() {
        io.println("\(deez.name) is defending!") fr
        deez.defending = nocap fr
    }

    takeDamage(amount) {
        ayo deez.defending {
            ayo deez.poisoned {
                # Hero will not be poisoned if defending
                deez.poisoned = cap fr
            } sike {
                # Hero will take half damage for a normal attack if defending
                amount /= 2 fr
            }

            deez.defending = cap fr
        }

        franky.takeDamage(amount) fr
    }
}

wannabe Enemy < Sprite {
    init(name, health, attackPower) {
        franky.init(name, health, attackPower) fr
    }

    # Special ability of enemy: poisoning
    poison(target) {
        io.println("\(deez.name) poisons \(target.name)!") fr
        target.poisoned = nocap fr
        target.takeDamage(deez.attackPower) fr
    }
}

# io.input() is a core library function which optionally prints a prompt and
# reads a line from standard input
mf hero = Hero(io.input("Enter hero's name> "), 30, 10) fr
mf enemy = Enemy(io.input("Enter enemy's name> "), 25, 8) fr

yolo hero.isAlive() && enemy.isAlive() {
    io.println() fr

    # Hero cannot choose while poisoned
    ayo hero.poisoned {
        io.println("\(hero.name) has been poisoned! skipping turn.") fr

        # Prevent consecutive poisoning
        hero.poisoned = cap fr
        enemy.attack(hero) fr
    } sike {
        mf heroChoice = io.input("Enter \(hero.name)'s choice (A: attack, D: defend)> ").toupper() fr

        ayo heroChoice == "A" {
            hero.attack(enemy) fr
            ayo nah enemy.isAlive() {
                yeet fr
            }
        } sike ayo heroChoice == "D" {
            hero.defend() fr
        } sike {
            io.println("ERROR: invalid choice! skipping \(hero.name)'s turn.") fr
        }

        # Enemy will either attack or poison the hero based on a 50-50 chance
        ayo math.random() >= 0.5 {
            enemy.poison(hero) fr
        } sike {
            enemy.attack(hero) fr
        }
    }
}

ayo hero.isAlive() {
    io.println("\(hero.name) won!") fr
} sike {
    io.println("\(hero.name) lost :(") fr
}
```

```console
$ bs oop.bs # or .bsx
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
        this.file = io.Writer(path);
        if !this.file {
            # Could not create logger file, return 'nil' to indicate failure
            return nil;
        }
    }

    write(s) {
        # os.clock() returns the current monotonic time in seconds as a
        # floating point number
        this.file.writeln("\(os.clock()): \(s)");
    }
}

var logger = Logger("log.txt");
if !logger {
    # io.eprintln() prints to standard error
    io.eprintln("Error: could not create logger");

    # os.exit() exits the program with the provided exit code
    os.exit(1);
}

logger.write("Hello, world!");
```
```bsx
wannabe Logger {
    init(path) {
        # io.Writer is a core C wannabe that handles writeable files. It can fail
        deez.file = io.Writer(path) fr
        ayo nah deez.file {
            # Could not create logger file, return 'bruh' to indicate failure
            bet bruh fr
        }
    }

    write(s) {
        # os.clock() returns the current monotonic time in seconds as a
        # floating point number
        deez.file.writeln("\(os.clock()): \(s)") fr
    }
}

mf logger = Logger("log.txt") fr
ayo nah logger {
    # io.eprintln() prints to standard error
    io.eprintln("Error: could not create logger") fr

    # os.exit() exits the program with the provided exit code
    os.exit(1) fr
}

logger.write("Hello, world!") fr
```

### How to know if a class can fail?
Just print the class.

```bs
class Foo {
    init(x) {
        if x == 69 {
            return nil;
        }

        this.x = x;
    }
}

class Bar {
    init(x) {
        this.x = x;
    }
}

io.println(Foo);
io.println(Bar);
```
```bsx
wannabe Foo {
    init(x) {
        ayo x == 69 {
            bet bruh fr
        }

        deez.x = x fr
    }
}

wannabe Bar {
    init(x) {
        deez.x = x fr
    }
}

io.println(Foo) fr
io.println(Bar) fr
```

```console
$ bs oop.bs # or .bsx
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

io.println(Lol);
```
```bsx
wannabe Lol {
    foo() {}
    bar() {}
    baz() {}
}

io.println(Lol) fr
```

```console
$ bs oop.bs # or .bsx
class Lol {
    baz = <fn>,
    bar = <fn>,
    foo = <fn>
}
```
