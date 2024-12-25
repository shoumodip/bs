:i count 145
:b shell 29
../bin/bs arithmetics/main.bs
:i returncode 0
:b stdout 157
Basic
69
420
69
420
1
2
1.59999999999999
0.399999999999997
-1337
Grouping
69
420
Decimals
69
420
Bitwise
69
420
69
420
69
420
1337
Hex
1772867055
2875837935

:b stderr 0

:b shell 44
../bin/bs arithmetics/error_invalid_digit.bs
:i returncode 1
:b stdout 0

:b stderr 100
arithmetics/error_invalid_digit.bs:1:3: error: invalid digit 'x' in number

    1 | 69x
      |   ^

:b shell 48
../bin/bs arithmetics/error_invalid_hex_digit.bs
:i returncode 1
:b stdout 0

:b stderr 108
arithmetics/error_invalid_hex_digit.bs:1:5: error: invalid digit 'g' in number

    1 | 0x69g
      |     ^

:b shell 28
../bin/bs assignment/main.bs
:i returncode 0
:b stdout 79
69
420
69
420
1
2
1.59999999999999
0.399999999999997
69
420
69
420
69
420
1337

:b stderr 0

:b shell 28
../bin/bs assignment/join.bs
:i returncode 0
:b stdout 20
Hello
Hello, world!

:b stderr 0

:b shell 37
../bin/bs assignment/key_shorthand.bs
:i returncode 0
:b stdout 3
69

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

:b shell 25
../bin/bs strings/main.bs
:i returncode 0
:b stdout 143
Deez nuts
Joe Mama
69 Nice!
The truth is 420
Nested interpolation? Are you crazy?
69 and 420. Hehe
true
false
false
  69
true
false
false
true

:b stderr 0

:b shell 43
../bin/bs strings/error_invalid_addition.bs
:i returncode 1
:b stdout 0

:b stderr 294
strings/error_invalid_addition.bs:1:15: error: invalid operands to binary (+): number, string

    1 | io.println(69 + " Nice!")
      |               ^

Use ($) for string concatenation, or use string interpolation instead

```
"Hello, " $ "world!"
"Hello, " $ 69
"Hello, {34 + 35} nice!"
```

:b shell 28
../bin/bs strings/compare.bs
:i returncode 0
:b stdout 12
1
-1
0
-1
1

:b stderr 0

:b shell 28
../bin/bs variables/local.bs
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

:b shell 38
../bin/bs variables/error_undefined.bs
:i returncode 1
:b stdout 0

:b stderr 91
variables/error_undefined.bs:1:1: error: undefined identifier 'foo'

    1 | foo
      | ^

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

:b shell 45
../bin/bs arrays/error_index_out_of_bounds.bs
:i returncode 1
:b stdout 0

:b stderr 148
arrays/error_index_out_of_bounds.bs:3:14: error: cannot get value at index 0 in array of length 0

    3 | io.println(xs[0])
      |              ^

:b shell 39
../bin/bs arrays/error_invalid_index.bs
:i returncode 1
:b stdout 0

:b stderr 156
arrays/error_invalid_index.bs:2:4: error: expected array index or method name to be positive integer or string, got boolean

    2 | xs[false]
      |    ^

:b shell 46
../bin/bs arrays/error_invalid_index_assign.bs
:i returncode 1
:b stdout 0

:b stderr 145
arrays/error_invalid_index_assign.bs:2:4: error: expected array index to be positive integer, got string

    2 | xs["string"] = 69
      |    ^

:b shell 52
../bin/bs arrays/error_invalid_index_const_assign.bs
:i returncode 1
:b stdout 0

:b stderr 145
arrays/error_invalid_index_const_assign.bs:2:4: error: expected array index to be positive integer, got string

    2 | xs.foo = 69
      |    ^

:b shell 32
../bin/bs arrays/compare_sort.bs
:i returncode 0
:b stdout 80
["foo", "bar", "lol", "lmao", "foobar"]
["bar", "foo", "foobar", "lmao", "lol"]

:b stderr 0

