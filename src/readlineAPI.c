/* Readline API
   Copyright (C) 2012-2023 Markus Uhlin. All rights reserved.

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

#include <limits.h>
#include <string.h>

#include "assertAPI.h"
#include "dataClassify.h"
#include "errHand.h"
#include "libUtils.h"
#include "printtext.h"
#include "readline.h"
#include "readlineAPI.h"
#include "strHand.h"

#define WATTR_ON(win, attrs)  ((void) wattr_on(win, attrs, NULL))
#define WATTR_OFF(win, attrs) ((void) wattr_off(win, attrs, NULL))

/**
 * Convert a wide-character to a multibyte sequence
 */
static STRING
convert_wc(wchar_t wc)
{
	const size_t	 size = MB_LEN_MAX;
	STRING		 mbs = xmalloc(size + 1);
	mbstate_t	 ps;
	size_t		 bytes_written = 0;

	BZERO(&ps, sizeof(mbstate_t));
	mbs[size] = '\0';

	if (wc == L'\0') {
		mbs[0] = '\0';
		return mbs;
	}

#ifdef HAVE_BCI
	if ((errno = wcrtomb_s(&bytes_written, mbs, size, wc, &ps)) != 0 ||
	    bytes_written == g_conversion_failed ||
	    bytes_written > size) {
		free(mbs);
		readline_error(errno, "wcrtomb_s");
	}
#else
	if ((bytes_written = wcrtomb(mbs, wc, &ps)) == g_conversion_failed ||
	    bytes_written > size) {
		free(mbs);
		readline_error(errno, "wcrtomb");
	}
#endif

	mbs[bytes_written] = '\0';
	return xrealloc(mbs, bytes_written + 1);
}

/**
 * Check for text-decoration
 */
static bool
is_text_decoration(const wint_t wc)
{
	return (wc == btowc(BLINK) ||
	    wc == btowc(BOLD) ||
	    wc == btowc(COLOR) ||
	    wc == btowc(NORMAL) ||
	    wc == btowc(REVERSE) ||
	    wc == btowc(UNDERLINE));
}

/**
 * Add a complex character
 */
static void
add_complex_char(WINDOW *win, int c)
{
#define WADDCH(win, c) ((void) waddch(win, c))
	WATTR_ON(win, A_REVERSE);

	switch (c) {
	case BLINK:
		WADDCH(win, 'I');
		break;
	case BOLD:
		WADDCH(win, 'B');
		break;
	case COLOR:
		WADDCH(win, 'C');
		break;
	case NORMAL:
		WADDCH(win, 'N');
		break;
	case REVERSE:
		WADDCH(win, 'R');
		break;
	case UNDERLINE:
		WADDCH(win, 'U');
		break;
	default:
		sw_assert_not_reached();
	}

	WATTR_OFF(win, A_REVERSE);
}

/**
 * Insert a complex character
 */
static void
ins_complex_char(WINDOW *win, int c)
{
#define WINSCH(win, c) ((void) winsch(win, c))
	WATTR_ON(win, A_REVERSE);

	switch (c) {
	case BLINK:
		WINSCH(win, 'I');
		break;
	case BOLD:
		WINSCH(win, 'B');
		break;
	case COLOR:
		WINSCH(win, 'C');
		break;
	case NORMAL:
		WINSCH(win, 'N');
		break;
	case REVERSE:
		WINSCH(win, 'R');
		break;
	case UNDERLINE:
		WINSCH(win, 'U');
		break;
	default:
		sw_assert_not_reached();
	}

	WATTR_OFF(win, A_REVERSE);
}

/**
 * Readline error handling
 */
NORETURN void
readline_error(int error, CSTRING msg)
{
	PRINTTEXT_CONTEXT ctx;
	char strerrbuf[MAXERROR] = { '\0' };

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "non-fatal: %s: %s", msg, xstrerror(error, strerrbuf,
	    MAXERROR));
	g_readline_loop = false;
	longjmp(g_readline_loc_info, READLINE_RESTART);
}

/**
 * Readline error handling
 * (Formatted version)
 */
NORETURN void
readline_ferror(int error, CSTRING fmt, ...)
{
	PRINTTEXT_CONTEXT ctx;
	char out[900] = { '\0' };
	char strerrbuf[MAXERROR] = { '\0' };
	va_list ap;

	va_start(ap, fmt);
#if defined(UNIX)
	(void) vsnprintf(out, sizeof out, fmt, ap);
#elif defined(WIN32)
	(void) vsnprintf_s(out, sizeof out, _TRUNCATE, fmt, ap);
#endif
	va_end(ap);

	if (error) {
		(void) sw_strcat(out, ": ", sizeof out);
		(void) sw_strcat(out, xstrerror(error, strerrbuf, MAXERROR),
		    sizeof out);
	}

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "non-fatal: %s", out);
	g_readline_loop = false;
	longjmp(g_readline_loc_info, READLINE_RESTART);
}

