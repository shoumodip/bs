:i count 60
:b shell 29
../bin/bs arithmetics/main.bs
:i returncode 0
:b stdout 34
69
420
69
420
-1337
69
420
69
420

:b stderr 0

:b shell 30
../bin/bs arithmetics/main.bsx
:i returncode 0
:b stdout 34
69
420
69
420
-1337
69
420
69
420

:b stderr 0

:b shell 29
../bin/bs comparisons/main.bs
:i returncode 0
:b stdout 183
false
true
true
false
false
false
false
true
false
true
true
true
false
false
true
true
false
false
true
true
false
true
false
false
false
true
false
false
false
true
true
true
false

:b stderr 0

:b shell 30
../bin/bs comparisons/main.bsx
:i returncode 0
:b stdout 162
cap
nocap
nocap
cap
cap
cap
cap
nocap
cap
nocap
nocap
nocap
cap
cap
nocap
nocap
cap
cap
nocap
nocap
cap
nocap
cap
cap
cap
nocap
cap
cap
cap
nocap
nocap
nocap
cap

:b stderr 0

:b shell 28
../bin/bs conditions/main.bs
:i returncode 0
:b stdout 10
1
1
2
2
3

:b stderr 0

:b shell 29
../bin/bs conditions/main.bsx
:i returncode 0
:b stdout 10
1
1
2
2
3

:b stderr 0

:b shell 25
../bin/bs strings/main.bs
:i returncode 0
:b stdout 116
Deez nuts
Joe Mama
69 Nice!
The truth is 420
Nested interpolation? Are you crazy?
69 and 420. Hehe
true
false
false

:b stderr 0

:b shell 26
../bin/bs strings/main.bsx
:i returncode 0
:b stdout 113
Deez nuts
Joe Mama
69 Nice!
The truth is 420
Nested interpolation? Are you crazy?
69 and 420. Hehe
nocap
cap
cap

:b stderr 0

:b shell 43
../bin/bs strings/error_invalid_addition.bs
:i returncode 1
:b stdout 0

:b stderr 238
strings/error_invalid_addition.bs:1:15: error: invalid operands to binary (+): number, string

Use (..) for string concatenation, or use string interpolation instead

```
"Hello, ".."world!";
"Hello, "..69;
"Hello, \(34 + 35) nice!";
```

:b shell 44
../bin/bs strings/error_invalid_addition.bsx
:i returncode 1
:b stdout 0

:b stderr 239
strings/error_invalid_addition.bsx:1:15: error: invalid operands to binary (+): number, string

Use (..) for string concatenation, or use string interpolation instead

```
"Hello, ".."world!";
"Hello, "..69;
"Hello, \(34 + 35) nice!";
```

:b shell 28
../bin/bs variables/local.bs
:i returncode 0
:b stdout 14
69
420
69
420

:b stderr 0

:b shell 29
../bin/bs variables/local.bsx
:i returncode 0
:b stdout 14
69
420
69
420

:b stderr 0

:b shell 33
../bin/bs variables/global_use.bs
:i returncode 0
:b stdout 7
69
420

:b stderr 0

:b shell 34
../bin/bs variables/global_use.bsx
:i returncode 0
:b stdout 7
69
420

:b stderr 0

:b shell 38
../bin/bs variables/error_undefined.bs
:i returncode 1
:b stdout 0

:b stderr 68
variables/error_undefined.bs:1:1: error: undefined identifier 'foo'

:b shell 39
../bin/bs variables/error_undefined.bsx
:i returncode 1
:b stdout 0

:b stderr 69
variables/error_undefined.bsx:1:1: error: undefined identifier 'foo'

:b shell 48
../bin/bs variables/error_global_redefinition.bs
:i returncode 1
:b stdout 0

:b stderr 91
variables/error_global_redefinition.bs:2:9: error: redefinition of global identifier 'foo'

:b shell 49
../bin/bs variables/error_global_redefinition.bsx
:i returncode 1
:b stdout 0

:b stderr 92
variables/error_global_redefinition.bsx:2:8: error: redefinition of global identifier 'foo'

:b shell 24
../bin/bs arrays/main.bs
:i returncode 0
:b stdout 33
1
4
9
[1, 4, 9]
3
[69, 420]
1337

