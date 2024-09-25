:i count 196
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

:b stderr 242
strings/error_invalid_addition.bs:1:15: error: invalid operands to binary (+): number, string

Use (++) for string concatenation, or use string interpolation instead

```
"Hello, " ++ "world!";
"Hello, " ++ 69;
"Hello, \(34 + 35) nice!";
```

:b shell 44
../bin/bs strings/error_invalid_addition.bsx
:i returncode 1
:b stdout 0

:b stderr 243
strings/error_invalid_addition.bsx:1:15: error: invalid operands to binary (+): number, string

Use (++) for string concatenation, or use string interpolation instead

```
"Hello, " ++ "world!";
"Hello, " ++ 69;
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

:b shell 39
../bin/bs arrays/error_invalid_index.bs
:i returncode 1
:b stdout 0

:b stderr 89
arrays/error_invalid_index.bs:2:4: error: expected array index to be number, got boolean

:b shell 40
../bin/bs arrays/error_invalid_index.bsx
:i returncode 1
:b stdout 0

:b stderr 90
arrays/error_invalid_index.bsx:2:4: error: expected array index to be number, got capness

:b shell 46
../bin/bs arrays/error_invalid_index_assign.bs
:i returncode 1
:b stdout 0

:b stderr 95
arrays/error_invalid_index_assign.bs:2:4: error: expected array index to be number, got string

:b shell 47
../bin/bs arrays/error_invalid_index_assign.bsx
:i returncode 1
:b stdout 0

:b stderr 96
arrays/error_invalid_index_assign.bsx:2:4: error: expected array index to be number, got string

:b shell 52
../bin/bs arrays/error_invalid_index_const_assign.bs
:i returncode 1
:b stdout 0

:b stderr 101
arrays/error_invalid_index_const_assign.bs:2:4: error: expected array index to be number, got string

:b shell 53
../bin/bs arrays/error_invalid_index_const_assign.bsx
:i returncode 1
:b stdout 0

:b stderr 102
arrays/error_invalid_index_const_assign.bsx:2:4: error: expected array index to be number, got string

:b shell 24
../bin/bs tables/main.bs
:i returncode 0
:b stdout 22
69
420
1337
3
bar
foo

:b stderr 0

:b shell 25
../bin/bs tables/main.bsx
:i returncode 0
:b stdout 22
69
420
1337
3
bar
foo

:b stderr 0

:b shell 37
../bin/bs tables/error_invalid_key.bs
:i returncode 1
:b stdout 0

:b stderr 70
tables/error_invalid_key.bs:2:4: error: cannot use 'nil' as table key

:b shell 38
../bin/bs tables/error_invalid_key.bsx
:i returncode 1
:b stdout 0

:b stderr 72
tables/error_invalid_key.bsx:2:4: error: cannot use 'bruh' as table key

:b shell 44
../bin/bs tables/error_invalid_key_assign.bs
:i returncode 1
:b stdout 0

:b stderr 77
tables/error_invalid_key_assign.bs:2:4: error: cannot use 'nil' as table key

:b shell 45
../bin/bs tables/error_invalid_key_assign.bsx
:i returncode 1
:b stdout 0

:b stderr 79
tables/error_invalid_key_assign.bsx:2:4: error: cannot use 'bruh' as table key

:b shell 47
../bin/bs containers/error_invalid_container.bs
:i returncode 1
:b stdout 0

:b stderr 82
containers/error_invalid_container.bs:2:2: error: cannot invoke or index into nil

:b shell 48
../bin/bs containers/error_invalid_container.bsx
:i returncode 1
:b stdout 0

:b stderr 84
containers/error_invalid_container.bsx:2:2: error: cannot invoke or index into bruh

:b shell 53
../bin/bs containers/error_invalid_container_const.bs
:i returncode 1
:b stdout 0

:b stderr 88
containers/error_invalid_container_const.bs:2:2: error: cannot invoke or index into nil

:b shell 54
../bin/bs containers/error_invalid_container_const.bsx
:i returncode 1
:b stdout 0

:b stderr 90
containers/error_invalid_container_const.bsx:2:2: error: cannot invoke or index into bruh

:b shell 54
../bin/bs containers/error_invalid_container_assign.bs
:i returncode 1
:b stdout 0

:b stderr 95
containers/error_invalid_container_assign.bs:2:2: error: cannot take mutable index into number

:b shell 55
../bin/bs containers/error_invalid_container_assign.bsx
:i returncode 1
:b stdout 0

:b stderr 96
containers/error_invalid_container_assign.bsx:2:2: error: cannot take mutable index into number

:b shell 60
../bin/bs containers/error_invalid_container_const_assign.bs
:i returncode 1
:b stdout 0

:b stderr 101
containers/error_invalid_container_const_assign.bs:2:2: error: cannot take mutable index into number

:b shell 61
../bin/bs containers/error_invalid_container_const_assign.bsx
:i returncode 1
:b stdout 0

:b stderr 102
containers/error_invalid_container_const_assign.bsx:2:2: error: cannot take mutable index into number

:b shell 35
../bin/bs containers/in_operator.bs
:i returncode 0
:b stdout 55
true
true
false
true
false
false
true
false
true
false

:b stderr 0

:b shell 36
../bin/bs containers/in_operator.bsx
:i returncode 0
:b stdout 50
nocap
nocap
cap
nocap
cap
cap
nocap
cap
nocap
cap

:b stderr 0

:b shell 59
../bin/bs containers/error_in_operator_invalid_container.bs
:i returncode 1
:b stdout 0

:b stderr 87
containers/error_in_operator_invalid_container.bs:1:4: error: cannot index into number

:b shell 60
../bin/bs containers/error_in_operator_invalid_container.bsx
:i returncode 1
:b stdout 0

:b stderr 88
containers/error_in_operator_invalid_container.bsx:1:4: error: cannot index into number

:b shell 55
../bin/bs containers/error_in_operator_invalid_index.bs
:i returncode 1
:b stdout 0

:b stderr 88
containers/error_in_operator_invalid_index.bs:1:5: error: cannot use 'nil' as table key

:b shell 56
../bin/bs containers/error_in_operator_invalid_index.bsx
:i returncode 1
:b stdout 0

:b stderr 90
containers/error_in_operator_invalid_index.bsx:1:6: error: cannot use 'bruh' as table key

:b shell 59
../bin/bs containers/error_undefined_c_instance_property.bs
:i returncode 1
:b stdout 0

:b stderr 110
containers/error_undefined_c_instance_property.bs:1:9: error: undefined instance property or method name: foo

:b shell 60
../bin/bs containers/error_undefined_c_instance_property.bsx
:i returncode 1
:b stdout 0

:b stderr 111
containers/error_undefined_c_instance_property.bsx:1:9: error: undefined instance property or method name: foo

:b shell 57
../bin/bs containers/error_undefined_instance_property.bs
:i returncode 1
:b stdout 0

:b stderr 108
containers/error_undefined_instance_property.bs:2:7: error: undefined instance property or method name: foo

:b shell 58
../bin/bs containers/error_undefined_instance_property.bsx
:i returncode 1
:b stdout 0

:b stderr 109
containers/error_undefined_instance_property.bsx:2:7: error: undefined instance property or method name: foo

:b shell 49
../bin/bs containers/error_undefined_table_key.bs
:i returncode 1
:b stdout 0

:b stderr 77
containers/error_undefined_table_key.bs:5:4: error: undefined table key: bar

:b shell 50
../bin/bs containers/error_undefined_table_key.bsx
:i returncode 1
:b stdout 0

:b stderr 78
containers/error_undefined_table_key.bsx:5:4: error: undefined table key: bar

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

:b stderr 72
loops/error_invalid_iterator.bs:1:13: error: cannot iterate over number

:b shell 42
../bin/bs loops/error_invalid_iterator.bsx
:i returncode 1
:b stdout 0

:b stderr 73
loops/error_invalid_iterator.bsx:1:19: error: cannot iterate over number

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
functions/error_native_stack_trace.bs:6:15: in array.map()
functions/error_native_stack_trace.bs:9:5: in main()

:b shell 48
../bin/bs functions/error_native_stack_trace.bsx
:i returncode 1
:b stdout 0

:b stderr 248
[C]: error: expected argument #1 to be string, got number
functions/error_native_stack_trace.bsx:2:21: in str.reverse()
[C]: in foo()
functions/error_native_stack_trace.bsx:6:15: in array.map()
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

:b shell 46
../bin/bs oop/error_cannot_return_from_init.bs
:i returncode 1
:b stdout 0

:b stderr 534
oop/error_cannot_return_from_init.bs:3:16: error: can only explicity return 'nil' from an initializer method

When an initializer method explicitly returns 'nil', it indicates that the
initialization failed due to some reason, and the site of the instantiation
gets 'nil' as the result. This is not strictly OOP, but I missed the part where
that's my problem.

```
var f = io.Reader("does_not_exist.txt");
if !f {
    io.eprintln("Error: could not read file!");
    os.exit(1);
}