:b shell 42
../bin/bs arrays/table_str_compare_sort.bs
:i returncode 0
:b stdout 852
[
    {
        isdir = false,
        name = 4
    },
    {
        isdir = false,
        name = 9
    },
    {
        isdir = false,
        name = 2
    },
    {
        isdir = false,
        name = 3
    },
    {
        isdir = true,
        name = 1
    },
    {
        isdir = false,
        name = 5
    },
    {
        isdir = true,
        name = 10
    },
    {
        isdir = false,
        name = 7
    }
]
[
    {
        isdir = true,
        name = 1
    },
    {
        isdir = false,
        name = 2
    },
    {
        isdir = false,
        name = 3
    },
    {
        isdir = false,
        name = 4
    },
    {
        isdir = false,
        name = 5
    },
    {
        isdir = false,
        name = 7
    },
    {
        isdir = false,
        name = 9
    },
    {
        isdir = true,
        name = 10
    }
]

:b stderr 0

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

:b shell 37
../bin/bs tables/error_invalid_key.bs
:i returncode 1
:b stdout 0

:b stderr 100
tables/error_invalid_key.bs:2:4: error: cannot use 'nil' as table key

    2 | xs[nil]
      |    ^

:b shell 44
../bin/bs tables/error_invalid_key_assign.bs
:i returncode 1
:b stdout 0

:b stderr 112
tables/error_invalid_key_assign.bs:2:4: error: cannot use 'nil' as table key

    2 | xs[nil] = 69
      |    ^

:b shell 47
../bin/bs containers/error_invalid_container.bs
:i returncode 1
:b stdout 0

:b stderr 109
containers/error_invalid_container.bs:2:2: error: cannot invoke or index into nil

    2 | x[420]
      |  ^

:b shell 53
../bin/bs containers/error_invalid_container_const.bs
:i returncode 1
:b stdout 0

:b stderr 114
containers/error_invalid_container_const.bs:2:2: error: cannot invoke or index into nil

    2 | x.foo
      |  ^

:b shell 54
../bin/bs containers/error_invalid_container_assign.bs
:i returncode 1
:b stdout 0

:b stderr 129
containers/error_invalid_container_assign.bs:2:2: error: cannot take mutable index into number

    2 | x[420] = 1337
      |  ^

:b shell 60
../bin/bs containers/error_invalid_container_const_assign.bs
:i returncode 1
:b stdout 0

:b stderr 133
containers/error_invalid_container_const_assign.bs:2:2: error: cannot take mutable index into number

    2 | x.foo = 420
      |  ^

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

:b shell 59
../bin/bs containers/error_in_operator_invalid_container.bs
:i returncode 1
:b stdout 0

:b stderr 119
containers/error_in_operator_invalid_container.bs:1:4: error: cannot index into number

    1 | 69 in 420
      |    ^

:b shell 55
../bin/bs containers/error_in_operator_invalid_index.bs
:i returncode 1
:b stdout 0

:b stderr 121
containers/error_in_operator_invalid_index.bs:1:5: error: cannot use 'nil' as table key

    1 | nil in {}
      |     ^

:b shell 59
../bin/bs containers/error_undefined_c_instance_property.bs
:i returncode 1
:b stdout 0

:b stderr 144
containers/error_undefined_c_instance_property.bs:1:9: error: undefined instance property or method: foo

    1 | Bytes().foo
      |         ^

:b shell 57
../bin/bs containers/error_undefined_instance_property.bs
:i returncode 1
:b stdout 0

:b stderr 138
containers/error_undefined_instance_property.bs:2:7: error: undefined instance property or method: foo

    2 | Foo().foo
      |       ^

:b shell 49
../bin/bs containers/error_undefined_table_key.bs
:i returncode 1
:b stdout 0

:b stderr 106
containers/error_undefined_table_key.bs:5:4: error: undefined table key: bar

    5 | xs.bar
      |    ^

:b shell 24
../bin/bs import/main.bs
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

:b shell 40
../bin/bs import/error_could_not_open.bs
:i returncode 1
:b stdout 0

:b stderr 135
import/error_could_not_open.bs:1:8: error: could not import module 'does_not_exist'

    1 | import("does_not_exist")
      |        ^

:b shell 41
../bin/bs import/error_expected_string.bs
:i returncode 1
:b stdout 0

:b stderr 127
import/error_expected_string.bs:1:8: error: expected module name to be string, got number

    1 | import(69)
      |        ^

:b shell 31
../bin/bs import/main_module.bs
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

:b shell 41
../bin/bs loops/error_invalid_iterator.bs
:i returncode 1
:b stdout 0

:b stderr 121
loops/error_invalid_iterator.bs:1:13: error: cannot iterate over number

    1 | for i, v in 69 {}
      |             ^

