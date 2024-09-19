:i count 82
:b shell 29
../bin/bs arithmetics/main.bs
:i returncode 0
:b stdout 121
69
420
69
420
1
2
1.59999999999999
0.399999999999997
-1337
69
420
69
420
69
420
69
420
69
420
1337
1772867055
2875837935

:b stderr 0

:b shell 30
../bin/bs arithmetics/main.bsx
:i returncode 0
:b stdout 121
69
420
69
420
1
2
1.59999999999999
0.399999999999997
-1337
69
420
69
420
69
420
69
420
69
420
1337
1772867055
2875837935

:b stderr 0

:b shell 44
../bin/bs arithmetics/error_invalid_digit.bs
:i returncode 1
:b stdout 0

:b stderr 75
arithmetics/error_invalid_digit.bs:1:3: error: invalid digit 'x' in number

:b shell 45
../bin/bs arithmetics/error_invalid_digit.bsx
:i returncode 1
:b stdout 0

:b stderr 76
arithmetics/error_invalid_digit.bsx:1:3: error: invalid digit 'x' in number

:b shell 48
../bin/bs arithmetics/error_invalid_hex_digit.bs
:i returncode 1
:b stdout 0

:b stderr 79
arithmetics/error_invalid_hex_digit.bs:1:5: error: invalid digit 'g' in number

:b shell 49
../bin/bs arithmetics/error_invalid_hex_digit.bsx
:i returncode 1
:b stdout 0

:b stderr 80
arithmetics/error_invalid_hex_digit.bsx:1:5: error: invalid digit 'g' in number

:b shell 29
../bin/bs comparisons/main.bs
:i returncode 0
:b stdout 205
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
false
true
true
false

:b stderr 0

:b shell 30
../bin/bs comparisons/main.bsx
:i returncode 0
:b stdout 182
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
cap
nocap
nocap
cap

:b stderr 0

:b shell 28
../bin/bs conditions/main.bs
:i returncode 0
:b stdout 17
1
1
2
2
3
69
420

:b stderr 0

:b shell 29
../bin/bs conditions/main.bsx
:i returncode 0
:b stdout 17
1
1
2
2
3
69
420

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
:b stdout 104
[]
[
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    69
]

:b stderr 0

:b shell 41
../bin/bs arrays/assign_out_of_bounds.bsx
:i returncode 0
:b stdout 114
[]
[
    bruh,
    bruh,
    bruh,
    bruh,
    bruh,
    bruh,
    bruh,
    bruh,
    bruh,
    bruh,
    69
]

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

:b stderr 84
import/error_could_not_open.bs:1:8: error: could not import module 'does_not_exist'

:b shell 41
../bin/bs import/error_could_not_open.bsx
:i returncode 1
:b stdout 0

:b stderr 85
import/error_could_not_open.bsx:1:9: error: could not import module 'does_not_exist'

:b shell 41
../bin/bs import/error_expected_string.bs
:i returncode 1
:b stdout 0

:b stderr 90
import/error_expected_string.bs:1:8: error: expected module name to be string, got number

:b shell 42
../bin/bs import/error_expected_string.bsx
:i returncode 1
:b stdout 0

:b stderr 91
import/error_expected_string.bsx:1:9: error: expected module name to be string, got number

:b shell 31
../bin/bs import/main_module.bs
:i returncode 0
:b stdout 16
Directly called

:b stderr 0

:b shell 41
../bin/bs import/main_module_extended.bsx
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

:b shell 41
../bin/bs loops/error_invalid_iterator.bs
:i returncode 1
:b stdout 0

:b stderr 78
loops/error_invalid_iterator.bs:1:13: error: cannot iterate over number value

:b shell 42
../bin/bs loops/error_invalid_iterator.bsx
:i returncode 1
:b stdout 0

:b stderr 79
loops/error_invalid_iterator.bsx:1:19: error: cannot iterate over number value

:b shell 44
../bin/bs loops/error_invalid_range_start.bs
:i returncode 1
:b stdout 0

:b stderr 91
loops/error_invalid_range_start.bs:1:10: error: expected range start to be number, got nil

:b shell 45
../bin/bs loops/error_invalid_range_start.bsx
:i returncode 1
:b stdout 0

:b stderr 93
loops/error_invalid_range_start.bsx:1:16: error: expected range start to be number, got bruh

:b shell 42
../bin/bs loops/error_invalid_range_end.bs
:i returncode 1
:b stdout 0

:b stderr 87
loops/error_invalid_range_end.bs:1:13: error: expected range end to be number, got nil

:b shell 43
../bin/bs loops/error_invalid_range_end.bsx
:i returncode 1
:b stdout 0

:b stderr 89
loops/error_invalid_range_end.bsx:1:19: error: expected range end to be number, got bruh

