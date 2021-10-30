/*****
 * Copyright (c) 2015-2016, Stefan Reif
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****/

/*
 * xpr.c
 *
 * This file implements the XPR function. It uses a shift/reduce style parser
 * that operates on a stack. The stack contains a representation of all tokens
 * that have been read so far, partially evaluated. The shift operation pushes
 * the next token onto the stack. The reduce operation applies operators, which
 * reduces the stack size.
 *
 * For the reduce operation, most tokens have a BS field, which stores the
 * binding strenght to the left side. When the new operator on the righ-hand
 * side has a lower binding strength, the reduce function evaluates pending
 * operators until the left-hand side binds stronger. The reduce operation
 * evaluates all operators as early as possible.
 *
 * In consequence, the binding strength of all operators on the stack is sorted
 * in ascending order, except for braces. An TK_OPEN token can be pushed onto
 * any token, and the TK_CLOSE token can only be temporarly on the stack. The
 * TK_CLOSE token thus enforces reduction until the elimination of the
 * corresponding TK_OPEN token. More precisely, the TK_CLOSE token has the
 * lowest binding strength and thus repeatedly triggers the reduce operation,
 * until the reduce operation detects that the TK_CLOSE token consumes the
 * TK_OPEN token.
 *
 * Functions are represented by explicit function tokens. When handling a pair
 * of braces, (i.e. TK_OPEN and TK_CLOSE), the reduce function searches for a
 * function token ahead of TK_OPEN. The absence of a function token represents
 * the identity function.
 *
 */
#include "xpr.h"
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>
#include <alloca.h>
#include <limits.h>
#include <stdint.h>

/*
 * the tok.tag value is a bit field:
 *
 * CLASS = val & 0xf
 *  -> NONE
 *     tokens space, eof, error
 *  -> VALUE (only TK_NUM)
 *     bs        = val & 0xf000
 *  -> OP
 *     id        = val & 0x00f0
 *     can unary = val & 0x0100
 *     is unary  = val & 0x0200
 *     left as   = val & 0x0400
 *     right as  = val & 0x0800
 *     bs        = val & 0xf000
 *  -> FUNC
 *     id        = val & 0x0ff0
 */

#define CLASS_NONE       0x0000
#define CLASS_VALUE      0x0001
#define CLASS_OP         0x0002
#define CLASS_FUNC       0x0003
#define CLASS_GET(tk)    ((tk) & 0xf)

#define TK_OP_BY_ID(id)  ((id) << 4 | CLASS_OP)
#define TK_FUN_BY_ID(id) ((id) << 4 | CLASS_FUNC)
#define FUNID(tk)        ((tk) >> 4 & 0xff)

#define TK_OP_CAN_UNARY  0x0100
#define TK_OP_IS_UNARY   0x0200

#define AS_LEFT          0x0400
#define AS_RIGHT         0x0800
#define AS_GET(tk)       ((tk) & (AS_LEFT | AS_RIGHT))
#define AS_SET(as, tk)   ((as) | ((tk) & ~(AS_LEFT | AS_RIGHT)))

#define BS_NONE          0
#define BS_OPEN          1
#define BS_PLUS          2
#define BS_MUL           3
#define BS_EXP           4
#define BS_UNARY         5

#define BS_SET(bs, tk)   (((bs) << 12) | ((tk) & 0xfff))
#define BS_GET(tk)       (((tk) >> 12) & 0xf)
#define BS_IGN(tk)       ((tk) & 0xfff)

#define TK_NUM           CLASS_VALUE
#define TK_SPACE         (0x10 | CLASS_NONE)
#define TK_EOF           (0x20 | CLASS_NONE)
#define TK_ERR           (0x30 | CLASS_NONE)