:b shell 44
../bin/bs loops/error_invalid_range_start.bs
:i returncode 1
:b stdout 0

:b stderr 140
loops/error_invalid_range_start.bs:1:10: error: expected range start to be number, got nil

    1 | for i in nil, nil {}
      |          ^

:b shell 42
../bin/bs loops/error_invalid_range_end.bs
:i returncode 1
:b stdout 0

:b stderr 137
loops/error_invalid_range_end.bs:1:13: error: expected range end to be number, got nil

    1 | for i in 0, nil {}
      |             ^

:b shell 43
../bin/bs loops/error_invalid_range_step.bs
:i returncode 1
:b stdout 0

:b stderr 152
loops/error_invalid_range_step.bs:1:17: error: expected range step to be number, got boolean

    1 | for i in 0, 10, true {}
      |                 ^

:b shell 27
../bin/bs functions/main.bs
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

:b shell 42
../bin/bs functions/error_invalid_arity.bs
:i returncode 1
:b stdout 0

:b stderr 96
functions/error_invalid_arity.bs:3:2: error: expected 1 argument, got 0

    3 | f()
      |  ^

:b shell 40
../bin/bs functions/error_stack_trace.bs
:i returncode 1
:b stdout 0

:b stderr 347
functions/error_stack_trace.bs:2:5: error: undefined identifier 'oops'

    2 |     oops
      |     ^

functions/error_stack_trace.bs:6:8: in baz()

    6 |     baz()
      |        ^

functions/error_stack_trace.bs:10:8: in bar()

    10 |     bar()
       |        ^

functions/error_stack_trace.bs:13:4: in foo()

    13 | foo()
       |    ^

:b shell 47
../bin/bs functions/error_native_stack_trace.bs
:i returncode 1
:b stdout 0

:b stderr 373
functions/error_native_stack_trace.bs:2:22: error: expected argument #1 to be string, got number

    2 |     return os.getenv(s)
      |                      ^

[C]: in foo()

functions/error_native_stack_trace.bs:6:15: in array.map()

    6 |     [1, 2].map(foo)
      |               ^

functions/error_native_stack_trace.bs:9:5: in main()

    9 | main()
      |     ^

:b shell 26
../bin/bs closures/main.bs
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

:b shell 22
../bin/bs typeof/is.bs
:i returncode 0
:b stdout 104
true
true
true
true
true
true
true
true
true
true
false
false
false
false
false
false
false
false
false

:b stderr 0

:b shell 33
../bin/bs pretty_printing/main.bs
:i returncode 0
:b stdout 323
{}
ligma
{
    ["1337 bro"] = {
        [69] = "nice",
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
        ]
    },
    bar = 420,
    foo = 69
}

:b stderr 0

:b shell 46
../bin/bs oop/error_cannot_return_from_init.bs
:i returncode 1
:b stdout 0

:b stderr 737
oop/error_cannot_return_from_init.bs:3:16: error: can only explicity return 'nil' from an initializer method

    3 |         return 69
      |                ^

When an initializer method explicitly returns 'nil', it indicates that the
initialization failed due to some reason, and the site of the instantiation
gets 'nil' as the result. This is not strictly OOP, but I missed the part where
that's my problem.

```
class Log {
    init(path) {
        this.file = io.Writer(path)
        if !this.file {
            return nil # Failed to open log file
        }
    }

    write(s) => this.file.writeln(s)
}

var log = Log("log.txt")
if !log {
    panic() # Handle error
}

log.write("Hello, world!") # Or whatever you want to do
```

:b shell 41
../bin/bs oop/error_this_outside_class.bs
:i returncode 1
:b stdout 0

:b stderr 105
oop/error_this_outside_class.bs:1:1: error: cannot use 'this' outside of 'class'

    1 | this
      | ^

:b shell 44
../bin/bs oop/error_with_init_wrong_arity.bs
:i returncode 1
:b stdout 0

:b stderr 102
oop/error_with_init_wrong_arity.bs:5:4: error: expected 1 argument, got 0

    5 | Foo()
      |    ^

:b shell 47
../bin/bs oop/error_without_init_wrong_arity.bs
:i returncode 1
:b stdout 0

:b stderr 108
oop/error_without_init_wrong_arity.bs:3:4: error: expected 0 arguments, got 1

    3 | Foo(69)
      |    ^

