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
 * xpr.h
 *
 * XPR is a KISS library to evaluate arithmetic expressions. It uses double
 * precision floating point numbers for all calculations.
 */
#ifndef __XPR_H__
#define __XPR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <math.h>

/*
 * error code for the xpr() function
 */
#define XPR_ERR NAN

/*
 * data structure for XPR variables
 *
 * an XPR variable maps an identifier to a double value. The identifier must
 *   start with an alphabetic character, followed by any number of alphanumeric
 *   characters: [a-zA-Z][a-zA-Z0-9]+
 */
struct xpr_var {
	const char *name;
	double value;
};

/*
 * evaluate an arithmetic expression
 *
 * params:
 *    expr  The expression to evaluate, as a null-terminated string
 *    vars  An array of variables, terminated by an entry with the name NULL.
 *          This parameter can be NULL, which is equivalent to an empty list.
 *
 * returns:
 *          The result of the given expression, evaluated using the data type
 *          double. On error, the function returns XPR_ERR, which is NAN.
 */
extern double xpr(const char *expr, const struct xpr_var *vars);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* __XPR_H__ */
