/* Interpret command-line options
   Copyright (C) 2013 Markus Uhlin. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   - Neither the name of the author nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"

#include <stdio.h>
#include <string.h>

#include "options.h"

#define SELF_TEST 0

int	 g_option_index = 1;
int	 g_option_save  = -1;
char	*g_option_arg   = NULL;

int
options(int argc, char *argv[], const char *optstring)
{
	const char	*p;
	int		 opt;
	static char	*nextchar = "";

	if (g_option_index >= argc ||
	    (!*nextchar && *(nextchar = argv[g_option_index]) != '-')) {
		nextchar = "";
		return EOF;
	} else if (*nextchar == '-' && *++nextchar == '-') {
		nextchar = "";
		return EOF;
	}

	g_option_save = opt = *nextchar++;

	if ((p = strchr(optstring, opt)) == NULL) {
		if (!*nextchar)
			g_option_index++;
		return UNRECOGNIZED_OPTION;
	} else if (*++p != ':') {
		if (!*nextchar)
			g_option_index++;
		g_option_arg = NULL;
	} else {
		if (*nextchar) {
			g_option_arg = nextchar;
		} else {
			if (++g_option_index >= argc) {
				nextchar = "";
				return OPTION_ARG_MISSING;
			} else {
				g_option_arg = argv[g_option_index];
			}
		}

		nextchar = "";
		g_option_index++;
	}

	return opt;
}

#if SELF_TEST
int
main(int argc, char *argv[])
{
	const char	optstring[] = "a:b:cd";
	int		opt;

	while ((opt = options(argc, argv, optstring)) != EOF) {
		switch (opt) {
		case 'a':
			printf("Catched -%c\ng_option_arg holds \"%s\"\n", opt,
			    g_option_arg);
			break;
		case 'b':
			printf("Catched -%c\ng_option_arg holds \"%s\"\n", opt,
			    g_option_arg);
			break;
		case 'c':
			printf("Catched -%c (no arg)\n", opt);
			break;
		case 'd':
			printf("Catched -%c (no arg)\n", opt);
			break;
		case UNRECOGNIZED_OPTION:
			printf("%s: -%c: unrecognized option\n", argv[0],
			    g_option_save);
			return EXIT_FAILURE;
		case OPTION_ARG_MISSING:
			printf("%s: -%c: option argument missing\n", argv[0],
			    g_option_save);
			return EXIT_FAILURE;
		}
	}

	puts("Done processing, exiting.");
	return EXIT_SUCCESS;
}
#endif