:b shell 34
../bin/bs oop/error_inside_init.bs
:i returncode 1
:b stdout 0

:b stderr 179
oop/error_inside_init.bs:3:9: error: invalid operand to unary (-): nil

    3 |         -nil
      |         ^

oop/error_inside_init.bs:7:4: in Foo()

    7 | Foo()
      |    ^

:b shell 41
../bin/bs oop/error_inside_method_call.bs
:i returncode 1
:b stdout 0

:b stderr 210
oop/error_inside_method_call.bs:3:9: error: invalid operand to unary (-): nil

    3 |         -nil
      |         ^

oop/error_inside_method_call.bs:7:10: in Foo.foo()

    7 | Foo().foo()
      |          ^

:b shell 41
../bin/bs oop/return_without_expr_init.bs
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

:b shell 29
../bin/bs oop/without_init.bs
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

:b shell 45
../bin/bs oop/error_undefined_super_method.bs
:i returncode 1
:b stdout 0

:b stderr 203
oop/error_undefined_super_method.bs:4:15: error: undefined super method: foo

    4 |         super.foo()
      |               ^

oop/error_undefined_super_method.bs:8:2: in A()

    8 | A()
      |  ^

:b shell 29
../bin/bs invokation/chain.bs
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

:b stderr 113
invokation/error_call_invalid_container.bs:2:2: error: cannot invoke or index into nil

    2 | t.f()
      |  ^

:b shell 46
../bin/bs invokation/error_call_invalid_key.bs
:i returncode 1
:b stdout 0

:b stderr 157
invokation/error_call_invalid_key.bs:2:4: error: expected array index or method name to be positive integer or string, got nil

    2 | xs[nil]
      |    ^

:b shell 42
../bin/bs invokation/error_invoked_body.bs
:i returncode 1
:b stdout 0

:b stderr 222
invokation/error_invoked_body.bs:3:13: error: invalid operands to binary (+): nil, number

    3 |         nil + 69
      |             ^

invokation/error_invoked_body.bs:7:4: in <anonymous>()

    7 | t.f()
      |    ^

:b shell 56
../bin/bs invokation/error_key_call_argument_location.bs
:i returncode 1
:b stdout 0

:b stderr 147
invokation/error_key_call_argument_location.bs:5:5: error: expected argument #1 to be positive integer, got nil

    5 | t.f(nil, 0)
      |     ^

:b shell 55
../bin/bs invokation/error_key_call_invalid_function.bs
:i returncode 1
:b stdout 0

:b stderr 105
invokation/error_key_call_invalid_function.bs:5:4: error: cannot call number

    5 | t.f()
      |    ^

:b shell 50
../bin/bs invokation/error_key_call_wrong_arity.bs
:i returncode 1
:b stdout 0

:b stderr 111
invokation/error_key_call_wrong_arity.bs:7:4: error: expected 0 arguments, got 1

    7 | t.f(69)
      |    ^

:b shell 53
../bin/bs invokation/error_method_call_wrong_arity.bs
:i returncode 1
:b stdout 0

:b stderr 118
invokation/error_method_call_wrong_arity.bs:8:6: error: expected 0 arguments, got 1

    8 | foo.f(69)
      |      ^

:b shell 59
../bin/bs invokation/error_native_call_argument_location.bs
:i returncode 1
:b stdout 0

:b stderr 184
invokation/error_native_call_argument_location.bs:1:21: error: expected argument #1 to be positive integer, got nil

    1 | io.println("".slice(nil, 0))
      |                     ^

:b shell 53
../bin/bs invokation/error_native_call_wrong_arity.bs
:i returncode 1
:b stdout 0

:b stderr 164
invokation/error_native_call_wrong_arity.bs:1:28: error: expected 0 arguments, got 1

    1 | io.println("Hello!".reverse(69))
      |                            ^

:b shell 61
../bin/bs invokation/error_property_call_argument_location.bs
:i returncode 1
:b stdout 0

:b stderr 156
invokation/error_property_call_argument_location.bs:5:7: error: expected argument #1 to be positive integer, got nil

    5 | foo.f(nil, 0)
      |       ^

:b shell 60
../bin/bs invokation/error_property_call_invalid_function.bs
:i returncode 1
:b stdout 0

:b stderr 114
invokation/error_property_call_invalid_function.bs:5:6: error: cannot call number

    5 | foo.f()
      |      ^