io.print(f.read()); # Or whatever you want to do
```

:b shell 47
../bin/bs oop/error_cannot_return_from_init.bsx
:i returncode 1
:b stdout 0

:b stderr 538
oop/error_cannot_return_from_init.bsx:3:13: error: can only explicity return 'bruh' from an initializer method

When an initializer method explicitly returns 'bruh', it indicates that the
initialization failed due to some reason, and the site of the instantiation
gets 'bruh' as the result. This is not strictly OOP, but I missed the part where
that's my problem.

```
var f = io.Reader("does_not_exist.txt");
if !f {
    io.eprintln("Error: could not read file!");
    os.exit(1);
}

io.print(f.read()); # Or whatever you want to do
```

:b shell 41
../bin/bs oop/error_this_outside_class.bs
:i returncode 1
:b stdout 0

:b stderr 81
oop/error_this_outside_class.bs:1:1: error: cannot use 'this' outside of 'class'

:b shell 42
../bin/bs oop/error_this_outside_class.bsx
:i returncode 1
:b stdout 0

:b stderr 84
oop/error_this_outside_class.bsx:1:1: error: cannot use 'deez' outside of 'wannabe'

:b shell 44
../bin/bs oop/error_with_init_wrong_arity.bs
:i returncode 1
:b stdout 0

:b stderr 74
oop/error_with_init_wrong_arity.bs:5:4: error: expected 1 argument, got 0

:b shell 45
../bin/bs oop/error_with_init_wrong_arity.bsx
:i returncode 1
:b stdout 0

:b stderr 75
oop/error_with_init_wrong_arity.bsx:5:4: error: expected 1 argument, got 0

:b shell 47
../bin/bs oop/error_without_init_wrong_arity.bs
:i returncode 1
:b stdout 0

:b stderr 78
oop/error_without_init_wrong_arity.bs:3:4: error: expected 0 arguments, got 1

:b shell 48
../bin/bs oop/error_without_init_wrong_arity.bsx
:i returncode 1
:b stdout 0

:b stderr 79
oop/error_without_init_wrong_arity.bsx:3:4: error: expected 0 arguments, got 1

:b shell 34
../bin/bs oop/error_inside_init.bs
:i returncode 1
:b stdout 0

:b stderr 110
oop/error_inside_init.bs:3:9: error: invalid operand to unary (-): nil
oop/error_inside_init.bs:7:4: in Foo()

:b shell 35
../bin/bs oop/error_inside_init.bsx
:i returncode 1
:b stdout 0

:b stderr 113
oop/error_inside_init.bsx:3:9: error: invalid operand to unary (-): bruh
oop/error_inside_init.bsx:7:4: in Foo()

:b shell 41
../bin/bs oop/error_inside_method_call.bs
:i returncode 1
:b stdout 0

:b stderr 129
oop/error_inside_method_call.bs:3:9: error: invalid operand to unary (-): nil
oop/error_inside_method_call.bs:7:10: in Foo.foo()

:b shell 42
../bin/bs oop/error_inside_method_call.bsx
:i returncode 1
:b stdout 0

:b stderr 132
oop/error_inside_method_call.bsx:3:9: error: invalid operand to unary (-): bruh
oop/error_inside_method_call.bsx:7:10: in Foo.foo()

:b shell 41
../bin/bs oop/return_without_expr_init.bs
:i returncode 0
:b stdout 19
Foo {
    a = 69
}

:b stderr 0

:b shell 42
../bin/bs oop/return_without_expr_init.bsx
:i returncode 0
:b stdout 19
Foo {
    a = 69
}

:b stderr 0

:b shell 26
../bin/bs oop/with_init.bs
:i returncode 0
:b stdout 17
Hello, John Doe!

:b stderr 0

:b shell 27
../bin/bs oop/with_init.bsx
:i returncode 0
:b stdout 17
Hello, John Doe!

:b stderr 0

:b shell 29
../bin/bs oop/without_init.bs
:i returncode 0
:b stdout 3
69

:b stderr 0

:b shell 30
../bin/bs oop/without_init.bsx
:i returncode 0
:b stdout 3
69

:b stderr 0

:b shell 28
../bin/bs oop/inheritance.bs
:i returncode 0
:b stdout 613
[Joe] Created new bank account with balance 69
[Joe] Deposited 420. New balance: 489
[Joe] Withdrew 69. New balance: 420
[Bob] Created new bank account with balance 420
[Bob] Switched to savings account with interest rate 0.07
[Bob] Deposited 69. New balance: 489
[Bob] Applied 7% interest. New balance: 523.23
[Bob] Withdrew 420. New balance: 103.23
[Alice] Created new bank account with balance 420
[Alice] Switched to checking account with overdraft limit 420
[Alice] Deposited 69. New balance: 489
[Alice] Withdrew 420. New balance: 69
[Alice] Withdrew 420. New balance: -351
[Alice] Overdraft limit exceeded

:b stderr 0

:b shell 29
../bin/bs oop/inheritance.bsx
:i returncode 0
:b stdout 613
[Joe] Created new bank account with balance 69
[Joe] Deposited 420. New balance: 489
[Joe] Withdrew 69. New balance: 420
[Bob] Created new bank account with balance 420
[Bob] Switched to savings account with interest rate 0.07
[Bob] Deposited 69. New balance: 489
[Bob] Applied 7% interest. New balance: 523.23
[Bob] Withdrew 420. New balance: 103.23
[Alice] Created new bank account with balance 420
[Alice] Switched to checking account with overdraft limit 420
[Alice] Deposited 69. New balance: 489
[Alice] Withdrew 420. New balance: 69
[Alice] Withdrew 420. New balance: -351
[Alice] Overdraft limit exceeded

:b stderr 0

:b shell 45
../bin/bs oop/error_undefined_super_method.bs
:i returncode 1
:b stdout 0

:b stderr 125
oop/error_undefined_super_method.bs:4:15: error: undefined super method: foo
oop/error_undefined_super_method.bs:8:2: in A()

:b shell 46
../bin/bs oop/error_undefined_super_method.bsx
:i returncode 1
:b stdout 0

:b stderr 128
oop/error_undefined_super_method.bsx:4:16: error: undefined franky method: foo
oop/error_undefined_super_method.bsx:8:2: in A()

:b shell 29
../bin/bs invokation/chain.bs
:i returncode 0
:b stdout 14
69
420
69
420

:b stderr 0

:b shell 30
../bin/bs invokation/chain.bsx
:i returncode 0
:b stdout 14
69
420
69
420

:b stderr 0

:b shell 52
../bin/bs invokation/error_call_invalid_container.bs
:i returncode 1
:b stdout 0

:b stderr 87
invokation/error_call_invalid_container.bs:2:2: error: cannot invoke or index into nil

:b shell 53
../bin/bs invokation/error_call_invalid_container.bsx
:i returncode 1
:b stdout 0

:b stderr 89
invokation/error_call_invalid_container.bsx:2:2: error: cannot invoke or index into bruh

:b shell 46
../bin/bs invokation/error_call_invalid_key.bs
:i returncode 1
:b stdout 0

:b stderr 92
invokation/error_call_invalid_key.bs:2:4: error: expected array index to be number, got nil

:b shell 47
../bin/bs invokation/error_call_invalid_key.bsx
:i returncode 1
:b stdout 0

:b stderr 94
invokation/error_call_invalid_key.bsx:2:4: error: expected array index to be number, got bruh

:b shell 42
../bin/bs invokation/error_invoked_body.bs
:i returncode 1
:b stdout 0

:b stderr 145
invokation/error_invoked_body.bs:3:13: error: invalid operands to binary (+): nil, number
invokation/error_invoked_body.bs:7:4: in <anonymous>()

:b shell 43
../bin/bs invokation/error_invoked_body.bsx
:i returncode 1
:b stdout 0

:b stderr 148
invokation/error_invoked_body.bsx:3:14: error: invalid operands to binary (+): bruh, number
invokation/error_invoked_body.bsx:7:4: in <anonymous>()

:b shell 56
../bin/bs invokation/error_key_call_argument_location.bs
:i returncode 1
:b stdout 0

:b stderr 124
[C]: error: expected argument #1 to be string, got nil
invokation/error_key_call_argument_location.bs:5:5: in str.reverse()

:b shell 57
../bin/bs invokation/error_key_call_argument_location.bsx
:i returncode 1
:b stdout 0

:b stderr 126
[C]: error: expected argument #1 to be string, got bruh
invokation/error_key_call_argument_location.bsx:5:5: in str.reverse()

:b shell 55
../bin/bs invokation/error_key_call_invalid_function.bs
:i returncode 1
:b stdout 0

:b stderr 77
invokation/error_key_call_invalid_function.bs:5:4: error: cannot call number

:b shell 56
../bin/bs invokation/error_key_call_invalid_function.bsx
:i returncode 1
:b stdout 0

:b stderr 78
invokation/error_key_call_invalid_function.bsx:5:4: error: cannot call number

:b shell 50
../bin/bs invokation/error_key_call_wrong_arity.bs
:i returncode 1
:b stdout 0

:b stderr 81
invokation/error_key_call_wrong_arity.bs:7:4: error: expected 0 arguments, got 1

:b shell 51
../bin/bs invokation/error_key_call_wrong_arity.bsx
:i returncode 1
:b stdout 0

:b stderr 82
invokation/error_key_call_wrong_arity.bsx:7:4: error: expected 0 arguments, got 1

:b shell 53
../bin/bs invokation/error_method_call_wrong_arity.bs
:i returncode 1
:b stdout 0

:b stderr 84
invokation/error_method_call_wrong_arity.bs:8:6: error: expected 0 arguments, got 1

:b shell 54
../bin/bs invokation/error_method_call_wrong_arity.bsx
:i returncode 1
:b stdout 0

:b stderr 85
invokation/error_method_call_wrong_arity.bsx:8:6: error: expected 0 arguments, got 1

:b shell 59
../bin/bs invokation/error_native_call_argument_location.bs
:i returncode 1
:b stdout 0

:b stderr 128
[C]: error: expected argument #1 to be string, got nil
invokation/error_native_call_argument_location.bs:1:24: in str.reverse()

:b shell 60
../bin/bs invokation/error_native_call_argument_location.bsx
:i returncode 1
:b stdout 0

:b stderr 130
[C]: error: expected argument #1 to be string, got bruh
invokation/error_native_call_argument_location.bsx:1:24: in str.reverse()

:b shell 53
../bin/bs invokation/error_native_call_wrong_arity.bs
:i returncode 1
:b stdout 0

:b stderr 106
[C]: error: expected 1 argument, got 2
invokation/error_native_call_wrong_arity.bs:1:23: in str.reverse()

:b shell 54
../bin/bs invokation/error_native_call_wrong_arity.bsx
:i returncode 1
:b stdout 0

:b stderr 107
[C]: error: expected 1 argument, got 2
invokation/error_native_call_wrong_arity.bsx:1:23: in str.reverse()

:b shell 61
../bin/bs invokation/error_property_call_argument_location.bs
:i returncode 1
:b stdout 0

:b stderr 129
[C]: error: expected argument #1 to be string, got nil
invokation/error_property_call_argument_location.bs:5:7: in str.reverse()

:b shell 62
../bin/bs invokation/error_property_call_argument_location.bsx
:i returncode 1
:b stdout 0

:b stderr 131
[C]: error: expected argument #1 to be string, got bruh
invokation/error_property_call_argument_location.bsx:5:7: in str.reverse()

:b shell 60
../bin/bs invokation/error_property_call_invalid_function.bs
:i returncode 1
:b stdout 0

:b stderr 82
invokation/error_property_call_invalid_function.bs:5:6: error: cannot call number

:b shell 61
../bin/bs invokation/error_property_call_invalid_function.bsx
:i returncode 1
:b stdout 0

:b stderr 83
invokation/error_property_call_invalid_function.bsx:5:6: error: cannot call number

:b shell 55
../bin/bs invokation/error_property_call_wrong_arity.bs
:i returncode 1
:b stdout 0

:b stderr 86
invokation/error_property_call_wrong_arity.bs:8:6: error: expected 0 arguments, got 1

:b shell 56
../bin/bs invokation/error_property_call_wrong_arity.bsx
:i returncode 1
:b stdout 0

:b stderr 87
invokation/error_property_call_wrong_arity.bsx:8:6: error: expected 0 arguments, got 1

:b shell 32
../bin/bs invokation/key_call.bs
:i returncode 0
:b stdout 14
Hello, world!

:b stderr 0

:b shell 33
../bin/bs invokation/key_call.bsx
:i returncode 0
:b stdout 14
Hello, world!

:b stderr 0

:b shell 35
../bin/bs invokation/method_call.bs
:i returncode 0
:b stdout 14
Hello, world!

:b stderr 0

:b shell 36
../bin/bs invokation/method_call.bsx
:i returncode 0
:b stdout 14
Hello, world!

:b stderr 0

:b shell 35
../bin/bs invokation/native_call.bs
:i returncode 0
:b stdout 7
!olleH

:b stderr 0

:b shell 36
../bin/bs invokation/native_call.bsx
:i returncode 0
:b stdout 7
!olleH

:b stderr 0

:b shell 37
../bin/bs invokation/property_call.bs
:i returncode 0
:b stdout 14
Hello, world!

:b stderr 0

:b shell 38
../bin/bs invokation/property_call.bsx
:i returncode 0
:b stdout 14
Hello, world!

:b stderr 0

:b shell 45
../bin/bs delete/error_cannot_delete_super.bs
:i returncode 1
:b stdout 0

:b stderr 78
delete/error_cannot_delete_super.bs:4:16: error: cannot use 'delete' on super

:b shell 46
../bin/bs delete/error_cannot_delete_super.bsx
:i returncode 1
:b stdout 0

:b stderr 79
delete/error_cannot_delete_super.bsx:4:15: error: cannot use 'ghost' on franky

:b shell 51
../bin/bs delete/error_expected_index_expression.bs
:i returncode 1
:b stdout 0

:b stderr 199
delete/error_expected_index_expression.bs:2:8: error: expected index expression

Index expression can be any of the following

```
xs.foo;    # Constant index
xs["bar"]; # Expression based index
```

:b shell 52
../bin/bs delete/error_expected_index_expression.bsx
:i returncode 1
:b stdout 0

:b stderr 200
delete/error_expected_index_expression.bsx:2:7: error: expected index expression

Index expression can be any of the following

```
xs.foo;    # Constant index
xs["bar"]; # Expression based index
```

:b shell 43
../bin/bs delete/error_invalid_container.bs
:i returncode 1
:b stdout 0

:b stderr 72
delete/error_invalid_container.bs:2:9: error: cannot delete from number

:b shell 44
../bin/bs delete/error_invalid_container.bsx
:i returncode 1
:b stdout 0

:b stderr 73
delete/error_invalid_container.bsx:2:8: error: cannot delete from number

:b shell 51
../bin/bs delete/error_invalid_instance_property.bs
:i returncode 1
:b stdout 0

:b stderr 93
delete/error_invalid_instance_property.bs:2:13: error: cannot use 'nil' as instance property

:b shell 52
../bin/bs delete/error_invalid_instance_property.bsx
:i returncode 1
:b stdout 0

:b stderr 95
delete/error_invalid_instance_property.bsx:2:12: error: cannot use 'bruh' as instance property

:b shell 43
../bin/bs delete/error_invalid_table_key.bs
:i returncode 1
:b stdout 0

:b stderr 77
delete/error_invalid_table_key.bs:2:10: error: cannot use 'nil' as table key

:b shell 44
../bin/bs delete/error_invalid_table_key.bsx
:i returncode 1
:b stdout 0

:b stderr 78
delete/error_invalid_table_key.bsx:2:9: error: cannot use 'bruh' as table key

:b shell 37
../bin/bs delete/instance_property.bs
:i returncode 0
:b stdout 108
Foo {
    x = 69,
    y = 420
}
true
Foo {
    y = 420
}
false
Foo {
    y = 420
}
true
Foo {}
false
Foo {}

:b stderr 0

:b shell 38
../bin/bs delete/instance_property.bsx
:i returncode 0
:b stdout 106
Foo {
    x = 69,
    y = 420
}
nocap
Foo {
    y = 420
}
cap
Foo {
    y = 420
}
nocap
Foo {}
cap
Foo {}

:b stderr 0

:b shell 25
../bin/bs delete/table.bs
:i returncode 0
:b stdout 96
{
    bar = 420,
    foo = 69
}
true
{
    bar = 420
}
false
{
    bar = 420
}
true
{}
false
{}

:b stderr 0

:b shell 26
../bin/bs delete/table.bsx
:i returncode 0
:b stdout 94
{
    bar = 420,
    foo = 69
}
nocap
{
    bar = 420
}
cap
{
    bar = 420
}
nocap
{}
cap
{}

:b stderr 0

:b shell 34
../bin/bs builtin_methods/array.bs
:i returncode 0
:b stdout 10
[2, 4, 6]

:b stderr 0

:b shell 35
../bin/bs builtin_methods/array.bsx
:i returncode 0
:b stdout 10
[2, 4, 6]

:b stderr 0

:b shell 35
../bin/bs builtin_methods/number.bs
:i returncode 0
:b stdout 21
1.22464679914735e-16

:b stderr 0

:b shell 36
../bin/bs builtin_methods/number.bsx
:i returncode 0
:b stdout 21
1.22464679914735e-16

:b stderr 0

:b shell 35
../bin/bs builtin_methods/string.bs
:i returncode 0
:b stdout 6
HELLO

:b stderr 0

:b shell 36
../bin/bs builtin_methods/string.bsx
:i returncode 0
:b stdout 6
HELLO

:b stderr 0

:b shell 34
../bin/bs builtin_methods/table.bs
:i returncode 0
:b stdout 149
xs = {
    bar = 420,
    foo = 69
}
ys = {
    bar = 420,
    foo = 69
}
xs = {
    bar = 1337,
    foo = 69
}
ys = {
    bar = 420,
    foo = 69
}

:b stderr 0

:b shell 35
../bin/bs builtin_methods/table.bsx
:i returncode 0
:b stdout 149
xs = {
    bar = 420,
    foo = 69
}
ys = {
    bar = 420,
    foo = 69
}
xs = {
    bar = 1337,
    foo = 69
}
ys = {
    bar = 420,
    foo = 69
}

:b stderr 0

:b shell 50
../bin/bs builtin_methods/error_undefined_array.bs
:i returncode 1
:b stdout 0

:b stderr 75
builtin_methods/error_undefined_array.bs:1:4: error: undefined method: foo

:b shell 51
../bin/bs builtin_methods/error_undefined_array.bsx
:i returncode 1
:b stdout 0

:b stderr 76
builtin_methods/error_undefined_array.bsx:1:4: error: undefined method: foo

:b shell 51
../bin/bs builtin_methods/error_undefined_number.bs
:i returncode 1
:b stdout 0

:b stderr 76
builtin_methods/error_undefined_number.bs:1:4: error: undefined method: foo

:b shell 52
../bin/bs builtin_methods/error_undefined_number.bsx
:i returncode 1
:b stdout 0

:b stderr 77
builtin_methods/error_undefined_number.bsx:1:3: error: undefined method: foo

:b shell 48
../bin/bs builtin_methods/error_undefined_str.bs
:i returncode 1
:b stdout 0

:b stderr 73
builtin_methods/error_undefined_str.bs:1:4: error: undefined method: foo

:b shell 49
../bin/bs builtin_methods/error_undefined_str.bsx
:i returncode 1
:b stdout 0

:b stderr 74
builtin_methods/error_undefined_str.bsx:1:4: error: undefined method: foo

