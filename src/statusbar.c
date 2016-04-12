/* Swirc statusbar
   Copyright (C) 2012-2014, 2016 Markus Uhlin. All rights reserved.

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

#if defined(WIN32) && defined(PDC_EXP_EXTRAS)
#include "curses-funcs.h" /* is_scrollok() */
#endif

#include "cursesInit.h"
#include "printtext.h"
#include "statusbar.h"
#include "strdup_printf.h"
#include "terminal.h"
#include "theme.h"

static PANEL      *statusbar_pan       = NULL;
static const char  irc_client_slogan[] = "\0039,1The universal IRC client\017";

static void
apply_statusbar_options(WINDOW *win)
{
    if (is_scrollok(win)) {
	(void) scrollok(win, FALSE);
    }
}

static short int
get_pair_num()
{
    short int fg, bg;
    short int pair_n;

    fg = theme_color_unparse("statusbar_fg", COLOR_WHITE);
    bg = theme_color_unparse("statusbar_bg", COLOR_BLACK);

    if ((pair_n = color_pair_find(fg, bg)) != -1) {
	return pair_n;
    }

    return 0;
}

void
statusbar_init(void)
{
    statusbar_pan = term_new_panel(1, 0, LINES-2, 0);
    apply_statusbar_options(panel_window(statusbar_pan));
}

void
statusbar_deinit(void)
{
    term_remove_panel(statusbar_pan);
}

void
statusbar_update_display_beta(void)
{
#define WERASE(win)   ((void) werase(win))
#define WBKGD(win, c) ((void) wbkgd(win, c))
    WINDOW     *win    = panel_window(statusbar_pan);
    chtype      blank  = ' ';
    short int   pair_n = get_pair_num();
    const char *lb     = Theme("statusbar_leftBracket");
    const char *rb     = Theme("statusbar_rightBracket");
    char       *out_s  = Strdup_printf(
	"%s %s%d%s %s", Theme("statusbar_spec"), lb, g_active_window->refnum, rb, irc_client_slogan);

    WERASE(win);
    WBKGD(win, blank | COLOR_PAIR(pair_n) | A_NORMAL);

    printtext_puts(win, g_no_colors ? squeeze_text_deco(out_s) : out_s, -1, -1, NULL);
    free(out_s);
    statusbar_show();
}

void
statusbar_recreate(int rows, int cols)
{
    struct term_window_size newsize = {
	.rows	   = 1,
	.cols	   = cols,
	.start_row = rows - 2,
	.start_col = 0,
    };

    statusbar_pan = term_resize_panel(statusbar_pan, &newsize);
    apply_statusbar_options(panel_window(statusbar_pan));
    statusbar_update_display_beta();
}

void
statusbar_show(void)
{
    if (statusbar_pan) {
	(void) show_panel(statusbar_pan);
    }
}

void
statusbar_hide(void)
{
    if (statusbar_pan) {
	(void) hide_panel(statusbar_pan);
    }
}
