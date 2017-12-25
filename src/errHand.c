/* errHand.c  --  Error handling routines
   Copyright (C) 2012-2017 Markus Uhlin. All rights reserved.

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
#include <time.h>
#ifdef WIN32
#include <windows.h>		/* MessageBox() */
#endif

#include "curses-funcs.h"
#include "errHand.h"
#include "libUtils.h"
#include "nestHome.h"
#include "strHand.h"

static const char *
get_timestamp()
{
    time_t       seconds;
    const time_t unsuccessful = -1;
    struct tm    items        = {0};
    static char  buffer[200]  = "";

    if (time(&seconds) == unsuccessful) {
	return "";
    }

#if defined(UNIX)
    if (localtime_r(&seconds, &items) == NULL) {
	return "";
    }
#elif defined(WIN32)
    if (localtime_s(&items, &seconds) != 0) {
	return "";
    }
#endif

    return (strftime(buffer, sizeof buffer, "%c", &items) > 0 ? &buffer[0] : "");
}

static void
write_to_error_log(const char *msg)
{
    char  path[1300] = "";
    FILE *fp         = NULL;

    if (!msg || !g_log_dir || sw_strcpy(path, g_log_dir, sizeof path) != 0)
	return;

#if defined(UNIX)
    if (sw_strcat(path, "/error.log", sizeof path) != 0)
	return;
#elif defined(WIN32)
    if (sw_strcat(path, "\\error.log", sizeof path) != 0)
	return;
#endif

    if ((fp = xfopen(path, "a")) != NULL) {
	(void) fprintf(fp, "%s %s\n", get_timestamp(), msg);
	(void) fclose(fp);
    }
}

static void
err_doit(bool output_to_stderr, int error, const char *fmt, va_list ap)
{
    char out[1300] = "";

#if defined(UNIX)
    vsnprintf(out, sizeof out, fmt, ap);
#elif defined(WIN32)
    vsnprintf_s(out, sizeof out, _TRUNCATE, fmt, ap);
#endif

    if (error) {
	sw_strcat(out, ": ", sizeof out);
	sw_strcat(out, errdesc_by_num(error), sizeof out);
    }

    write_to_error_log(out);

    if (output_to_stderr) {
	escape_curses();
	fputs(out, stderr);
	fputc('\n', stderr);
#ifdef WIN32
	MessageBox(NULL, out, "Fatal", MB_OK | MB_ICONSTOP | MB_DEFBUTTON1);
#endif
    }
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
    err_doit(true, 0, fmt, ap);
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
err_log(int error, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(false, error, fmt, ap);
    va_end(ap);
}

void
err_msg(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(true, 0, fmt, ap);
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
