XPR
===

XPR is a KISS library to evaluate arithmetic expressions. All calculations use
double-precision floating-point numbers internally. XPR supports standard
operators, many arithmetic functions, nested braces and function calls, and
named variables.


Basic Usage
-----------

The XPR library intentionally provides a minimal interface. It is completely
thread-safe, because no intermediate states are accessible. The `xpr` function
takes the arithmetic expression, evaluates it, and returns the result.

	#include <xpr.h>
	// ... later ...
	const char *expr = "1+1";
	double d = xpr(expr, NULL);
	printf("%ld\n", d); // prints 2.0

The return value is the result of the given expression. On error, `xpr` returns
NAN.


Variables
---------

XPR supports variables, which are basically a mapping from an identifier (the
variable names) to a value. All variables remain constant during the evaluation
of an expression, since no assignment operator exists. However, an application
can re-evaluate an expression using a different set of variables.

The second parameter of the `xpr` function takes an array of variables. The
last entry must have the name `NULL`, combined with an arbitrary value. This
special entry serves as array end indicator. The second argument can be `NULL`,
which is equivalent to an empty variable list.

	#include <xpr.h>
	// ... later ...
	xpr_var variables[2] = {
		{ "x", 1.0 },        // x = 1.0
		{ NULL, 0 }          // end indicator
	};
	const char *expr = "1+x";
	double d = xpr(expr, variables);
	printf("%ld\n", d); // prints 2.0

Since variables do not change during the evaluation, concurrent calls to
`xpr()` can use the same variable array. However, the list of variables and
their values must not change concurrently.