:b shell 55
../bin/bs invokation/error_property_call_wrong_arity.bs
:i returncode 1
:b stdout 0

:b stderr 120
invokation/error_property_call_wrong_arity.bs:8:6: error: expected 0 arguments, got 1

    8 | foo.f(69)
      |      ^

:b shell 32
../bin/bs invokation/key_call.bs
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

:b shell 35
../bin/bs invokation/native_call.bs
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

:b shell 45
../bin/bs delete/error_cannot_delete_super.bs
:i returncode 1
:b stdout 0

:b stderr 138
delete/error_cannot_delete_super.bs:4:16: error: cannot use 'delete' on super

    4 |         delete(super.foo)
      |                ^

:b shell 51
../bin/bs delete/error_expected_index_expression.bs
:i returncode 1
:b stdout 0

:b stderr 237
delete/error_expected_index_expression.bs:2:8: error: expected index expression

    2 | delete(xs)
      |        ^

Index expression can be any of the following:

```
xs.foo;    # Constant index
xs["bar"]; # Expression based index
```

:b shell 43
../bin/bs delete/error_invalid_container.bs
:i returncode 1
:b stdout 0

:b stderr 113
delete/error_invalid_container.bs:2:9: error: cannot delete from number

    2 | delete(x.foo)
      |         ^

:b shell 51
../bin/bs delete/error_invalid_instance_property.bs
:i returncode 1
:b stdout 0

:b stderr 144
delete/error_invalid_instance_property.bs:2:14: error: cannot use 'nil' as instance property

    2 | delete(Foo()[nil])
      |              ^

:b shell 43
../bin/bs delete/error_invalid_table_key.bs
:i returncode 1
:b stdout 0

:b stderr 122
delete/error_invalid_table_key.bs:2:11: error: cannot use 'nil' as table key

    2 | delete(xs[nil])
      |           ^

:b shell 37
../bin/bs delete/instance_property.bs
:i returncode 0
:b stdout 66
Foo {
    x = 69,
    y = 420
}
69
Foo {
    y = 420
}
420
Foo {}

:b stderr 0

:b shell 25
../bin/bs delete/table.bs
:i returncode 0
:b stdout 60
{
    bar = 420,
    foo = 69
}
69
{
    bar = 420
}
420
{}

:b stderr 0

:b shell 34
../bin/bs builtin_methods/array.bs
:i returncode 0
:b stdout 10
[2, 4, 6]

:b stderr 0

:b shell 35
../bin/bs builtin_methods/number.bs
:i returncode 0
:b stdout 2
9

:b stderr 0

:b shell 35
../bin/bs builtin_methods/string.bs
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

:b shell 50
../bin/bs builtin_methods/error_undefined_array.bs
:i returncode 1
:b stdout 0

:b stderr 106
builtin_methods/error_undefined_array.bs:1:4: error: undefined method: foo

    1 | [].foo()
      |    ^

:b shell 51
../bin/bs builtin_methods/error_undefined_number.bs
:i returncode 1
:b stdout 0

:b stderr 107
builtin_methods/error_undefined_number.bs:1:4: error: undefined method: foo

    1 | 69.foo()
      |    ^

:b shell 48
../bin/bs builtin_methods/error_undefined_str.bs
:i returncode 1
:b stdout 0

:b stderr 104
builtin_methods/error_undefined_str.bs:1:4: error: undefined method: foo

    1 | "".foo()
      |    ^

:b shell 24
../bin/bs core/string.bs
:i returncode 0
:b stdout 274
Foo
bar

FOOBAR
foobar
4
8
nil
nil
["foo bar baz"]
["foo", "bar", "baz"]
["foo bar baz"]
foo bar baz
foo---bar---baz
foo bar ba
foo bar baz
foo bar baz  
   foo bar baz
foo
foo
6foo
69foo
696foo
foo
foo
foo6
foo69
foo696
true
false
false
true
false
false
foofoofoofoofoofoo

:b stderr 0

