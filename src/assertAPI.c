/* Swirc assert API!
   Copyright (C) 2012-2021 Markus Uhlin. All rights reserved.

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

#include "assertAPI.h"
#include "curses-funcs.h"
#include "errHand.h"

enum { DNOFILE, DNOFN, DNOASSERTION };

static const char *descriptions[] = {
	[DNOFILE]	= "<unknown file>",
	[DNOFN]		= "<unknown function>",
	[DNOASSERTION]	= "<unknown assertion>",
};

/*lint -printf(1, assert_doit) */

static void
assert_doit(const char *fmt, ...)
{
	char out[1000] = { '\0' };
	va_list ap;

	va_start(ap, fmt);
#if defined(UNIX)
	(void) vsnprintf(out, sizeof out, fmt, ap);
#elif defined(WIN32)
	(void) vsnprintf_s(out, sizeof out, _TRUNCATE, fmt, ap);
#endif
	va_end(ap);

	escape_curses();
	(void) fputs(out, stderr);
	(void) fputc('\n', stderr);
}

NORETURN void
SWAssertFail(const char *file, long int line, const char *fn,
    const char *assertion)
{
	if (file == NULL || *file == '\0')
		file = descriptions[DNOFILE];
	if (fn == NULL || *fn == '\0')
		fn = descriptions[DNOFN];
	if (assertion == NULL || *assertion == '\0')
		assertion = descriptions[DNOASSERTION];
	assert_doit("%s:%ld: %s: Assertion `%s' failed", file, line, fn,
	    assertion);
	abort();
}

NORETURN void
SWAssertPerrorFail(const char *file, long int line, const char *fn, int errnum)
{
	char	strerrbuf[MAXERROR] = { '\0' };

	if (file == NULL || *file == '\0')
		file = descriptions[DNOFILE];
	if (fn == NULL || *fn == '\0')
		fn = descriptions[DNOFN];
	assert_doit("%s:%ld: %s: Unexpected error: %s", file, line, fn,
	    xstrerror(errnum, strerrbuf, MAXERROR));
	abort();
}

NORETURN void
SWAssertNotReachedFail(const char *file, long int line, const char *fn)
{
	if (file == NULL || *file == '\0')
		file = descriptions[DNOFILE];
	if (fn == NULL || *fn == '\0')
		fn = descriptions[DNOFN];
	assert_doit("%s:%ld: %s: Should not be reached!", file, line, fn);
	abort();
}
