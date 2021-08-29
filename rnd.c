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
 * rnd.c
 *
 * This tool calls the xpr function with randomized input strings.
 *
 * Usage:
 * valgrind ./rnd 2>&1 | grep -b1 ^=
 */
#ifdef MAIN

#include "xpr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>


static char *tkns[] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"0.1", "3.2", "99.54", "34.987",
	"4e3", "5E2", "9E1", "1e0",
	"5.e2", "1.e2", "2.e9",
	"0.6e3", "0.5e-2", "0.4e2", "0.3E0",
	"7e-2", "6E-3", "5e-10",
	"2e", "eE"
	"pi",
	"+", "+", "+", "-", "-", "-", "*", "*", "*",  "/", "/", "/", "^", "^", "^",
	"(", "(", ",", ")", ")",
	"acos", "acosh", "asin", "asinh", "atan", "atanh", "cbrt", "ceil", "cos", "cosh", "exp", "floor", "log", "min", "max", "round", "sin", "sinh", "sqrt", "tan", "tanh",
	"foo"
};

#define NTOKENS (sizeof(tkns)/sizeof(tkns[0]))

int main(int argc, char **argv)
{
	int seed = 0;
	if (argc > 2) {
		fprintf(stderr, "usage: %s [seed]\n", argv[0]);
		exit(EXIT_FAILURE);
	} else if (argc == 2) {
		seed = atoi(argv[1]);
	} else {
		seed = time(NULL);
	}
	fprintf(stderr, "seed: %d\n", seed);
	srandom(seed);
	while (1) {
		size_t n = random() & 0xff ?: 1;
		char buf[n*8+1];
		char *str = buf;
		*str = '\0';
		for (size_t i = 0; i < n; i++) {
			str = stpcpy(str, " ");
			str = stpcpy(str, tkns[random() % NTOKENS]);
		}
		alarm(1);
		double d = xpr(buf, NULL);
		alarm(0);
		//if (!isnan(d))
			fprintf(stderr, "%-20lf %s\n", d, buf);
		(void) d;
	}
	return 0;
}

#undef MAIN
#include "xpr.c"

#endif /* MAIN */