/**
 * Add a character at given position
 *
 * @param win Window
 * @param row Row
 * @param col Col
 * @param wc Wide-character
 * @return Void
 */
void
readline_mvwaddch(WINDOW *win, int row, int col, wint_t wc)
{
	STRING mbs = convert_wc(wc);

	mutex_lock(&g_puts_mutex);
	if (wmove(win, row, col) == ERR) {
		free_and_null(&mbs);
		mutex_unlock(&g_puts_mutex);
		readline_ferror(0, "%s: wmove", __func__);
	} else if (!is_text_decoration(wc)) {
		if (waddnstr(win, mbs, -1) == ERR) {
			free_and_null(&mbs);
			mutex_unlock(&g_puts_mutex);
			readline_ferror(0, "%s: waddnstr", __func__);
		}
	} else {
		add_complex_char(win, *mbs);
	}
	free_and_null(&mbs);
	mutex_unlock(&g_puts_mutex);
}

/**
 * Add a character
 *
 * @param win Window
 * @param wc Wide-character
 * @return Void
 */
void
readline_waddch(WINDOW *win, wint_t wc)
{
	STRING mbs = convert_wc(wc);

	mutex_lock(&g_puts_mutex);
	if (!is_text_decoration(wc)) {
		if (waddnstr(win, mbs, -1) == ERR) {
			free_and_null(&mbs);
			mutex_unlock(&g_puts_mutex);
			readline_ferror(EIO, "%s: waddnstr", __func__);
			/* NOTREACHED */
		}
	} else {
		add_complex_char(win, *mbs);
	}
	free_and_null(&mbs);
	mutex_unlock(&g_puts_mutex);
}

/**
 * Add a string of characters
 *
 * @param win Window
 * @param s String
 * @param n Number of wide-characters
 * @return Void
 */
void
readline_waddnstr(WINDOW *win, const wchar_t *s, ptrdiff_t n)
{
	const ptrdiff_t length = (ptrdiff_t) wcslen(s);
	const ptrdiff_t i = (n <= 0 || n > length ? length : n);

	for (const wchar_t *ptr = &s[0]; ptr < &s[i]; ptr++)
		readline_waddch(win, *ptr);
}

/**
 * Insert character at given row/col
 *
 * @param win Window
 * @param row Row
 * @param col Col
 * @param wc Wide-character
 * @return Void
 */
void
readline_mvwinsch(WINDOW *win, int row, int col, wint_t wc)
{
	STRING mbs = convert_wc(wc);

	mutex_lock(&g_puts_mutex);
	if (wmove(win, row, col) == ERR) {
		free_and_null(&mbs);
		mutex_unlock(&g_puts_mutex);
		readline_ferror(0, "%s: wmove", __func__);
	} else if (!is_text_decoration(wc)) {
		if (winsnstr(win, mbs, size_to_int(strlen(mbs) + 1)) == ERR) {
			free_and_null(&mbs);
			mutex_unlock(&g_puts_mutex);
			readline_ferror(0, "%s: winsnstr", __func__);
		}
	} else {
		ins_complex_char(win, *mbs);
	}
	free_and_null(&mbs);
	mutex_unlock(&g_puts_mutex);
}

/**
 * Insert character before cursor
 *
 * @param win Window
 * @param wc Wide-character
 * @return Void
 */
void
readline_winsch(WINDOW *win, wint_t wc)
{
	STRING mbs = convert_wc(wc);

	mutex_lock(&g_puts_mutex);
	if (!is_text_decoration(wc)) {
		if (winsnstr(win, mbs, size_to_int(strlen(mbs) + 1)) == ERR) {
			free_and_null(&mbs);
			mutex_unlock(&g_puts_mutex);
			readline_ferror(EIO, "%s: winsnstr", __func__);
			/* NOTREACHED */
		}
	} else {
		ins_complex_char(win, *mbs);
	}
	free_and_null(&mbs);
	mutex_unlock(&g_puts_mutex);
}

/**
 * Insert string before cursor
 *
 * @param win Window
 * @param s String
 * @param n Number of wide-characters
 * @return Void
 */
void
readline_winsnstr(WINDOW *win, const wchar_t *s, ptrdiff_t n)
{
	const ptrdiff_t length = (ptrdiff_t) wcslen(s);
	const ptrdiff_t i = (n <= 0 || n > length ? length : n);

	for (const wchar_t *ptr = &s[i - 1]; ptr >= &s[0]; ptr--)
		readline_winsch(win, *ptr);
}

int
readline_wcwidth(const wchar_t wc, const int fwlen)
{
	if (is_text_decoration(wc))
		return 1;
	return xwcwidth(wc, fwlen);
}

int
readline_wcswidth(const wchar_t *str, const int fwlen)
{
	const wchar_t	*ptr = str;
	unsigned int	 width = 0;

	while (*ptr) {
		width += readline_wcwidth(*ptr, fwlen);
		ptr++;
	}
	return width;
}