#define TK_PLUS          BS_SET(BS_PLUS,  TK_OP_BY_ID(0x1) | AS_LEFT | TK_OP_CAN_UNARY)
#define TK_MINUS         BS_SET(BS_PLUS,  TK_OP_BY_ID(0x2) | AS_LEFT | TK_OP_CAN_UNARY)
#define TK_MUL           BS_SET(BS_MUL,   TK_OP_BY_ID(0x3) | AS_LEFT)
#define TK_DIV           BS_SET(BS_MUL,   TK_OP_BY_ID(0x4) | AS_LEFT)
#define TK_EXP           BS_SET(BS_EXP,   TK_OP_BY_ID(0x5) | AS_LEFT)
#define TK_OPEN          BS_SET(BS_OPEN,  TK_OP_BY_ID(0x6) | AS_LEFT)
#define TK_COMMA         BS_SET(BS_OPEN,  TK_OP_BY_ID(0x7) | AS_LEFT)
#define TK_CLOSE         BS_SET(BS_NONE,  TK_OP_BY_ID(0x8) | AS_RIGHT)

#define TK_TO_UNARY(tk)  BS_SET(BS_UNARY, AS_SET(AS_RIGHT, (tk) | TK_OP_IS_UNARY))
#define TK_UMINUS        TK_TO_UNARY(TK_MINUS)
#define TK_UPLUS         TK_TO_UNARY(TK_PLUS)

// function tokens
#define TK_FUN_NONE      TK_FUN_BY_ID(0x00)
#define TK_FUN_ACOS      TK_FUN_BY_ID(0x01)
#define TK_FUN_ACOSH     TK_FUN_BY_ID(0x02)
#define TK_FUN_ASIN      TK_FUN_BY_ID(0x03)
#define TK_FUN_ASINH     TK_FUN_BY_ID(0x04)
#define TK_FUN_ATAN      TK_FUN_BY_ID(0x05)
#define TK_FUN_ATANH     TK_FUN_BY_ID(0x06)
#define TK_FUN_CBRT      TK_FUN_BY_ID(0x07)
#define TK_FUN_CEIL      TK_FUN_BY_ID(0x08)
#define TK_FUN_COS       TK_FUN_BY_ID(0x09)
#define TK_FUN_COSH      TK_FUN_BY_ID(0x0a)
#define TK_FUN_EXP       TK_FUN_BY_ID(0x0b)
#define TK_FUN_FLOOR     TK_FUN_BY_ID(0x0c)
#define TK_FUN_LOG       TK_FUN_BY_ID(0x0d)
#define TK_FUN_MAX       TK_FUN_BY_ID(0x0e)
#define TK_FUN_MIN       TK_FUN_BY_ID(0x0f)
#define TK_FUN_ROUND     TK_FUN_BY_ID(0x10)
#define TK_FUN_SCALE     TK_FUN_BY_ID(0x11)
#define TK_FUN_SIN       TK_FUN_BY_ID(0x12)
#define TK_FUN_SINH      TK_FUN_BY_ID(0x13)
#define TK_FUN_SQRT      TK_FUN_BY_ID(0x14)
#define TK_FUN_SUM       TK_FUN_BY_ID(0x15)
#define TK_FUN_TAN       TK_FUN_BY_ID(0x16)
#define TK_FUN_TANH      TK_FUN_BY_ID(0x17)


typedef struct token {
	int tag;
	union token_data {
		double value;
	} data;
} tok;

typedef double (*fun)(int nargs, const tok *args);
typedef struct xpr_var var;

#include "dbg.h"
#include "fun.h"

static inline void next_num(const char **const strp, tok *const out)
{
	const char *start = *strp;
	double val = strtod(start, (char **) strp);
	if (*strp == start) {
		out->tag = TK_ERR;
		return;
	}
	out->tag = TK_NUM;
	out->data.value = val;
}

static inline const var *var_find(const var *const vars, const char *name, size_t len)
{
	if (NULL == vars)
		return NULL;
	const var *v = vars;
	while (NULL != v->name) {
		if ((strlen(v->name) == len) && (0 == memcmp(v->name, name, len)))
			return v;
		v++;
	}
	return NULL;
}

#define var_conf(v, name) var_find((v), (name), sizeof(name)-1)

