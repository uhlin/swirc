/* Readline API
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

#include <limits.h>
#include <string.h>

#include "assertAPI.h"
#include "errHand.h"
#include "libUtils.h"
#include "printtext.h"
#include "readline.h"
#include "readlineAPI.h"

#define WATTR_ON(win, attrs)  ((void) wattr_on(win, attrs, NULL))
#define WATTR_OFF(win, attrs) ((void) wattr_off(win, attrs, NULL))

/**
 * Convert a wide-character to a multibyte sequence
 */
static char *
convert_wc(wchar_t wc)
{
    mbstate_t ps;
#ifdef HAVE_BCI
    size_t bytes_written;
#endif
    const size_t size = MB_LEN_MAX + 1;
    char *mbs = xcalloc(size, 1);

    BZERO(&ps, sizeof(mbstate_t));
#ifdef HAVE_BCI
    if ((errno = wcrtomb_s(&bytes_written, mbs, size, wc, &ps)) != 0)
	readline_error(errno, "wcrtomb_s");
#else
    if (wcrtomb(mbs, wc, &ps) == ((size_t) -1))
	readline_error(errno, "wcrtomb");
#endif
    return (mbs);
}

/**
 * Check for text-decoration
 */
static bool
is_text_decoration(wint_t wc)
{
    return (wc==btowc(BLINK) || wc==btowc(BOLD) || wc==btowc(COLOR) ||
	    wc==btowc(NORMAL) || wc==btowc(REVERSE) || wc==btowc(UNDERLINE));
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
readline_error(int error, const char *msg)
{
    PRINTTEXT_CONTEXT ctx;
    char strerrbuf[MAXERROR] = { '\0' };

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_FAILURE, true);
    printtext(&ctx, "non-fatal: %s: %s", msg,
	xstrerror(error, strerrbuf, MAXERROR));
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
    char *mbs = convert_wc(wc);

    if (wmove(win, row, col) == ERR) {
	free_and_null(&mbs);
	readline_error(EPERM, "wmove");
    }
    if (!is_text_decoration(wc)) {
	if (waddnstr(win, mbs, -1) == ERR) {
	    free_and_null(&mbs);
	    readline_error(EPERM, "waddnstr");
	}
    } else {
	add_complex_char(win, *mbs);
    }
    free_and_null(&mbs);
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
    char *mbs = convert_wc(wc);

    if (!is_text_decoration(wc)) {
	if (waddnstr(win, mbs, -1) == ERR) {
	    free_and_null(&mbs);
	    readline_error(EPERM, "waddnstr");
	}
    } else {
	add_complex_char(win, *mbs);
    }
    free_and_null(&mbs);
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
    const ptrdiff_t length = (ptrdiff_t) (s ? wcslen(s) : 0);
    const ptrdiff_t this_index = (n <= 0 || n > length ? length : n);

    if (win == NULL || s == NULL)
	err_exit(EINVAL, "fatal: readline_waddnstr");

    update_panels();

    for (const wchar_t *ptr = &s[0]; ptr < &s[this_index]; ptr++)
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
    char *mbs = convert_wc(wc);

    if (wmove(win, row, col) == ERR) {
	free_and_null(&mbs);
	readline_error(EPERM, "wmove");
    }
    if (!is_text_decoration(wc)) {
	if (winsnstr(win, mbs, size_to_int(strlen(mbs) + 1)) == ERR) {
	    free_and_null(&mbs);
	    readline_error(EPERM, "winsnstr");
	}
    } else {
	ins_complex_char(win, *mbs);
    }
    free_and_null(&mbs);
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
    char *mbs = convert_wc(wc);

    if (!is_text_decoration(wc)) {
	if (winsnstr(win, mbs, size_to_int(strlen(mbs) + 1)) == ERR) {
	    free_and_null(&mbs);
	    readline_error(EPERM, "winsnstr");
	}
    } else {
	ins_complex_char(win, *mbs);
    }
    free_and_null(&mbs);
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
    const ptrdiff_t length = (ptrdiff_t) (s ? wcslen(s) : 0);
    const ptrdiff_t this_index = (n <= 0 || n > length ? length : n);

    if (win == NULL || s == NULL)
	err_exit(EINVAL, "fatal: readline_winsnstr");

    update_panels();

    for (const wchar_t *ptr = &s[this_index - 1]; ptr >= &s[0]; ptr--)
	readline_winsch(win, *ptr);
}
