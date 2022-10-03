/* libUtils.c  --  Library Utilities
   Copyright (C) 2012-2022 Markus Uhlin. All rights reserved.

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

#include "assertAPI.h"
#include "curses-funcs.h"
#include "dataClassify.h"
#include "errHand.h"
#include "libUtils.h"
#include "strHand.h"

#if defined(UNIX)
#define PRINT_SIZE	"%zu"
#elif defined(WIN32)
#define PRINT_SIZE	"%Iu"
#endif

const char	g_alphabet_upcase[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char	g_alphabet_downcase[] = "abcdefghijklmnopqrstuvwxyz";

const size_t	g_conversion_failed = ((size_t) -1);
const time_t	g_time_error = ((time_t) -1);

/* Check for vulnerabilities */
static bool
format_codes_are_ok(const char *fmt)
{
	const char *cp = NULL;
	ptrdiff_t diff = 0;
	static const char legal_index[] = "aAbBcdHIjmMpSUwWxXyYzZ%";

	while ((cp = strchr(&fmt[diff], '%')) != NULL) {
		if (isEmpty(++cp) || strchr(legal_index, *cp) == NULL)
			return false;
		else
			diff = (++cp - fmt);
	}
	return true;
}

FILE *
fopen_exit_on_error(const char *path, const char *mode)
{
	FILE	*fp;

	if (path == NULL || mode == NULL)
		err_exit(EINVAL, "fopen_exit_on_error");

#ifdef HAVE_BCI
	if ((errno = fopen_s(&fp, path, mode)) != 0)
		err_sys("fopen_s");
#else
	if ((fp = fopen(path, mode)) == NULL)
		err_sys("fopen");
#endif

	return (fp);
}

