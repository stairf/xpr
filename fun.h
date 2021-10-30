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

#define ARG(ap, n) (ap[2*(n)].data.value)

static inline double fun_identity(size_t nargs, const tok *ap)
{
	if (nargs != 1)
		return XPR_ERR;
	return ARG(ap, 0);
}

static inline double fun_dummy(size_t nargs, const tok *ap)
{
	printf("FUNCTION DUMMY: %zu {", nargs); for (size_t i = 0; i < nargs; i++) dbg_dump_tok(NULL, &ap[2*i], " "); printf("}\n");
	return 0;
}


#define FOLD(name, empty, expr) \
	static inline double fun_##name(size_t nargs, const tok *ap) \
	{ \
		if (0 == nargs) \
			return (empty); \
		double l = ARG(ap, 0); \
		if (isnan(l)) \
			return l; \
		for (size_t i = 1; i < nargs; i++) { \
			double r = ARG(ap, i); \
			l = (expr); \
			if (isnan(l)) \
				return l; \
		} \
		return l; \
	}

#define WRAP(name) \
	static inline double fun_##name(size_t nargs, const tok *ap) \
	{ \
		if (nargs != 1) \
			return XPR_ERR; \
		return name(ARG(ap, 0)); \
	}

FOLD(min, XPR_ERR, (l < r) ? l : r)
FOLD(max, XPR_ERR, (l > r) ? l : r)
FOLD(sum, 0, l + r);

WRAP(acos)
WRAP(asin)
WRAP(atan)
WRAP(acosh)
WRAP(asinh)
WRAP(atanh)
WRAP(ceil)
WRAP(cos)
WRAP(floor)
WRAP(round)
WRAP(sin)
WRAP(tan)
WRAP(cosh)
WRAP(sinh)
WRAP(tanh)
WRAP(sqrt)
WRAP(cbrt)
WRAP(exp)

static inline double fun_log(size_t nargs, const tok *ap)
{
	if (nargs == 1) {
		if (ARG(ap, 0) <= 0)
			return XPR_ERR;
		return log(ARG(ap, 0));
	} else if (nargs == 2) {
		if (ARG(ap, 0) <= 0 || ARG(ap, 0) == 1 || ARG(ap, 1) <= 0)
			return XPR_ERR;
		return log(ARG(ap, 1)) / log(ARG(ap, 0));
	} else {
		return XPR_ERR;
	}
}

static inline double fun_scale(size_t nargs, const tok *ap)
{
	if (3 == nargs) {
		// scale(A,B,x) translates x from scale [0,A] to [0,B]
		double da = ARG(ap, 0);
		double db = ARG(ap, 1);
		if (0 == da)
			return XPR_ERR;
		return ARG(ap, 2) / da * db;
	} else if (5 == nargs) {
		// scale(a,A,b,B,x) translates x from scale [a,A] to [b,B]
		double al = ARG(ap, 0);
		double ah = ARG(ap, 1);
		double bl = ARG(ap, 2);
		double bh = ARG(ap, 3);
		double da = ah - al;
		double db = bh - bl;
		if (0 == da)
			return XPR_ERR;
		return (ARG(ap, 4) - al) / da * db + bl;
	} else {
		return XPR_ERR;
	}
}

#undef ARG
#undef FOLD
#undef WRAP

