/* Swirc titlebar
   Copyright (C) 2012-2018 Markus Uhlin. All rights reserved.

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

#include <string.h>

#if defined(WIN32) && defined(PDC_EXP_EXTRAS)
#include "curses-funcs.h"
#endif

#include "printtext.h"
#include "strdup_printf.h"
#include "terminal.h"
#include "theme.h"
#include "titlebar.h"

static PANEL *titlebar_pan = NULL;

static void
apply_titlebar_options(WINDOW *win)
{
    if (is_scrollok(win)) {
	(void) scrollok(win, false);
    }
}

static short int
get_pair_num()
{
    short int fg, bg;
    short int pair_n;

    fg = theme_color_unparse("titlebar_fg", COLOR_BLACK);
    bg = theme_color_unparse("titlebar_bg", COLOR_WHITE);

    if ((pair_n = color_pair_find(fg, bg)) != -1) {
	return pair_n;
    }

    return 0;
}

void
titlebar_init(void)
{
    titlebar_pan = term_new_panel(1, 0, 0, 0);
    apply_titlebar_options(panel_window(titlebar_pan));
}

void
titlebar_deinit(void)
{
    term_remove_panel(titlebar_pan);
}

void
titlebar(const char *fmt, ...)
{
    va_list    ap;
    char      *fmt_copy;
    WINDOW    *win    = panel_window(titlebar_pan);
    chtype     blank  = ' ';
    short int  pair_n = get_pair_num();
    char      *reject = strdup_printf(
	"%c%c%c%c%c%c", BLINK, BOLD, COLOR, NORMAL, REVERSE, UNDERLINE);

    va_start(ap, fmt);
    fmt_copy = strdup_vprintf(fmt, ap);
    va_end(ap);

    (void) werase(win);
    (void) wbkgd(win, blank | COLOR_PAIR(pair_n) | A_NORMAL);

    if (strpbrk(fmt_copy, reject) != NULL) {
	printtext_puts(win, squeeze_text_deco(fmt_copy), -1, -1, NULL);
    } else {
	printtext_puts(win, fmt_copy, -1, -1, NULL);
    }

    free(fmt_copy);
    free(reject);
}

void
titlebar_recreate(int cols)
{
    struct term_window_size newsize = {
	.rows	   = 1,
	.cols      = cols,
	.start_row = 0,
	.start_col = 0,
    };

    titlebar_pan = term_resize_panel(titlebar_pan, &newsize);
    apply_titlebar_options(panel_window(titlebar_pan));
    titlebar(" %s ", g_active_window->title ? g_active_window->title : "");
}
