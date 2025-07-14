/* String handling functions
   Copyright (C) 2012-2025 Markus Uhlin. All rights reserved.

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

#if defined(CURSES_HDR)
#include CURSES_HDR
#elif UNIX
#include <curses.h>
#elif WIN32
#include "pdcurses/curses.h"
#else
#error Cannot determine Curses header file!
#endif

#include "dataClassify.h"
#include "errHand.h"
#include "strHand.h"

const int STRINGS_MATCH = 0;

/**
 * Convert string to lowercase
 *
 * @param s String to convert
 * @return The converted string
 */
char *
strToLower(char *s)
{
	size_t	len = 0;

	if (s == NULL)
		err_exit(EINVAL, "strToLower");
	else if (! (*s))
		return s;
	else
		len = strlen(s);
	for (char *p = &s[0]; p < &s[len]; p++) {
		if (sw_isupper(*p))
			*p = tolower(*p);
	}
	return s;
}

/**
 * Convert string to uppercase
 *
 * @param s String to convert
 * @return The converted string
 */
char *
strToUpper(char *s)
{
	size_t	len = 0;

	if (s == NULL)
		err_exit(EINVAL, "strToUpper");
	else if (! (*s))
		return s;
	else
		len = strlen(s);
	for (char *p = &s[0]; p < &s[len]; p++) {
		if (sw_islower(*p))
			*p = toupper(*p);
	}
	return s;
}

/**
 * Duplicate a string
 *
 * @param string String to duplicate
 * @return An exact copy of the input string whose storage is obtained with
 *         malloc()
 */
char *
sw_strdup(const char *string)
{
	char	*dest = NULL;
	size_t	 size = 0;

	if (isNull(string))
		err_exit(EINVAL, "%s error", __func__);
	else
		size = strlen(string) + 1;

	if ((dest = malloc(size)) == NULL) {
		err_exit(ENOMEM, "%s error (allocating " PRINT_SIZE " bytes)",
		    __func__, size);
	}

	if ((errno = sw_strcpy(dest, string, size)) != 0)
		err_sys("%s error", __func__);
	return dest;
}

/**
 * Remove trailing data determined by sw_isspace()
 *
 * @param string Target string
 * @return The result
 */
char *
trim(char *string)
{
	char	*p = NULL;

	if (isNull(string)) {
		err_exit(EINVAL, "trim");
	} else if (isEmpty(string)) {
		return string;
	} else {
		for (p = &string[strlen(string) - 1]; p >= &string[0]; p--) {
			if (!sw_isspace(*p))
				break;
		}

		*(p + 1) = '\0';
	}

	return string;
}

/**
 * Get string describing color
 *
 * @param color Color number constant (defined by the Ncurses headers)
 * @return String describing color
 */
const char *
strColor(short int color)
{
	switch (color) {
	case COLOR_BLACK:
		return ("Black");
	case COLOR_RED:
		return ("Red");
	case COLOR_GREEN:
		return ("Green");
	case COLOR_YELLOW:
		return ("Yellow");
	case COLOR_BLUE:
		return ("Blue");
	case COLOR_MAGENTA:
		return ("Magenta");
	case COLOR_CYAN:
		return ("Cyan");
	case COLOR_WHITE:
		return ("White");
	}

	return ("Unknown Color!");
}

/**
 * Replace spaces in a string with newlines
 *
 * @param string Target string
 * @param count  Attempt to replace this count of occurrences
 * @return Number of newlines successfully written
 */
int
strFeed(char *string, int count)
{
	char	*p = NULL;
	int	 feeds_written = 0;

	if (isNull(string) || isEmpty(string) || count <= 0)
		return feeds_written;
	while (feeds_written < count) {
		if ((p = strchr(string, ' ')) == NULL)
			break;
		*p = '\n', feeds_written++;
	}
	return feeds_written;
}

/**
 * Squeeze characters in a buffer.
 *
 * @param buffer Target buffer.
 * @param reject String with characters to squeeze.
 * @return Void
 */
void
squeeze(char *buffer, const char *reject)
{
	long int	i, j;

	if (isNull(buffer) || isEmpty(buffer) ||
	    isNull(reject) || isEmpty(reject))
		return;
	for (i = j = 0; buffer[i] != '\0'; i++) {
		if (strchr(reject, buffer[i]) == NULL)
			buffer[j++] = buffer[i];
	}
	buffer[j] = '\0';
}

/**
 * Wrapper function for snprintf() that doesn't care about truncation.
 *
 * @param dest Destination to write to.
 * @param size Destination size.
 * @param fmt  Format control.
 * @return Void
 */
void
sw_snprintf(char *dest, size_t size, const char *fmt, ...)
{
	int n_print;
	va_list ap;

	if (dest == NULL || size == 0 || fmt == NULL)
		err_exit(EINVAL, "%s", __func__);

	va_start(ap, fmt);
	if ((n_print = vsnprintf(dest, size, fmt, ap)) < 0)
		err_sys("%s: vsnprintf() returned %d", __func__, n_print);
	va_end(ap);
}