:b stderr 0

:b shell 25
../bin/bs arrays/main.bsx
:i returncode 0
:b stdout 33
1
4
9
[1, 4, 9]
3
[69, 420]
1337

:b stderr 0

:b shell 40
../bin/bs arrays/assign_out_of_bounds.bs
:i returncode 0
:b stdout 58
[]
[nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, 69]

:b stderr 0

:b shell 41
../bin/bs arrays/assign_out_of_bounds.bsx
:i returncode 0
:b stdout 68
[]
[bruh, bruh, bruh, bruh, bruh, bruh, bruh, bruh, bruh, bruh, 69]

:b stderr 0

:b shell 45
../bin/bs arrays/error_index_out_of_bounds.bs
:i returncode 1
:b stdout 0

:b stderr 98
arrays/error_index_out_of_bounds.bs:3:14: error: cannot get value at index 0 in array of length 0

:b shell 46
../bin/bs arrays/error_index_out_of_bounds.bsx
:i returncode 1
:b stdout 0

:b stderr 99
arrays/error_index_out_of_bounds.bsx:3:14: error: cannot get value at index 0 in array of length 0

:b shell 24
../bin/bs tables/main.bs
:i returncode 0
:b stdout 32
69
420
1337
3
nil
2
nil
bar
foo

:b stderr 0

:b shell 25
../bin/bs tables/main.bsx
:i returncode 0
:b stdout 34
69
420
1337
3
bruh
2
bruh
bar
foo

:b stderr 0

:b shell 24
../bin/bs import/main.bs
:i returncode 0
:b stdout 21
In common.bs!
69
420

:b stderr 0

:b shell 25
../bin/bs import/main.bsx
:i returncode 0
:b stdout 21
In common.bs!
69
420

:b stderr 0

:b shell 29
../bin/bs import/only_once.bs
:i returncode 0
:b stdout 14
In common.bs!

:b stderr 0

:b shell 30
../bin/bs import/only_once.bsx
:i returncode 0
:b stdout 14
In common.bs!

:b stderr 0

:b shell 40
../bin/bs import/error_could_not_open.bs
:i returncode 1
:b stdout 0

:b stderr 83
import/error_could_not_open.bs:1:1: error: could not read file 'does_not_exist.bs'

:b shell 41
../bin/bs import/error_could_not_open.bsx
:i returncode 1
:b stdout 0

:b stderr 84
import/error_could_not_open.bsx:1:1: error: could not read file 'does_not_exist.bs'

:b shell 41
../bin/bs import/error_expected_string.bs
:i returncode 1
:b stdout 0

:b stderr 90
import/error_expected_string.bs:1:1: error: expected import path to be string, got number

:b shell 42
../bin/bs import/error_expected_string.bsx
:i returncode 1
:b stdout 0

:b stderr 91
import/error_expected_string.bsx:1:1: error: expected import path to be string, got number

:b shell 31
../bin/bs import/main_module.bs
:i returncode 0
:b stdout 16
Directly called

:b stderr 0

:b shell 32
../bin/bs import/main_module.bsx
:i returncode 0
:b stdout 16
Directly called

:b stderr 0

:b shell 40
../bin/bs import/indirect_main_module.bs
:i returncode 0
:b stdout 3
69

:b stderr 0

:b shell 41
../bin/bs import/indirect_main_module.bsx
:i returncode 0
:b stdout 4
420

:b stderr 0

:b shell 24
../bin/bs loops/while.bs
:i returncode 0
:b stdout 20
0
1
2
3
4
5
6
7
8
9

:b stderr 0

:b shell 25
../bin/bs loops/while.bsx
:i returncode 0
:b stdout 20
0
1
2
3
4
5
6
7
8
9

:b stderr 0

:b shell 22
../bin/bs loops/for.bs
:i returncode 0
:b stdout 65
0
1
2
3
4
5
4
3
2
1
0
2
4
5
3
1
0 69
1 420
2 1337
bar 420
foo 69

:b stderr 0

:b shell 23
../bin/bs loops/for.bsx
:i returncode 0
:b stdout 65
0
1
2
3
4
5
4
3
2
1
0
2
4
5
3
1
0 69
1 420
2 1337
bar 420
foo 69

:b stderr 0

