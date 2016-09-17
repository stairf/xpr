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
 * fuzzme.c
 *
 * This tool passes stdin directly to xpr.
 *
 * Usage:
 * ./fuzzme <input
 * afl-fuzz -i input-dir -o findings-dir ./fuzzme
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
		printf("%llu: %lf\n", lineno, xpr(line, NULL));
	}
	if (ferror(stdin))
		die("getline");
	free(line);
	exit(EXIT_SUCCESS);
}

#undef MAIN
#include "xpr.c"

#endif /* MAIN */