:b shell 43
../bin/bs loops/error_invalid_range_step.bs
:i returncode 1
:b stdout 0

:b stderr 93
loops/error_invalid_range_step.bs:1:17: error: expected range step to be number, got boolean

:b shell 44
../bin/bs loops/error_invalid_range_step.bsx
:i returncode 1
:b stdout 0

:b stderr 94
loops/error_invalid_range_step.bsx:1:23: error: expected range step to be number, got capness

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

:b stderr 208
functions/error_stack_trace.bs:2:5: error: undefined identifier 'oops'
functions/error_stack_trace.bs:6:8: in baz()
functions/error_stack_trace.bs:10:8: in bar()
functions/error_stack_trace.bs:13:4: in foo()

:b shell 41
../bin/bs functions/error_stack_trace.bsx
:i returncode 1
:b stdout 0

:b stderr 212
functions/error_stack_trace.bsx:2:5: error: undefined identifier 'oops'
functions/error_stack_trace.bsx:6:8: in baz()
functions/error_stack_trace.bsx:10:8: in bar()
functions/error_stack_trace.bsx:13:4: in foo()

:b shell 47
../bin/bs functions/error_native_stack_trace.bs
:i returncode 1
:b stdout 0

:b stderr 245
[C]: error: expected argument #1 to be string, got number
functions/error_native_stack_trace.bs:2:24: in str.reverse()
[C]: in foo()
functions/error_native_stack_trace.bs:6:14: in array.map()
functions/error_native_stack_trace.bs:9:5: in main()

:b shell 48
../bin/bs functions/error_native_stack_trace.bsx
:i returncode 1
:b stdout 0

:b stderr 248
[C]: error: expected argument #1 to be string, got number
functions/error_native_stack_trace.bsx:2:21: in str.reverse()
[C]: in foo()
functions/error_native_stack_trace.bsx:6:14: in array.map()
functions/error_native_stack_trace.bsx:9:5: in main()

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

:b shell 26
../bin/bs extended/main.bs
:i returncode 0
:b stdout 38
x = true; y = nil
x = nocap; y = bruh

:b stderr 0

:b shell 27
../bin/bs extended/main.bsx
:i returncode 0
:b stdout 38
x = true; y = nil
x = nocap; y = bruh

:b stderr 0

:b shell 44
../bin/bs extended/error_type_name_normal.bs
:i returncode 1
:b stdout 0

:b stderr 148
[C]: error: expected argument #1 to be string, got nil
extended/common.bs:9:17: in str.reverse()
extended/error_type_name_normal.bs:1:31: in oops()

:b shell 45
../bin/bs extended/error_type_name_normal.bsx
:i returncode 1
:b stdout 0

:b stderr 149
[C]: error: expected argument #1 to be string, got nil
extended/common.bs:9:17: in str.reverse()
extended/error_type_name_normal.bsx:1:32: in oops()

:b shell 46
../bin/bs extended/error_type_name_extended.bs
:i returncode 1
:b stdout 0

:b stderr 164
[C]: error: expected argument #1 to be string, got capness
extended/common_extended.bsx:9:17: in str.reverse()
extended/error_type_name_extended.bs:1:40: in oops()

:b shell 47
../bin/bs extended/error_type_name_extended.bsx
:i returncode 1
:b stdout 0

:b stderr 165
[C]: error: expected argument #1 to be string, got capness
extended/common_extended.bsx:9:17: in str.reverse()
extended/error_type_name_extended.bsx:1:41: in oops()

:b shell 24
../bin/bs typeof/main.bs
:i returncode 0
:b stdout 56
nil
boolean
number
function
string
array
table
function

:b stderr 0

:b shell 25
../bin/bs typeof/main.bsx
:i returncode 0
:b stdout 57
bruh
capness
number
function
string
array
table
function

:b stderr 0

:b shell 33
../bin/bs pretty_printing/main.bs
:i returncode 0
:b stdout 323
{}
ligma
{
    ["1337 bro"] = {
        a = 1,
        c = [1, 2, 3, 4, 5],
        e = [
            [
                [69]
            ]
        ],
        d = [
            1,
            2,
            3,
            4,
            5,
            6
        ],
        [69] = "nice"
    },
    bar = 420,
    foo = 69
}

:b stderr 0

:b shell 34
../bin/bs pretty_printing/main.bsx
:i returncode 0
:b stdout 323
{}
ligma
{
    ["1337 bro"] = {
        a = 1,
        c = [1, 2, 3, 4, 5],
        e = [
            [
                [69]
            ]
        ],
        d = [
            1,
            2,
            3,
            4,
            5,
            6
        ],
        [69] = "nice"
    },
    bar = 420,
    foo = 69
}

:b stderr 0

