/* errHand.c  --  Error handling routines
   Copyright (C) 2012-2014, 2016 Markus Uhlin. All rights reserved.

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

   THIS SOFTWARE IS PROVIDED THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"

#include <stdio.h>
#include <string.h>		/* strerror_r() */

#include "curses-funcs.h"
#include "errHand.h"
#include "strHand.h"

static void
err_doit(bool errnoflag, int error, const char *fmt, va_list ap)
{
    char out[1300] = "";

#if defined(UNIX)
    vsnprintf(out, sizeof out, fmt, ap);
#elif defined(WIN32)
    vsnprintf_s(out, sizeof out, _TRUNCATE, fmt, ap);
#endif

    if (errnoflag) {
	sw_strcat(out, ": ", sizeof out);
	sw_strcat(out, errdesc_by_num(error), sizeof out);
    }

    escape_curses();
    fputs(out, stderr);
    fputc('\n', stderr);
}

SW_NORET void
err_dump(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(true, errno, fmt, ap);
    va_end(ap);

    abort();
}

SW_NORET void
err_exit(int error, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(true, error, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

SW_NORET void
err_quit(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(false, 0, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

SW_NORET void
err_sys(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(true, errno, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

void
err_msg(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(false, 0, fmt, ap);
    va_end(ap);
}

void
err_ret(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(true, errno, fmt, ap);
    va_end(ap);
}

const char *
errdesc_by_num(int num)
{
    static char desc[600];

    BZERO(desc, sizeof desc);

#ifdef HAVE_BCI
    if (num == 0 || strerror_s(desc, sizeof desc, num) != 0)
#else
    if (num == 0 || strerror_r(num, desc, sizeof desc) != 0)
#endif
	{
	    return ("Unknown error!");
	}

    return (trim(desc));
}
