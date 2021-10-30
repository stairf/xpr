# XPR

![make-ci](https://github.com/stairf/xpr/actions/workflows/make-ci.yml/badge.svg)

XPR is a KISS library to evaluate arithmetic expressions (for example, it
evaluates the string `"1+1"` to the number `2`). All calculations use
double-precision floating-point numbers internally. XPR supports canonical
arithmetic operators, various functions, nested braces, function calls, and
named constants and variables.

### Contents

 - [Why XPR](#why-xpr)
 - [Basic Usage](#basic-usage)
 - [Constants](#constants)
 - [Functions](#functions)
 - [Variables](#variables)
 - [Concurrency](#concurrency)


## Why XPR?

 - XPR is **intuitive**, **powerful**, and **correct**.

   XPR evaluates arbitrary arithmetic expressions (with the precision of the
   `double` data type). It supports canonical arithmetic unary and binary
   operators, function calls, braces with unlimited nesting depth, and named
   identifiers. Syntactically wrong expressions and illegal computations
   (e.g., division by zero) are reliably detected and indicated as errors.

 - XPR is **fast**.

   Computation with XPR is much faster than programming language interpreters.
   The time complexity of `xpr()` is linear in the length of the given
   expression.

 - XPR is **safe**.

   XPR works correctly even if the given expression is invalid. For example,
   dividing by zero is proactively detected and handled appropriately (by
   signalling an error), and does not cause a crash. It is not possible to
   inject code, to read or modify the file system, or to access networks.

 - XPR is **well-tested**.

   XPR has extensive hand-written tests that cover both valid and invalid input,
   a randomized test suite, and a fuzzing interface.


## Basic Usage

The XPR library intentionally provides a minimal interface, following an *make
it easy to use it right, and difficult to use it incorrectly* approach. No
internal state is exposed. The `xpr` function reads the arithmetic expression,
evaluates it, and returns the result.

```c
#include <xpr.h>
// ... later ...
const char *expr = "1+1";
double d = xpr(expr, NULL);
printf("%ld\n", d); // prints 2.0
```

The return value is the result of the given expression. On error, `xpr()` returns
`NAN`. Use `isnan()` to check for errors.


## Constants

Constants are a built-in mapping of identifiers to values.

### Example

```c
const char *expression = "((2*phi)-1)^2";
double computed = xpr(expression, NULL);
printf("%ld\n", computed); // prints 5.0
```

### Overview
The following constants are supported:

| Name  | Approximate value | Description                                       |
| :---- | :---------------- | :------------------------------------------------ |
| `e`   | `2.718282`        | euler's number                                    |
| `phi` | `1.618034`        | golden ratio                                      |
| `pi`  | `3.141593`        | ratio of a circle's circumference to its diameter |


## Functions

Functions compute a result from their arguments. The result will be a value or
an error.

For functions, the number of arguments is important. The result of a function is
an error if the argument count is wrong. Some functions (e.g., `log`) behave
differently, depending on how many arguments are given. Some functions (e.g.,
`max`) have no upper limit on the number of arguments.

### Example

```c
const char *expression = "sqrt((1+1)*2)";
double computed = xpr(expression, NULL);
printf("%ld\n", computed); // prints 2.0
```

### Overview

The following functions are supported:

| Name               | Description                                                                                |
| :---------------   | :----------------------------------------------------------------------------------------- |
| `(x)`              | Identity function, returns `x`                                                             |
| `acos(x)`          | Inverse cosine                                                                             |
| `acosh(x)`         | Inverse hyperbolic cosine                                                                  |
| `asin(x)`          | Inverse sine                                                                               |
| `asinh(x)`         | Inverse hyperbolic sine                                                                    |
| `atan(x)`          | Inverse tangent                                                                            |
| `atanh(x)`         | Inverse hyperbolic tangent                                                                 |
| `cbrt(x)`          | Cubic root, same as `x^(1/3)`                                                              |
| `ceil(x)`          | Find the next integer near `x` towards `+inf`                                              |
| `cos(x)`           | Cosine, where `x` is in radians                                                            |
| `cosh(x)`          | Hyperbolic cosine                                                                          |
| `exp(x)`           | Exponential function with base `e`, use operator `a^b` to raise arbitrary values           |
| `floor(x)`         | Find the next integer near `x` towards `-inf`                                              |
| `log(x)`           | Natural logarithm of `x`                                                                   |
| `log(b,x)`         | Logarithm of `x` with base `b`                                                             |
| `max(a0,...)`      | Maximum of all given values                                                                |
| `min(a0,...)`      | Minimum of all given values                                                                |
| `round(x)`         | Find the integer closest to `x`                                                            |
| `scale(A,B,x)`     | Translate `x` from scale `[0,A]` to scale `[0,B]`                                          |
| `scale(a,A,b,B,x)` | Translate `x` from scale `[a,A]` to scale `[b,B]`                                          |
| `sin(x)`           | Sine, where `x` is in radians                                                              |
| `sinh(x)`          | Hyperbolic sine                                                                            |
| `sqrt(x)`          | Square root, same as `x^0.5`                                                               |
| `sum(...)`         | Sum of all arguments                                                                       |
| `tan(x)`           | Tangent, where `x` is in radians                                                           |
| `tanh(x)`          | Hyperbolic tangent                                                                         |


## Variables

Like constants, variables map identifiers to values. The difference between
constants and variables is that the former are automatically recognized, while
the latter are configurable.

The second argument of `xpr()` is a list containing variables, terminated by an
entry where the identifier is `NULL`. Passing `NULL` as list of variables is
equivalent to an empty list.

The list of variables is read-only---the `xpr()` function does not modify it.
Variables may only change between invocations of `xpr()`. Evaluating the same
expression with a different list of variables may lead to a different result.

### Example

```c
#include <xpr.h>
// ... later ...
xpr_var variables[2] = {
	{ "x", 1.0 },        // x := 1.0
	{ NULL, 0 }          // end indicator
};
const char *expr = "1+x";
double d = xpr(expr, variables);
printf("%ld\n", d); // prints 2.0
```

## Concurrency

The `xpr()` function is entirely thread-safe. It does not expose any
intermediate states that could cause race conditions. In addition, it does not
have any global state. All called library functions are annotated as `MT-Safe`
in the glibc documentation.

Lists of variables can be used by concurrent calls to `xpr()`, because they are
read-only. However, the list must not be modified concurrently during an
invocation of `xpr()`.