static inline void next_ident(const char **const strp, tok *const out, const var *const vars)
{
	const char *s = *strp;
	size_t len = 1;
	while (isalnum(s[len]))
		len++;
	*strp = s + len;

	// search identifier in variable list
	if (NULL != vars) {
		const var *v = vars;
		while (NULL != v->name) {
			if ((strlen(v->name) == len) && (0 == memcmp(v->name, s, len))) {
				out->tag = TK_NUM;
				out->data.value = v->value;
				return;
			}
			v++;
		}
	}

	// unknown identifier means error
	out->tag = TK_ERR;

	// check for all known identifiers
	switch (len) {
#	define CASE(name, tok) if (0 == memcmp(name, s, len)) { out->tag = (tok); break; }
#	define CNST(name, val) if (0 == memcmp(name, s, len)) { out->tag = TK_NUM; out->data.value = (val); break; }
	case 1:
		CNST("e", M_E)
		break;
	case 2:
		CNST("pi", M_PI)
		break;
	case 3:
		CASE("cos", TK_FUN_COS)
		CASE("exp", TK_FUN_EXP)
		CASE("log", TK_FUN_LOG)
		CASE("max", TK_FUN_MAX)
		CASE("min", TK_FUN_MIN)
		CNST("phi", 1.61803398874989484820458683436563811772030917980576)
		CASE("sin", TK_FUN_SIN)
		CASE("sum", TK_FUN_SUM)
		CASE("tan", TK_FUN_TAN)
		break;
	case 4:
		CASE("acos", TK_FUN_ACOS)
		CASE("asin", TK_FUN_ASIN)
		CASE("atan", TK_FUN_ATAN)
		CASE("cbrt", TK_FUN_CBRT)
		CASE("ceil", TK_FUN_CEIL)
		CASE("cosh", TK_FUN_COSH)
		CASE("sinh", TK_FUN_SINH)
		CASE("sqrt", TK_FUN_SQRT)
		CASE("tanh", TK_FUN_TANH)
		break;
	case 5:
		CASE("acosh", TK_FUN_ACOSH)
		CASE("asinh", TK_FUN_ASINH)
		CASE("atanh", TK_FUN_ATANH)
		CASE("floor", TK_FUN_FLOOR)
		CASE("round", TK_FUN_ROUND)
		CASE("scale", TK_FUN_SCALE)
		break;
#	undef CASE
#	undef CNST
	}
}

static inline void next_operator(const char **const strp, tok *const out)
{
	const char *s = *strp;
	*strp = s + 1;
	switch (*s) {
	case '+': out->tag = TK_PLUS;  break;
	case '-': out->tag = TK_MINUS; break;
	case '*': out->tag = TK_MUL;   break;
	case '/': out->tag = TK_DIV;   break;
	case '^': out->tag = TK_EXP;   break;
	case '(': out->tag = TK_OPEN;  break;
	case ')': out->tag = TK_CLOSE; break;
	case ',': out->tag = TK_COMMA; break;
	default:  out->tag = TK_ERR;
	}
}

static inline void next_space(const char **const strp, tok *const out)
{
	const char *s = *strp;
	while (isspace(*s))
		s++;
	*strp = s;
	out->tag = TK_SPACE;
}

static inline void next(const char **const strp, tok *const out, const var *const vars)
{
	const char first = **strp;
	if ('\0' == first)
		out->tag = TK_EOF;
	else if (('.' == first) || isdigit(first))
		next_num(strp, out);
	else if (isalpha(first))
		next_ident(strp, out, vars);
	else if (isspace(first))
		next_space(strp, out);
	else
		next_operator(strp, out);
}

static inline size_t checkstack(size_t stacksz, size_t sp, size_t i)
{
	assert(sp < stacksz || !!! "stack overflow");
	assert(i <= sp || !!! "stack underflow");
	return sp - i;
}