:b shell 23
../bin/bs core/array.bs
:i returncode 0
:b stdout 704
[2, 4, 6, 8, 10]
[2, 4]
15
35
xs = [1, 2, 3, 4, 5]
ys = [1, 2, 3, 4, 5]
zs = [1, 2, 3, 4, 5]
xs = [69, 2, 3, 4, 5]
ys = [1, 2, 3, 4, 5]
zs = [69, 2, 3, 4, 5]
1
4
true
true
false
Before: [2, 5, 1, 4, 3]
Return: [1, 2, 3, 4, 5]
Final:  [1, 2, 3, 4, 5]
Before: [2, 5, 1, 4, 3]
Return: [3, 4, 1, 5, 2]
Final:  [3, 4, 1, 5, 2]
[1, 2, 3, 4, 5]
[
    1,
    2,
    3,
    4,
    5,
    "Temp #0",
    "Temp #1",
    "Temp #2",
    "Temp #3",
    "Temp #4"
]
[1, 2, 3, 4, 5]
[
    1,
    2,
    3,
    4,
    5,
    nil,
    nil,
    nil,
    nil,
    nil
]
[1, 2, 3, 4, 5]
[3, 4, 5]
[2, 3]
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
[1, 2, 3, 4, 5]
5
[1, 2, 3, 4]

:b stderr 0

