/* Readline API
   Copyright (C) 2012-2014 Markus Uhlin. All rights reserved.

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

#include <limits.h>
#include <string.h>

#include "assertAPI.h"
#include "errHand.h"
#include "libUtils.h"
#include "printtext.h"
#include "readline.h"
#include "readlineAPI.h"

#define WATTR_ON(win, attrs)  ((void) wattr_on(win, attrs, 0))
#define WATTR_OFF(win, attrs) ((void) wattr_off(win, attrs, 0))

static bool	 is_text_decoration (wint_t wc);
static char	*convert_wc         (wchar_t);
static void	 add_complex_char   (WINDOW *, int c);
static void	 ins_complex_char   (WINDOW *, int c);

SW_NORET void
readline_error(int error, const char *msg)
{
    struct printtext_context ptext_ctx = {
	.window     = g_status_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    printtext(&ptext_ctx, "non-fatal: %s: %s", msg, errdesc_by_num(error));
    g_readline_loop = false;
    longjmp(g_readline_loc_info, 1);
}

static char *
convert_wc(wchar_t wc)
{
    mbstate_t ps;
#ifdef HAVE_BCI
    size_t bytes_written;
#endif
    const size_t size = MB_LEN_MAX + 1;
    char *mbs = xcalloc(size, 1);

    BZERO(&ps, sizeof (mbstate_t));
#ifdef HAVE_BCI
    if ((errno = wcrtomb_s(&bytes_written, mbs, size, wc, &ps)) != 0) {
	readline_error(errno, "wcrtomb_s");
    }
#else
    if (wcrtomb(mbs, wc, &ps) == ((size_t) -1)) {
	readline_error(errno, "wcrtomb");
    }
#endif
    return (mbs);
}

static bool
is_text_decoration(wint_t wc)
{
    return (wc==btowc(BLINK) || wc==btowc(BOLD) || wc==btowc(COLOR) ||
	    wc==btowc(NORMAL) || wc==btowc(REVERSE) || wc==btowc(UNDERLINE));
}

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

void
readline_mvwaddch(WINDOW *win, int row, int col, wint_t wc)
{
    char *mbs = convert_wc(wc);

    if (wmove(win, row, col) == ERR) {
	free(mbs);
	readline_error(EPERM, "wmove");
    }
    if (!is_text_decoration(wc)) {
	if (waddnstr(win, mbs, -1) == ERR) {
	    free(mbs);
	    readline_error(EPERM, "waddnstr");
	}
    } else {
	add_complex_char(win, *mbs);
    }
    free(mbs);
}

void
readline_waddch(WINDOW *win, wint_t wc)
{
    char *mbs = convert_wc(wc);

    if (!is_text_decoration(wc)) {
	if (waddnstr(win, mbs, -1) == ERR) {
	    free(mbs);
	    readline_error(EPERM, "waddnstr");
	}
    } else {
	add_complex_char(win, *mbs);
    }
    free(mbs);
}

void
readline_waddnstr(WINDOW *win, const wchar_t *s, ptrdiff_t n)
{
    const wchar_t   *ptr;
    const ptrdiff_t  len_of_s   = (ptrdiff_t) wcslen(s ? s : L"");
    const ptrdiff_t  this_index = ((n <= 0 || n > len_of_s) ? len_of_s : n);

    if (win == NULL || s == NULL) {
	err_exit(EINVAL, "readline_waddnstr fatal!");
    }
    update_panels();
    for (ptr = &s[0]; ptr < &s[this_index]; ptr++) {
	readline_waddch(win, *ptr);
    }
}

void
readline_mvwinsch(WINDOW *win, int row, int col, wint_t wc)
{
    char *mbs = convert_wc(wc);

    if (wmove(win, row, col) == ERR) {
	free(mbs);
	readline_error(EPERM, "wmove");
    }
    if (!is_text_decoration(wc)) {
	if (winsnstr(win, mbs, -1) == ERR) {
	    free(mbs);
	    readline_error(EPERM, "winsnstr");
	}
    } else {
	ins_complex_char(win, *mbs);
    }
    free(mbs);
}

void
readline_winsch(WINDOW *win, wint_t wc)
{
    char *mbs = convert_wc(wc);

    if (!is_text_decoration(wc)) {
	if (winsnstr(win, mbs, -1) == ERR) {
	    free(mbs);
	    readline_error(EPERM, "winsnstr");
	}
    } else {
	ins_complex_char(win, *mbs);
    }
    free(mbs);
}

void
readline_winsnstr(WINDOW *win, const wchar_t *s, ptrdiff_t n)
{
    const wchar_t   *ptr;
    const ptrdiff_t  len_of_s   = (ptrdiff_t) wcslen(s ? s : L"");
    const ptrdiff_t  this_index = ((n <= 0 || n > len_of_s) ? len_of_s : n);

    if (win == NULL || s == NULL) {
	err_exit(EINVAL, "readline_winsnstr fatal!");
    }
    update_panels();
    for (ptr = &s[this_index-1]; ptr >= &s[0]; ptr--) {
	readline_winsch(win, *ptr);
    }
}