static inline size_t reduce_fun(tok *const stack, const size_t stacksz, const size_t sp)
{
#	define get(i) (stack[checkstack(stacksz,sp,i)])
	checkstack(stacksz,sp,0);
	size_t ntoks = 1;
	size_t nargs = 0;
	bool expect_val = true;
	assert(0 != sp || !!! "not enough tokens for a function call");
	if (BS_IGN(TK_OPEN) == BS_IGN(get(1).tag)) {
		// empty argument list. nothing to do.
	} else {
		// verify alternating sequence of TK_VALUE and TK_COMMA, until TK_OPEN
		while ((ntoks < sp) && (BS_IGN(TK_OPEN) != BS_IGN(get(ntoks).tag))) {
			if (expect_val) {
				if (CLASS_VALUE != CLASS_GET(get(ntoks).tag))
					goto error;
				nargs++;
			} else {
				if (TK_COMMA != get(ntoks).tag)
					goto error;
			}
			expect_val = !expect_val;
			ntoks++;
		}
		// check if TK_OPEN was found, or stopped at first token
		if (expect_val || (BS_IGN(TK_OPEN) != BS_IGN(get(ntoks).tag)))
			goto error;
	}
	// check that ntoks and nargs are consistent
	assert(ntoks == nargs * 2 + !nargs || !!! "wrong function argument token sequence");
	assert(ntoks <= sp || !!! "stack underflow");

	// get first argument token and stored BS value
	tok *firstarg = &get(ntoks - 1);
	int bs = BS_GET(get(ntoks).tag);

	// search function token
	int funid = FUNID(TK_FUN_NONE);
	if ((ntoks < sp) && (CLASS_FUNC == CLASS_GET(get(ntoks + 1).tag))) {
		// skip function token as well
		ntoks++;
		funid = FUNID(get(ntoks).tag);
	}

	dbg("fcall funid=%d ntoks=%zu nargs=%zu\n", (int) funid, ntoks, nargs);

	double val;
	switch (funid) {
#	define CASE(tok, fun) case FUNID(tok): val = (fun) (nargs, firstarg); break;
	CASE(TK_FUN_NONE,  fun_identity)
	CASE(TK_FUN_ACOS,  fun_acos)
	CASE(TK_FUN_ACOSH, fun_acosh)
	CASE(TK_FUN_ASIN,  fun_asin)
	CASE(TK_FUN_ASINH, fun_asinh)
	CASE(TK_FUN_ATAN,  fun_atan)
	CASE(TK_FUN_ATANH, fun_atanh)
	CASE(TK_FUN_CBRT,  fun_cbrt)
	CASE(TK_FUN_CEIL,  fun_ceil)
	CASE(TK_FUN_COS,   fun_cos)
	CASE(TK_FUN_COSH,  fun_cosh)
	CASE(TK_FUN_EXP,   fun_exp)
	CASE(TK_FUN_FLOOR, fun_floor)
	CASE(TK_FUN_LOG,   fun_log)
	CASE(TK_FUN_MAX,   fun_max)
	CASE(TK_FUN_MIN,   fun_min)
	CASE(TK_FUN_ROUND, fun_round)
	CASE(TK_FUN_SCALE, fun_scale)
	CASE(TK_FUN_SIN,   fun_sin)
	CASE(TK_FUN_SINH,  fun_sinh)
	CASE(TK_FUN_SUM,   fun_sum)
	CASE(TK_FUN_SQRT,  fun_sqrt)
	CASE(TK_FUN_TAN,   fun_tan)
	CASE(TK_FUN_TANH,  fun_tanh)
	default:
		assert(0 || !!! "unknown function ID");
		goto error;
#	undef CASE
	}
	if (isnan(val))
		goto error;

	get(ntoks).tag = BS_SET(bs, TK_NUM);
	get(ntoks).data.value = val;
	return ntoks - 1;

error:
	get(0).tag = TK_ERR;
	return 0;
#	undef get
}

