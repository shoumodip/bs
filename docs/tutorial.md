# Tutorial
Let's start with a simple "Hello World" program.

```
io.println("Hello, world!");
---
io.println("Hello, world!") fr
```

Save this to a file named <code>hello.bs</code> and run it as follows.

```console
$ bs hello.bs
Hello, world!
```

That's not all though. As you can see in the code block above, there seems to
be two "modes" that this language operates in. What's up with that?

## BS and BSX
BS is more "mainstream" in its syntax, whereas BSX is a parody of Gen Z
brainrot. There are subtle differences between the two, even besides syntax
alone, which will be introduced later in appropriate places in the tutorial.

You can choose whether to use BS or BSX based on the extension of your file.
Save the BSX version of the previous example to <code>hello.bsx</code> and run
it as before.

```console
$ bs hello.bsx
Hello, world!
```

## Types and Values
BS supports the types present in all scripting languages: nil, numbers,
booleans, strings, arrays, tables, and functions.

### Nil
```
io.println(nil); # Output: nil
---
io.println(bruh) fr # Output: bruh
```

This is the first difference between BS and BSX. Not only is the keyword
different, but the output itself changes.

### Numbers
BS supports 64-bit floating point numbers only.

```
io.println(69);     # Output: 69
io.println(420.69); # Output: 420.69
io.println(0xff);   # Output: 255
---
io.println(69) fr     # Output: 69
io.println(420.69) fr # Output: 420.69
io.println(0xff) fr   # Output: 255
```

Standard arithmetic as well as bitwise operations are supported.

```
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
---
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
```
io.println(true);  # Output: true
io.println(false); # Output: false
---
io.println(nocap) fr # Output: nocap
io.println(cap) fr   # Output: cap
```

Again, note that the output differs depending on the mode.

```
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

# Logical XOR.
io.println(true ^^ true);   # Output: false
io.println(true ^^ false);  # Output: true
---
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

# Logical XOR.
io.println(nocap ^^ nocap) fr # Output: cap
io.println(nocap ^^ cap) fr   # Output: nocap
```

### Strings
```
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
---
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
```
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
---
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
```
