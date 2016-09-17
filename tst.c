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
 * tst.c
 *
 * This tool runs test cases for xpr
 *
 * Usage:
 * valgrind ./tst <test.in
 */
#ifdef MAIN

#include "xpr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

static void test_fail(char *line, unsigned long long lineno)
{
	char *realline = strtok(line, "\n");
	if (!realline)
		realline = "";
	double is = xpr(realline, NULL);
	if (!isnan(is))
		fprintf(stderr, "%llu: %s=%lf but should fail\n", lineno, realline, is);
}

#define EPS (1.0/(1<<20))

static bool equal_enough(double is, double exp, bool exact)
{
	if (is == exp)
		return true;
	if (exact)
		return false;

	if (EPS >= exp && -EPS <= exp) {
		if (is > EPS)
			return false;
		if (is < -EPS)
			return false;
		return true;
	}
	if (is > exp * (1+EPS))
		return false;
	if (is < exp * (1-EPS))
		return false;
	return true;

}

static void test_success(char *line, unsigned long long lineno)
{
	char *sep = "=";
	bool exact = true;
	if (strchr(line, '~')) {
		sep = "~";
		exact = false;
	}
	char *realline = strtok(line, "\n");
	if (!realline)
		goto syntax_error;
	char *expr = strtok(realline, sep);
	char *expect = strtok(NULL, sep);
	if (!expr || !expect)
		goto syntax_error;
	char *end;
	double exp = strtod(expect, &end);
	if (end && *end)
		goto syntax_error;
	double is = xpr(expr, NULL);
	if (!equal_enough(is, exp, exact))
		fprintf(stderr, "%llu: %s=%lf expected=%lf [%la %s %la]\n", lineno, expr, is, exp, is, exact ? "!=" : "!~", exp);
	return;
	syntax_error:
	fprintf(stderr, "%llu: syntax error. skip.\n", lineno);
}

int main(void)
{
	char *line = NULL;
	size_t linesz = 0;
	unsigned long long lineno = 0;
	while (1) {
		lineno++;
		ssize_t result = getline(&line, &linesz, stdin);
		if (result < 0)
			break;
		if (line[0] == '#' || line[0] == '\n')
			continue;
		if (line[0] == '!')
			test_fail(line + 1, lineno);
		else
			test_success(line, lineno);
	}
	if (ferror(stdin))
		die("getline");
	free(line);
	exit(EXIT_SUCCESS);
}

#undef MAIN
#include "xpr.c"

#endif /* MAIN */