static inline size_t reduce_step(tok *const stack, const size_t stacksz, const size_t sp)
{
#	define get(i) (stack[checkstack(stacksz,sp,i)])

#	define UNARY_REDUCE(tk, op) \
		if ((tk) == get(1).tag) { \
			if (CLASS_VALUE != CLASS_GET(get(0).tag)) \
				goto error; \
			get(1).data.value = op(get(0).data.value); \
			if (2 > sp) \
				get(1).tag = BS_SET(BS_NONE, TK_NUM); \
			else if (BS_IGN(TK_OPEN) == BS_IGN(get(2).tag)) \
				get(1).tag = BS_SET(BS_OPEN, TK_NUM); \
			else \
				get(1).tag = BS_SET(BS_GET(get(2).tag), TK_NUM); \
			return 1; \
		}

#	define BINARY_REDUCE_COND(tk, cond, expr) \
		if ((tk) == get(1).tag) { \
			if ((CLASS_VALUE != CLASS_GET(get(0).tag)) || (CLASS_VALUE != CLASS_GET(get(2).tag))) \
				goto error; \
			double l = get(2).data.value; \
			double r = get(0).data.value; \
			if (!(cond)) \
				goto error; \
			get(2).data.value = (expr); \
			return 2; \
		}

#	define BINARY_REDUCE(tk, expr) BINARY_REDUCE_COND(tk, true, expr)

	assert(0 != sp || !!! "cannot reduce an empty stack");

	// check for unary operators
	if ((1 <= sp) && (CLASS_OP == CLASS_GET(get(1).tag)) && (TK_OP_IS_UNARY & get(1).tag)) {
		UNARY_REDUCE(TK_UMINUS, -)
		UNARY_REDUCE(TK_UPLUS,  +)
		assert(0 || !!! "unknown unary operator");
		goto error;
	}

	// check for binary operators
	if ((2 <= sp) && (CLASS_OP == CLASS_GET(get(1).tag))) {
		BINARY_REDUCE(TK_PLUS, l + r)
		BINARY_REDUCE(TK_MINUS, l - r)
		BINARY_REDUCE(TK_MUL, l * r)
		BINARY_REDUCE_COND(TK_DIV, 0 != r, l / r)
		BINARY_REDUCE_COND(TK_EXP, !isnan(l) && !isnan(r) && ((0 <= l) || (round(r) == r)), pow(l, r))
		// reachable if stack looks like [ ..., {TK_OPEN or TK_COMMA}, <value> ]
		goto error;
	}

#	undef UNARY_REDUCE
#	undef BINARY_REDUCE
#	undef BINARY_REDUCE_COND

	// reduction is not possible
error:
	get(0).tag = TK_ERR;
	return 0;
#	undef get
}

static inline size_t reduce(tok *const stack, const size_t stacksz, const size_t sp, const int bs)
{
	size_t delta = 0;
	while ((delta < sp) && (BS_GET(stack[sp - delta].tag) > bs)) {
		delta += reduce_step(stack, stacksz, sp - delta);
		assert(delta <= sp || !!! "attempt to make stack more than empty");
		if ((TK_ERR == stack[sp - delta].tag)) {
			stack[sp].tag = TK_ERR;
			return 0;
		}
		dbg("here: delta=%zu\n", delta);
	}
	return delta;
}