:b shell 23
../bin/bs core/table.bs
:i returncode 0
:b stdout 381
xs = {
    bar = 420,
    foo = 69
}
ys = {
    bar = 420,
    foo = 69
}
zs = {
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
zs = {
    bar = 1337,
    foo = 69
}
true
true
false
{
    bar = 420,
    foo = 69
}
{
    baz = 42,
    bar = 1337,
    foo = 69
}
{
    lol = 420,
    baz = 42,
    bar = 1337,
    foo = 69
}

:b stderr 0

:b shell 22
../bin/bs core/math.bs
:i returncode 0
:b stdout 457
2.71828182845905
3.14159265358979
0
1
1
-1
0
1.63312393531954e+16
-1.22464679914735e-16
0
1.5707963267949
0
1.5707963267949
3.14159265358979
0
1.5707963267949
8
8.30662386291807
421
-420
420
-421
421
69
-421
-69
2
2
3
3
1
1
1
1
4
1
6
6
0
1
-1
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
[1, 3, 5, 7, 9]
[
    11,
    10,
    9,
    8,
    7,
    6,
    5,
    4,
    3,
    2
]
[11, 9, 7, 5, 3]
3.14159
0
420.69
420.69
45
-1a4

:b stderr 0

:b shell 21
../bin/bs core/bit.bs
:i returncode 0
:b stdout 13
8
8
16
4
8
8

:b stderr 0

:b shell 23
../bin/bs core/ascii.bs
:i returncode 0
:b stdout 282
E
69
true
true
true
false
true
true
false
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
false
false
false
true
true
true
false
false
false
true
true
true
true
true
false
false
false
false
false
true
true
false
false
false
false
true
true
true
false
true
false
false
false

:b stderr 0

:b shell 41
../bin/bs core/error_expected_function.bs
:i returncode 1
:b stdout 0

:b stderr 127
core/error_expected_function.bs:1:8: error: expected argument #1 to be function, got nil

    1 | [].map(nil)
      |        ^

:b shell 23
../bin/bs core/regex.bs
:i returncode 0
:b stdout 203
nil
0
["foo"]
foo
0
1
nil
nil
["foobar"]
["foo", "bar"]
["foo", "bar", "baz"]
{type: apples, count: 69}, {type: oranges, count: 420}
{type: apples, count: 69}, 420  oranges
ayo noice!
["", ",420"]
-,420

:b stderr 0

:b shell 21
../bin/bs ffi/main.bs
:i returncode 0
:b stdout 7
69
420

:b stderr 0

:b shell 38
../bin/bs ffi/error_invalid_library.bs
:i returncode 1
:b stdout 0

:b stderr 727
ffi/error_invalid_library.bs:1:8: error: invalid native library 'executables/invalid'

    1 | import("executables/invalid")
      |        ^

A BS native library must define 'bs_library_init'

```
BS_LIBRARY_INIT void bs_library_init(Bs *bs, Bs_C_Lib *library) {
    // Perform any initialization you wish to do
    // lolcat_init();

    // Add BS values to the library
    // bs_c_lib_set(bs, library, Bs_Sv_Static("foo"), bs_value_object(lolcat_foo));

    // Add multiple BS native functions to the library at once
    // static const Bs_FFI ffi[] = {
    //     {"hello", lolcat_hello},
    //     {"urmom", lolcat_urmom},
    //     /* ... */
    // };
    // bs_c_lib_ffi(bs, library, ffi, bs_c_array_size(ffi));
}
```

:b shell 38
../bin/bs core/os_process_with_args.bs
:i returncode 0
:b stdout 12
foo
bar
baz

:b stderr 23
Number of arguments: 4

:b shell 43
../bin/bs core/os_process_capture_stdout.bs
:i returncode 0
:b stdout 0

:b stderr 31
Number of arguments: 3
foo
bar

:b shell 43
../bin/bs core/os_process_capture_stderr.bs
:i returncode 0
:b stdout 31
foo
bar
Number of arguments: 3

:b stderr 0

:b shell 42
../bin/bs core/os_process_capture_stdin.bs
:i returncode 0
:b stdout 36
Echo stdin!
> foo
> bar
> baz
Done!

:b stderr 0

:b shell 24
../bin/bs panic/panic.bs
:i returncode 1
:b stdout 0

:b stderr 53
panic/panic.bs:1:1: panic

    1 | panic()
      | ^

:b shell 31
../bin/bs panic/with_message.bs
:i returncode 1
:b stdout 0

:b stderr 65
panic/with_message.bs:1:1: Here

    1 | panic("Here")
      | ^

:b shell 26
../bin/bs assert/assert.bs
:i returncode 1
:b stdout 0

:b stderr 72
assert/assert.bs:2:1: assertion failed

    2 | assert(false)
      | ^

:b shell 32
../bin/bs assert/with_message.bs
:i returncode 1
:b stdout 0

:b stderr 76
assert/with_message.bs:2:1: Ligma

    2 | assert(false, "Ligma")
      | ^

:b shell 25
../bin/bs core/readdir.bs
:i returncode 0
:b stdout 402
arithmetics DIR
arrays DIR
assert DIR
assignment DIR
builtin_methods DIR
closures DIR
comparisons DIR
conditions DIR
containers DIR
core DIR
delete DIR
executables DIR
ffi DIR
functions DIR
import DIR
invokation DIR
lexer DIR
loops DIR
match DIR
not_is_in_binary_op DIR
oop DIR
panic DIR
pretty_printing DIR
rere.py FILE
strings DIR
tables DIR
test.list FILE
test.list.bi FILE
typeof DIR
variables DIR

:b stderr 0

:b shell 37
../bin/bs not_is_in_binary_op/main.bs
:i returncode 0
:b stdout 22
false
true
false
true

:b stderr 0

:b shell 26
../bin/bs strings/index.bs
:i returncode 0
:b stdout 48
0 f
1 o
2 o
3 b
4 a
5 r
0 f
1 o
2 o
3 b
4 a
5 r

:b stderr 0

:b shell 55
../bin/bs core/math_range_indefinite_ascending_guard.bs
:i returncode 1
:b stdout 0

:b stderr 176
core/math_range_indefinite_ascending_guard.bs:1:19: error: a step of -1 in an ascending range would run indefinitely

    1 | math.range(0, 10, -1)
      |                   ^

:b shell 56
../bin/bs core/math_range_indefinite_descending_guard.bs
:i returncode 1
:b stdout 0

:b stderr 175
core/math_range_indefinite_descending_guard.bs:1:19: error: a step of 1 in a descending range would run indefinitely

    1 | math.range(10, 0, 1)
      |                   ^

:b shell 33
../bin/bs strings/single_quote.bs
:i returncode 0
:b stdout 35
Hello, world!
Nice, 69!
Here: '"'!

:b stderr 0

:b shell 41
../bin/bs invokation/asi_esception_dot.bs
:i returncode 0
:b stdout 11
XxfoobarXx

:b stderr 0

:b shell 22
../bin/bs core/meta.bs
:i returncode 1
:b stdout 168
<fn>
69
Row: 1
Col: 5
Line: Oops@
Message: invalid character '@' (64)
Explanation: nil
Example: nil
420
Nice!
Nice!
Nice!
Nice!
Nice!
Nice!
Nice!
Nice!
Nice!
Nice!
nil

:b stderr 178
<meta>:1:5: error: invalid character '@' (64)

    1 | Hehe@
      |     ^

core/meta.bs:25:21: in eval()

    25 | io.println(meta.eval("Hehe@"))
       |                     ^

:b shell 53
../bin/bs conditions/invocation_optimization_guard.bs
:i returncode 0
:b stdout 6
First

:b stderr 7
Second

:b shell 23
../bin/bs match/main.bs
:i returncode 0
:b stdout 77
A
A
B
A
x = 0
A
x = 1
A
x = 69
B
x = 420
B
x = 1337
B
x = 42
C
x = 5
D
HERE!

:b stderr 0

:b shell 41
../bin/bs match/error_cannot_use_class.bs
:i returncode 1
:b stdout 0

:b stderr 137
match/error_cannot_use_class.bs:2:11: error: cannot use 'class' here without wrapping in {}

    2 |     69 -> class
      |           ^

:b shell 38
../bin/bs match/error_cannot_use_fn.bs
:i returncode 1
:b stdout 0

:b stderr 128
match/error_cannot_use_fn.bs:2:11: error: cannot use 'fn' here without wrapping in {}

    2 |     69 -> fn
      |           ^

:b shell 39
../bin/bs match/error_cannot_use_var.bs
:i returncode 1
:b stdout 0

:b stderr 131
match/error_cannot_use_var.bs:2:11: error: cannot use 'var' here without wrapping in {}

    2 |     69 -> var
      |           ^

:b shell 27
../bin/bs lexer/comments.bs
:i returncode 0
:b stdout 14
7
10
12
13
69

:b stderr 0

:b shell 55
../bin/bs lexer/multiline_string_binary_continuation.bs
:i returncode 1
:b stdout 16
69Nice!
Lol 420

:b stderr 141
lexer/multiline_string_binary_continuation.bs:18:6: error: invalid operands to binary (-): string, number

    18 | Bar" - 1
       |      ^

:b shell 41
../bin/bs core/meta_eval_runtime_error.bs
:i returncode 1
:b stdout 0

:b stderr 194
<meta>:1:5: error: invalid operands to binary (+): nil, number

    1 | nil + 69
      |     ^

core/meta_eval_runtime_error.bs:1:10: in eval()

    1 | meta.eval("nil + 69")
      |          ^

:b shell 33
../bin/bs core/meta_call_error.bs
:i returncode 0
:b stdout 461
OK!
69

ERROR!
Row: 20
Col: 27
Path: core/meta_call_error.bs
Line: handle(meta.call(fn () -> -nil))              // Bs Fail
Message: invalid operand to unary (-): nil
Explanation: nil
Example: nil

ERROR!
Row: nil
Col: nil
Path: nil
Line: nil
Message: expected 1 or 2 arguments, got 0
Explanation: nil
Example: nil

ERROR!
Row: 27
Col: 15
Path: core/meta_call_error.bs
Line:     fn h() -> lmao
Message: undefined identifier 'lmao'
Explanation: nil
Example: nil

:b stderr 0

:b shell 25
../bin/bs delete/array.bs
:i returncode 0
:b stdout 31
[0, 2, 4, 6, 8]
4
[0, 2, 6, 8]

:b stderr 0

:b shell 39
../bin/bs delete/array_invalid_index.bs
:i returncode 1
:b stdout 0

:b stderr 141
delete/array_invalid_index.bs:1:11: error: expected array index to be positive integer, got nil

    1 | delete([][nil])
      |           ^

:b shell 44
../bin/bs delete/array_out_of_range_index.bs
:i returncode 1
:b stdout 0

:b stderr 146
delete/array_out_of_range_index.bs:1:12: error: cannot delete item at index 1 from array of length 1

    1 | delete([1][1])
      |            ^

:b shell 32
../bin/bs functions/variadics.bs
:i returncode 0
:b stdout 106
[]
[69]
[69, 420, 1337]
69
[]
69
[420]
69
[420, 1337, 666]
69
420
[]
69
420
[1337]
69
420
[1337, 666, 42]

:b stderr 0

:b shell 29
../bin/bs functions/spread.bs
:i returncode 0
:b stdout 7
69
420

:b stderr 0

:b shell 21
../bin/bs match/if.bs
:i returncode 0
:b stdout 2
4

:b stderr 0

:b shell 29
../bin/bs conditions/match.bs
:i returncode 0
:b stdout 2
4

:b stderr 0

:b shell 33
../bin/bs strings/quoted_print.bs
:i returncode 0
:b stdout 30
["Hello \e\n\r\t\0\"''\\\{}"]

:b stderr 0

:b shell 37
../bin/bs import/from_file_dirname.bs
:i returncode 0
:b stdout 60
Module 'arith'
Module 'arith/add'
Module 'arith/sub'
69
420

:b stderr 0