FILE *
xfopen(const char *path, const char *mode)
{
	FILE	*fp = NULL;

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

bool
bool_false(const char *str)
{
	if (strings_match_ignore_case(str, "off") ||
	    strings_match_ignore_case(str, "false") ||
	    strings_match_ignore_case(str, "no"))
		return true;
	return false;
}

bool
bool_true(const char *str)
{
	if (strings_match_ignore_case(str, "on") ||
	    strings_match_ignore_case(str, "true") ||
	    strings_match_ignore_case(str, "yes"))
		return true;
	return false;
}

bool
getval_strtol(const char *str, const long int lo, const long int hi,
    long int *val)
{
	if (!is_numeric(str)) {
		*val = 0;
		return false;
	}

	errno = 0;
	*val = strtol(str, NULL, 10);

	if (errno != 0 || (*val < lo || *val > hi)) {
		*val = 0;
		return false;
	}

	return true;
}

bool
time_format_ok(const char *fmt)
{
	return format_codes_are_ok(fmt);
}

char
rot13_byte(char b)
{
	char bs[2];
	size_t x;
	static const size_t upper_bound = 26;

	sw_static_assert(ARRAY_SIZE(g_alphabet_upcase) == 27,
	    "g_alphabet_upcase: sizes mismatch");
	sw_static_assert(ARRAY_SIZE(g_alphabet_downcase) == 27,
	    "g_alphabet_downcase: sizes mismatch");

	if (b >= 'A' && b <= 'Z') {
		bs[0] = b;
		bs[1] = '\0';
		x = strcspn(addrof(g_alphabet_upcase[0]), addrof(bs[0]));
		x += 13;
		return g_alphabet_upcase[x % upper_bound];
	} else if (b >= 'a' && b <= 'z') {
		bs[0] = b;
		bs[1] = '\0';
		x = strcspn(addrof(g_alphabet_downcase[0]), addrof(bs[0]));
		x += 13;
		return g_alphabet_downcase[x % upper_bound];
	}
	return b;
}

char *
rot13_str(char *str)
{
	for (char *cp = addrof(str[0]); *cp != '\0'; cp++)
		*cp = rot13_byte(*cp);
	return str;
}

const char *
current_time(const char *fmt)
{
	static char	buffer[200] = { 0 };
	struct tm	items = { 0 };
	time_t		seconds = 0;

	if (isNull(fmt) || isEmpty(fmt) || time(&seconds) == g_time_error)
		return "";

#if defined(UNIX)
	if (localtime_r(&seconds, &items) == NULL)
		return "";
#elif defined(WIN32)
	if (localtime_s(&items, &seconds) != 0)
		return "";
#endif

	return ((strftime(buffer, ARRAY_SIZE(buffer), fmt, &items) > 0)
	    ? &buffer[0] : "");
}

const char *
getuser(void)
{
	char *var_data;
	static char buf[100] = { '\0' };
	static const char var[] =
#if defined(UNIX)
	    "USER";
#elif defined(WIN32)
	    "USERNAME";
#endif

	if ((var_data = getenv(var)) == NULL ||
	    sw_strcpy(buf, var_data, ARRAY_SIZE(buf)) != 0)
		return "";

	buf[strcspn(buf, " \f\n\r\t\v")] = '\0';
	return addrof(buf[0]);
}

/* Return the difference of 'a - b' */
int
int_diff(const int a, const int b)
{
	if ((b > 0 && a < INT_MIN + b) || (b < 0 && a > INT_MAX + b)) {
		err_msg("%s: Integer overflow: a=%d b=%d", __func__, a, b);
		abort();
	}
	return (a - b);
}

/* Return the sum of 'a + b' */
int
int_sum(const int a, const int b)
{
	if ((b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b)) {
		err_msg("%s: Integer overflow: a=%d b=%d", __func__, a, b);
		abort();
	}
	return (a + b);
}

int
size_to_int(const size_t value)
{
	if (value > INT_MAX) {
		errno = ERANGE;
		err_dump("fatal: %s: loss of data", __func__);
	}
	return ((int) value);
}

/* Return 'elt_count' elements of size 'elt_size' (elt_count * elt_size)
   -- but check for overflow... */
size_t
size_product(const size_t elt_count, const size_t elt_size)
{
	if (elt_size && elt_count > SIZE_MAX / elt_size) {
		err_msg("%s: integer overflow", __func__);
		abort();
	}
	return (elt_count * elt_size);
}

size_t
xmbstowcs(wchar_t *pwcs, const char *s, size_t n)
{
	size_t	bytes_convert = 0;

	if (s == NULL) {
		errno = EINVAL;
		return (g_conversion_failed);
	}

#if defined(UNIX)
	bytes_convert = mbstowcs(pwcs, s, n);

	return (bytes_convert);
#elif defined(WIN32)
	if ((errno = mbstowcs_s(&bytes_convert, pwcs, n + 1, s, n)) != 0)
		return (g_conversion_failed);

	return (bytes_convert);
#endif
}

unsigned int
hash_djb_g(const char *str, const bool lc, const size_t upper_bound)
{
#define MAGIC_NUMBER 5381
	char *str_copy, *cp;
	char c;
	unsigned int hashval;

	str_copy	= (lc ? strToLower(sw_strdup(str)) : sw_strdup(str));
	cp		= &str_copy[0];
	hashval		= MAGIC_NUMBER;

	while ((c = *cp++) != '\0')
		hashval = ((hashval << 5) + hashval) + c;

	free(str_copy);
	return (hashval % upper_bound);
}

unsigned int
hash_pjw_g(const char *str, const bool lc, const size_t upper_bound)
{
	char *str_copy, *cp;
	char c;
	unsigned int hashval;

	str_copy	= (lc ? strToLower(sw_strdup(str)) : sw_strdup(str));
	cp		= &str_copy[0];
	hashval		= 0;

	while ((c = *cp++) != '\0') {
		unsigned int tmp;

		hashval = (hashval << 4) + c;
		tmp = (hashval & 0xf0000000);

		if (tmp) {
			hashval ^= (tmp >> 24);
			hashval ^= tmp;
		}
	}

	free(str_copy);
	return (hashval % upper_bound);
}

void
fclose_ensure_success(FILE *fp)
{
	if (fp != NULL && fclose(fp) != 0)
		err_sys("fclose");
}

void
realloc_strcat(char **dest, const char *src)
{
	size_t	newsize = 0;

	if (isNull(dest) || isNull(*dest) || isNull(src))
		err_exit(EINVAL, "realloc_strcat");
	else
		newsize = strlen(*dest) + strlen(src) + 1;

	if ((*dest = realloc(*dest, newsize)) == NULL)
		err_exit(ENOMEM, "realloc_strcat");
	else if ((errno = sw_strcat(*dest, src, newsize)) != 0)
		err_sys("realloc_strcat");
}

void
write_setting(FILE *stream, const char *name, const char *value,
    const bool do_padding_using_tabs, const short int count)
{
	const char	c = do_padding_using_tabs ? '\t' : ' ';

	(void) fputs(name, stream);
	for (short int si = 0; si < count; si++)
		(void) fputc(c, stream);
	(void) fprintf(stream, "= \"%s\";\n", value);
}

void
write_to_stream(FILE *stream, const char *fmt, ...)
{
	int n_print;
	va_list ap;

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
			err_ret("%s: %s returned %d", __func__, fn, n_print);
		} else {
			err_msg("%s: %s returned %d (error)", __func__, fn,
			    n_print);
		}

		abort();
	}
}

void *
xcalloc(size_t elt_count, size_t elt_size)
{
	void	*vp;

	if (elt_count == 0) {
		err_exit(EINVAL, "%s: invalid argument: element count is zero",
		    __func__);
	} else if (elt_size == 0) {
		err_exit(EINVAL, "%s: invalid argument: element size is zero",
		    __func__);
	} else if (SIZE_MAX / elt_count < elt_size) {
		err_quit("%s: integer overflow", __func__);
	} else if ((vp = calloc(elt_count, elt_size)) == NULL) {
		err_exit(ENOMEM, "%s: out of memory "
		    "(allocating " PRINT_SIZE " bytes)", __func__,
		    (elt_count * elt_size));
	}
	return (vp);
}

void *
xmalloc(size_t size)
{
	void	*vp;

	if (size == 0) {
		err_exit(EINVAL, "%s: invalid argument -- zero size", __func__);
	} else if ((vp = malloc(size)) == NULL) {
		err_exit(ENOMEM, "%s: error allocating " PRINT_SIZE " bytes",
		    __func__, size);
	}
	return (vp);
}

void *
xrealloc(void *ptr, size_t newSize)
{
	void	*newPtr;

	if (ptr == NULL) {
		err_exit(EINVAL, "%s: invalid argument: "
		    "a null pointer was passed", __func__);
	} else if (newSize == 0) {
		err_exit(EINVAL, "%s: invalid argument: zero size  --  "
		    "use free", __func__);
	} else if ((newPtr = realloc(ptr, newSize)) == NULL) {
		err_exit(errno, "%s: error changing memory block to "
		    PRINT_SIZE " bytes", __func__, newSize);
	}
	return (newPtr);
}
