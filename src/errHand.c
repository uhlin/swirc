/* errHand.c  --  Error handling routines
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
#include <string.h>		/* strerror_r() */
#include <time.h>
#ifdef WIN32
#include <windows.h>		/* MessageBox() */
#endif

#include "assertAPI.h"
#include "curses-funcs.h"
#include "errHand.h"
#include "libUtils.h"
#include "main.h"
#include "nestHome.h"
#include "strHand.h"

static const char *
get_timestamp(void)
{
    static char		buffer[200] = "";
    struct tm		items = { 0 };
    time_t		seconds = 0;

    if (time(&seconds) == g_time_error) {
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

    return ((strftime(buffer, sizeof buffer, "%c", &items) > 0)
	    ? &buffer[0]
	    : "");
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
	(void) fprintf(fp, "%s %s[%ld]: %s\n",
	    get_timestamp(), g_progname, g_pid, msg);
	(void) fclose(fp);
    }
}

static void
err_doit(bool output_to_stderr, int error, const char *fmt, va_list ap)
{
    char out[1300] = "";
    char strerrbuf[MAXERROR] = "";

#if defined(UNIX)
    (void) vsnprintf(out, sizeof out, fmt, ap);
#elif defined(WIN32)
    (void) vsnprintf_s(out, sizeof out, _TRUNCATE, fmt, ap);
#endif

    if (error) {
	(void) sw_strcat(out, ": ", sizeof out);
	(void) sw_strcat(out, xstrerror(error, strerrbuf, MAXERROR),
	    sizeof out);
    }

    write_to_error_log(out);

    if (output_to_stderr) {
	escape_curses();

	(void) fputs(out, stderr);
	(void) fputc('\n', stderr);

#ifdef WIN32
	(void) MessageBox(NULL, out, "Fatal",
	    (MB_OK | MB_ICONSTOP | MB_DEFBUTTON1));
#endif
    }
}

NORETURN void
err_dump(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(true, errno, fmt, ap);
    va_end(ap);

    abort();
}

NORETURN void
err_exit(int error, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(true, error, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

NORETURN void
err_quit(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(true, 0, fmt, ap);
	va_end(ap);

	exit(EXIT_FAILURE);
}

NORETURN void
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
errdesc_by_last_err(void)
{
#if defined(UNIX)
	return ("Unknown error!");
#elif defined(WIN32)
	TCHAR		lpBuffer[MAXERROR];
	const DWORD	dwFlags = (FORMAT_MESSAGE_FROM_SYSTEM |
				   FORMAT_MESSAGE_IGNORE_INSERTS);
	const DWORD	dwLanguageId = MAKELANGID(LANG_NEUTRAL,
						  SUBLANG_SYS_DEFAULT);
	const DWORD	dwMessageId = GetLastError();
	static char	desc[MAXERROR];

	BZERO(lpBuffer, ARRAY_SIZE(lpBuffer));

	if (sizeof(TCHAR) != 1 || !FormatMessage(dwFlags, NULL, dwMessageId,
	    dwLanguageId, addrof(lpBuffer[0]), ARRAY_SIZE(lpBuffer), NULL))
		return ("Unknown error!");

	sw_static_assert(sizeof(TCHAR) == 1, "TCHAR unexpectedly large. "
	    "UNICODE defined?");

	(void) memcpy(desc, lpBuffer, ARRAY_SIZE(desc));

	return addrof(desc[0]);
#endif
}

const char *
errdesc_by_num(int num)
{
	static char	desc[600] = { '\0' };

#ifdef HAVE_BCI
	if (num == 0 || strerror_s(desc, sizeof desc, num) != 0)
		return ("Unknown error!");
#else
	if (num == 0 || strerror_r(num, desc, sizeof desc) != 0)
		return ("Unknown error!");
#endif

	return (trim(desc));
}

const char *
xstrerror(int errnum, char *strerrbuf, size_t buflen)
{
#ifdef HAVE_BCI
	if (errnum == 0 || strerror_s(strerrbuf, buflen, errnum) != 0)
		return ("Unknown error!");
#else
	if (errnum == 0 || strerror_r(errnum, strerrbuf, buflen) != 0)
		return ("Unknown error!");
#endif

	return (trim(strerrbuf));
}

/* ----------------------------------------------------------------- */

static void
debug_doit(const char *fmt, va_list ap)
{
	FILE	*fp;
	char	 out[1300] = { '\0' };
	char	 path[1300] = { '\0' };

#if defined(UNIX)
	(void) vsnprintf(out, ARRAY_SIZE(out), fmt, ap);
#elif defined(WIN32)
	(void) vsnprintf_s(out, ARRAY_SIZE(out), _TRUNCATE, fmt, ap);
#endif

	if (g_log_dir == NULL ||
	    sw_strcpy(path, g_log_dir, ARRAY_SIZE(path)) != 0)
		return;

#if defined(UNIX)
	if (sw_strcat(path, "/debug.log", ARRAY_SIZE(path)) != 0)
		return;
#elif defined(WIN32)
	if (sw_strcat(path, "\\debug.log", ARRAY_SIZE(path)) != 0)
		return;
#endif

	if ((fp = xfopen(path, "a")) != NULL) {
		(void) fprintf(fp, "%s %s[%ld]: %s\n", get_timestamp(),
		    g_progname, g_pid, out);
		(void) fclose(fp);
	}
}

void
debug(const char *fmt, ...)
{
	if (g_debug_logging) {
		va_list ap;

		va_start(ap, fmt);
		debug_doit(fmt, ap);
		va_end(ap);
	}
}
