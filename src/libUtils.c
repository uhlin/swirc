/* libUtils.c  --  Library Utilities
   Copyright (C) 2012-2019 Markus Uhlin. All rights reserved.

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

#include <stdint.h>
#include <string.h>
#include <time.h>

#include "curses-funcs.h"
#include "dataClassify.h"
#include "errHand.h"
#include "libUtils.h"
#include "strHand.h"

#if defined(UNIX)
#define PRINT_SZ	"%zu"
#elif defined(WIN32)
#define PRINT_SZ	"%Iu"
#endif

FILE *
fopen_exit_on_error(const char *path, const char *mode)
{
    FILE *fp;

    if (path == NULL || mode == NULL) {
	err_exit(EINVAL, "fopen_exit_on_error");
    }

#ifdef HAVE_BCI
    if ((errno = fopen_s(&fp, path, mode)) != 0) {
	err_sys("fopen_s");
    }
#else
    if ((fp = fopen(path, mode)) == NULL) {
	err_sys("fopen");
    }
#endif

    return (fp);
}

FILE *
xfopen(const char *path, const char *mode)
{
    FILE *fp = NULL;

    if (path == NULL || mode == NULL) {
	errno = EINVAL;
	return NULL;
    }
#ifdef HAVE_BCI
    if ((errno = fopen_s(&fp, path, mode)) != 0)
	return NULL;
#else
    if ((fp = fopen(path, mode)) == NULL)
	return NULL;
#endif
    return fp;
}

/* Check for vulnerabilities */
static bool
format_codes_are_ok(const char *fmt)
{
    const char *ccp;
    const char  legal_index[] = "aAbBcdHIjmMpSUwWxXyYzZ%";
    ptrdiff_t   diff          = 0;

    while (ccp = strchr(&fmt[diff], '%'), ccp != NULL) {
	if (isEmpty(++ccp) || strchr(legal_index, *ccp) == NULL) {
	    return false;
	} else {
	    diff = ++ccp - fmt;
	}
    }

    return true;
}

const char *
current_time(const char *fmt)
{
    time_t		seconds;
    const time_t	unsuccessful = -1;
    struct tm		items        = {0};
    static char		buffer[200]  = "";

    if (isNull(fmt) || isEmpty(fmt) || time(&seconds) == unsuccessful) {
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

    if (!format_codes_are_ok(fmt)) {
	err_msg("In current_time: Erroneous format codes. Aborting...");
	abort();
    }

    return (strftime(buffer, sizeof buffer, fmt, &items) > 0 ? &buffer[0] : "");
}

/* Return the difference of 'a - b' */
int
int_diff(const int a, const int b)
{
    if ((b > 0 && a < INT_MIN + b) || (b < 0 && a > INT_MAX + b)) {
	err_msg("int_diff: Integer overflow: a=%d b=%d", a, b);
	abort();
    }

    return (a - b);
}

/* Return the sum of 'a + b' */
int
int_sum(const int a, const int b)
{
    if ((b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b)) {
	err_msg("int_sum: Integer overflow: a=%d b=%d", a, b);
	abort();
    }

    return (a + b);
}

int
size_to_int(const size_t value)
{
    if (value > INT_MAX) {
	errno = ERANGE;
	err_dump("fatal: size_to_int: loss of data");
    }

    return ((int) value);
}

/* Return 'elt_count' elements of size 'elt_size' (elt_count * elt_size)
   -- but check for overflow... */
size_t
size_product(const size_t elt_count, const size_t elt_size)
{
    if (elt_size && elt_count > SIZE_MAX / elt_size) {
	err_msg("Integer overflow");
	abort();
    }

    return (elt_count * elt_size);
}

size_t
xmbstowcs(wchar_t *pwcs, const char *s, size_t n)
{
    size_t bytes_convert = 0;
    static const size_t CONVERT_FAILED = (size_t) -1;

    if (s == NULL) {
	errno = EINVAL;
	return (CONVERT_FAILED);
    }
#if defined(UNIX)
    bytes_convert = mbstowcs(pwcs, s, n);
    return (bytes_convert);
#elif defined(WIN32)
    if ((errno = mbstowcs_s(&bytes_convert, pwcs, n + 1, s, n)) != 0)
	return (CONVERT_FAILED);
    return (bytes_convert);
#endif
}

void
fclose_ensure_success(FILE *fp)
{
    if (fp != NULL && fclose(fp) != 0) {
	err_sys("fclose");
    }
}

void
realloc_strcat(char **dest, const char *src)
{
    size_t newsize = 0;

    if (isNull(dest) || isNull(*dest) || isNull(src))
	err_exit(EINVAL, "realloc_strcat");
    else
	newsize = strlen(*dest) + strlen(src) + 1;
    if ((*dest = realloc(*dest, newsize)) == NULL)
	err_exit(ENOMEM, "realloc_strcat");
    if ((errno = sw_strcat(*dest, src, newsize)) != 0)
	err_sys("realloc_strcat");
}

void
write_setting(FILE *stream, const char *name, const char *value,
	      const bool do_padding_using_tabs, const short int count)
{
    fputs(name, stream);
    const char c = do_padding_using_tabs ? '\t' : ' ';
    for (short int si = 0; si < count; si++)
	fputc(c, stream);
    fprintf(stream, "= \"%s\";\n", value);
}

void
write_to_stream(FILE *stream, const char *fmt, ...)
{
    va_list	ap;
    int		n_print;

    va_start(ap, fmt);
    errno = 0;
#ifdef HAVE_BCI
    n_print = vfprintf_s(stream, fmt, ap);
#else
    n_print = vfprintf(stream, fmt, ap);
#endif
    va_end(ap);

    if (n_print < 0) {
#ifdef HAVE_BCI
	const char fn[] = "vfprintf_s()";
#else
	const char fn[] = "vfprintf()";
#endif

	if (errno != 0) {
	    err_ret("write_to_stream: %s returned %d", fn, n_print);
	} else {
	    err_msg("write_to_stream: %s returned %d (error)", fn, n_print);
	}

	abort();
    }
}

void *
xcalloc(size_t elt_count, size_t elt_size)
{
    void *vp;

    if (elt_count == 0) {
	err_exit(EINVAL, "xcalloc: invalid argument: element count is zero");
    } else if (elt_size == 0) {
	err_exit(EINVAL, "xcalloc: invalid argument: element size is zero");
    } else if (SIZE_MAX / elt_count < elt_size) {
	err_quit("xcalloc: integer overflow");
    } else {
	if ((vp = calloc(elt_count, elt_size)) == NULL)
	    err_exit(ENOMEM,
		"xcalloc: out of memory (allocating " PRINT_SZ " bytes)",
		(elt_count * elt_size));
    }

    return (vp);
}

void *
xmalloc(size_t size)
{
    void *vp;

    if (size == 0) {
	err_exit(EINVAL, "xmalloc: invalid argument -- zero size");
    }

    if ((vp = malloc(size)) == NULL) {
	err_exit(ENOMEM, "xmalloc: error allocating " PRINT_SZ " bytes", size);
    }

    return (vp);
}

void *
xrealloc(void *ptr, size_t newSize)
{
    void *newPtr;

    if (ptr == NULL) {
	err_exit(EINVAL, "xrealloc: invalid argument: "
	    "a null pointer was passed");
    } else if (newSize == 0) {
	err_exit(EINVAL, "xrealloc: invalid argument: zero size  --  use free");
    } else {
	if ((newPtr = realloc(ptr, newSize)) == NULL)
	    err_exit(errno,
		"xrealloc: error changing memory block to " PRINT_SZ " bytes",
		newSize);
    }

    return (newPtr);
}