double xpr(const char *str, const var *const vars)
{
	const size_t len = strlen(str);

	// at most, each character produces a token, and the trailing 0-byte produces an EOF token
	if (len >= SIZE_MAX / sizeof(tok) - 1)
		return XPR_ERR;
	const size_t capacity = sizeof(tok) * (len + 1);

	/*
	 * This parser iterates over the input string exactly once, from left to
	 * right. It contains a stack of tokens that represent the already-parsed
	 * input string.
	 */
	tok *stack;


#if CONFIG_STACK_LIMIT == 0
	bool use_malloc = false;
#elif CONFIG_STACK_LIMIT == 1
	bool use_malloc = true;
#else
	bool use_malloc = ((size_t)(CONFIG_STACK_LIMIT) - 1) <= len;
#endif

#if CONFIG_DYNAMIC_STACK_LIMIT
	const struct xpr_var *dyn_malloc = var_conf(vars, "$malloc");
	if (dyn_malloc) {
		if (!isinf(dyn_malloc->value) && !isnan(dyn_malloc->value) && dyn_malloc->value >= 0)
			use_malloc = ((size_t)(dyn_malloc->value) - 1) <= len;
	}
#endif


	if (use_malloc)
		stack = malloc(capacity);
	else
		stack = alloca(capacity);

	if (!stack)
		goto error;

	double result = 0;

	const size_t stacksz = len + 1;
	size_t sp = 0;
#	define get(i) (stack[checkstack(stacksz,sp,i)])
#	define cur get(0)
	int bs = BS_NONE;
	while (1) {
		dbg("sp=%zu { ", sp); for (size_t i = 0; i < sp; i++) dbg_dump_tok(NULL, &stack[i], " "); dbg("}, bs=%d\n", bs);
		next(&str, &cur, vars);
		dbg_dump_tok("next", &cur, "\n");

		if (TK_ERR == cur.tag) {
			goto error;
		} else if (TK_SPACE == cur.tag) {
			// ignore space, do not increment sp. In the next iteration, the
			// next() function will overwrite the space token.
		} else if (TK_EOF == cur.tag) {
			// an empty string is an error
			if (0 == sp)
				goto error;
			sp--; // pop EOF token
			sp -= reduce(stack, stacksz, sp, BS_NONE);
			// reduction to BS_NONE enforces evaluation of all operators,
			// leaving only one value token on the stack.
			if ((0 == sp) && (CLASS_VALUE == CLASS_GET(cur.tag))) {
				result = cur.data.value;
				goto out;
			}
			goto error;
		} else if (CLASS_VALUE == CLASS_GET(cur.tag)) {
			// store the current BS in the value token
			cur.tag = BS_SET(bs, cur.tag);
			sp++;
		} else if (CLASS_FUNC == CLASS_GET(cur.tag)) {
			sp++;
		} else if (TK_OPEN == cur.tag) {
			// store current BS in TK_OPEN token, this is required for
			// evaluation of the TK_CLOSE token: After the function call, the
			// resulting VALUE token will inherit this BS.
			cur.tag = BS_SET(bs, cur.tag);
			sp++;
			bs = BS_OPEN;
		} else if (TK_CLOSE == cur.tag) {
			// TK_CLOSE must not be the first token
			if (0 == sp)
				goto error;
			// reduce last function argument
			if ((BS_IGN(TK_OPEN) != BS_IGN(get(1).tag))) {
				sp -= reduce(stack, stacksz, sp - 1, BS_OPEN);
				if (TK_ERR == cur.tag)
					goto error;
			}
			// reduce the function call
			size_t delta = reduce_fun(stack, stacksz, sp);
			sp -= delta;
			if (TK_ERR == cur.tag)
				goto error;
		} else if (TK_COMMA == cur.tag) {
			// TK_COMMA must not be the first token
			if (0 == sp)
				goto error;
			// reduce argument before comma
			size_t delta = reduce(stack, stacksz, sp - 1, BS_OPEN);
			if (delta)
				get(delta) = cur;
			sp -= delta;
			if (TK_ERR == cur.tag)
				goto error;
			// keep current token, update bs
			sp++;
			bs = BS_OPEN;
		} else if (CLASS_OP == CLASS_GET(cur.tag)) {
			// we have an operator
			if (cur.tag & TK_OP_CAN_UNARY) {
				// get(1) can be either a value, or TK_OPEN, or any other operator
				if ((0 == sp) || (CLASS_OP == CLASS_GET(get(1).tag)))
					cur.tag = TK_TO_UNARY(cur.tag); // update IS_UNARY, BS, and AS
			} else if (0 == sp) {
				goto error;
			}

			if (0 != sp) {
				// when left-associative, also reduce values with same bs
				bool left_assoc = (AS_LEFT == AS_GET(cur.tag));
				size_t delta = reduce(stack, stacksz, sp - 1, BS_GET(cur.tag) - (left_assoc ? 1 : 0));
				if (delta) {
					get(delta) = cur;
				}
				sp -= delta;
				if (TK_ERR == cur.tag)
					goto error;
			}
			bs = BS_GET(cur.tag);
			sp++;
		} else {
			assert(0 || !!! "invalid token");
		}
	}
#	undef cur
#	undef get

error:
	result = XPR_ERR;
out:
	if (use_malloc)
		free(stack);
	return result;
}

#ifdef MAIN
int main(int argc, char **argv)
{
	int nvars = 0;
	var vars[argc];
	vars[0].name = NULL;
	for (int i = 1; i < argc; i++) {
		if (strchr(argv[i], '=')) {
			char *name = strtok(argv[i], "=");
			char *val  = strtok(NULL, "=");
			if (NULL == val)
				val = "0";
			double d = strtod(val, NULL);
			vars[nvars].name = name;
			vars[nvars].value = d;
			nvars++;
			vars[nvars].name = NULL;
		} else {
			fprintf(stdout, "%lf\n", xpr(argv[i], vars));
		}
	}
	return 0;
}
#endif /* MAIN */