:b shell 24
../bin/bs loops/break.bs
:i returncode 0
:b stdout 21
0
1
2
3
4
10
9
8
7
6

:b stderr 0

:b shell 25
../bin/bs loops/break.bsx
:i returncode 0
:b stdout 21
0
1
2
3
4
10
9
8
7
6

:b stderr 0

:b shell 27
../bin/bs loops/continue.bs
:i returncode 0
:b stdout 37
0
1
2
3
4
6
7
8
9
10
9
8
7
5
4
3
2
1

:b stderr 0

:b shell 28
../bin/bs loops/continue.bsx
:i returncode 0
:b stdout 37
0
1
2
3
4
6
7
8
9
10
9
8
7
5
4
3
2
1

:b stderr 0

:b shell 27
../bin/bs functions/main.bs
:i returncode 0
:b stdout 20
Factorial of 5: 120

:b stderr 0

:b shell 28
../bin/bs functions/main.bsx
:i returncode 0
:b stdout 20
Factorial of 5: 120

:b stderr 0

:b shell 29
../bin/bs functions/lambda.bs
:i returncode 0
:b stdout 21
Value of x is 69
420

:b stderr 0

:b shell 30
../bin/bs functions/lambda.bsx
:i returncode 0
:b stdout 21
Value of x is 69
420

:b stderr 0

:b shell 42
../bin/bs functions/error_invalid_arity.bs
:i returncode 1
:b stdout 0

:b stderr 72
functions/error_invalid_arity.bs:3:2: error: expected 1 argument, got 0

:b shell 43
../bin/bs functions/error_invalid_arity.bsx
:i returncode 1
:b stdout 0

:b stderr 73
functions/error_invalid_arity.bsx:3:2: error: expected 1 argument, got 0

:b shell 40
../bin/bs functions/error_stack_trace.bs
:i returncode 1
:b stdout 0

:b stderr 217
functions/error_stack_trace.bs:2:5: error: undefined identifier 'oops'
functions/error_stack_trace.bs:6:8: in fn baz()
functions/error_stack_trace.bs:10:8: in fn bar()
functions/error_stack_trace.bs:13:4: in fn foo()

:b shell 41
../bin/bs functions/error_stack_trace.bsx
:i returncode 1
:b stdout 0

:b stderr 221
functions/error_stack_trace.bsx:2:5: error: undefined identifier 'oops'
functions/error_stack_trace.bsx:6:8: in fn baz()
functions/error_stack_trace.bsx:10:8: in fn bar()
functions/error_stack_trace.bsx:13:4: in fn foo()

:b shell 47
../bin/bs functions/error_native_stack_trace.bs
:i returncode 1
:b stdout 0

:b stderr 251
[C]: error: expected argument #1 to be string, got number
functions/error_native_stack_trace.bs:2:23: in native fn ()
[C]: in fn foo()
functions/error_native_stack_trace.bs:6:14: in native fn ()
functions/error_native_stack_trace.bs:9:5: in fn main()

:b shell 48
../bin/bs functions/error_native_stack_trace.bsx
:i returncode 1
:b stdout 0

:b stderr 254
[C]: error: expected argument #1 to be string, got number
functions/error_native_stack_trace.bsx:2:20: in native fn ()
[C]: in fn foo()
functions/error_native_stack_trace.bsx:6:14: in native fn ()
functions/error_native_stack_trace.bsx:9:5: in fn main()

:b shell 26
../bin/bs closures/main.bs
:i returncode 0
:b stdout 29
69
420
Captured value is 69!

:b stderr 0

:b shell 27
../bin/bs closures/main.bsx
:i returncode 0
:b stdout 29
69
420
Captured value is 69!

:b stderr 0

:b shell 27
../bin/bs closures/loops.bs
:i returncode 0
:b stdout 160
Captured value is 0
Captured value is 1
Captured value is 2
Captured value is 3
Captured value is 4
Captured value is 3
Captured value is 2
Captured value is 1

:b stderr 0

:b shell 28
../bin/bs closures/loops.bsx
:i returncode 0
:b stdout 160
Captured value is 0
Captured value is 1
Captured value is 2
Captured value is 3
Captured value is 4
Captured value is 3
Captured value is 2
Captured value is 1

:b stderr 0

