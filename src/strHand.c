/* String handling functions
   Copyright (C) 2012-2015 Markus Uhlin. All rights reserved.

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

#if OS_X || BSD
#include <curses.h>
#elif LINUX
#include <ncursesw/curses.h>
#elif WIN32
#include "pdcurses/curses.h"
#else
#error "Cannot determine curses header file!"
#endif

#include "dataClassify.h"
#include "errHand.h"
#include "strHand.h"

#if defined(UNIX)
#define PRINT_SZ	"%zu"
#elif defined(WIN32)
#define PRINT_SZ	"%Iu"
#endif

char *
trim(char *string)
{
    char *p;

    if (isNull(string)) {
	err_exit(EINVAL, "trim error");
    } else if (isEmpty(string)) {
	return string;
    } else {
	for (p = &string[strlen(string) - 1]; p >= &string[0]; p--) {
	    if (!sw_isspace(*p)) {
		break;
	    }
	}

	*(p + 1) = '\0';
    }

    return string;
}

void
squeeze(char *buffer, const char *rej)
{
    long int i, j;

    if (isNull(buffer) || isEmpty(buffer)) {
	return;
    }

    for (i = j = 0; buffer[i] != '\0'; i++) {
	if (strchr(rej, buffer[i]) == NULL) {
	    buffer[j++] = buffer[i];
	}
    }

    buffer[j] = '\0';
}

int
Strfeed(char *string, int count)
{
    int		 feeds_written = 0;
    char	*p;

    if (isNull(string) || isEmpty(string) || count <= 0) {
	return feeds_written;
    }

    while (feeds_written < count) {
	if ((p = strchr(string, ' ')) == NULL) {
	    break;
	}

	*p = '\n', feeds_written++;
    }

    return feeds_written;
}

char *
str_toupper(char *s)
{
    size_t  len;
    char   *p;

    if (s == NULL) {
	err_exit(EINVAL, "str_toupper error");
    } else if (!*s) {
	return s;
    } else {
	len = strlen(s);
    }

    for (p = &s[0]; p < &s[len]; p++) {
	if (sw_islower(*p)) {
	    *p = toupper(*p);
	}
    }

    return s;
}

char *
str_tolower(char *s)
{
    size_t  len;
    char   *p;

    if (s == NULL) {
	err_exit(EINVAL, "str_tolower error");
    } else if (!*s) {
	return s;
    } else {
	len = strlen(s);
    }

    for (p = &s[0]; p < &s[len]; p++) {
	if (sw_isupper(*p)) {
	    *p = tolower(*p);
	}
    }

    return s;
}

void
sw_snprintf(char *dest, size_t sz, const char *fmt, ...)
{
    va_list	ap;
    int		n_print;

    if (dest == NULL || sz == 0 || fmt == NULL) {
	err_exit(EINVAL, "sw_snprintf error");
    }

    va_start(ap, fmt);
    if ((n_print = vsnprintf(dest, sz, fmt, ap)) < 0)
	err_sys("vsnprintf() returned %d", n_print);
    va_end(ap);
}

/* Unbounded */
void
sw_sprintf(char *dest, const char *fmt, ...)
{
    va_list	ap;
    int		n_print;

    if (dest == NULL || fmt == NULL) {
	err_exit(EINVAL, "sw_sprintf error");
    }

    va_start(ap, fmt);
    if ((n_print = vsprintf(dest, fmt, ap)) < 0)
	err_sys("vsprintf returned %d", n_print);
    va_end(ap);
}

char *
sw_strdup(const char *string)
{
    size_t	 size;
    char	*dest;

    if (isNull(string)) {
	err_exit(EINVAL, "sw_strdup error");
    } else {
	size = strlen(string) + 1;
    }

    if ((dest = malloc(size)) == NULL) {
	err_exit(ENOMEM, "sw_strdup error (allocating " PRINT_SZ " bytes)", size);
    }

    if ((errno = sw_strcpy(dest, string, size)) != 0) {
	err_sys("sw_strdup error");
    }

    return (dest);
}

const char *
Strcolor(short int color)
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
