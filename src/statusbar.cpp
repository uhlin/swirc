/* Swirc statusbar
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

#include <string>

#if defined(WIN32) && defined(PDC_EXP_EXTRAS)
#include "curses-funcs.h" /* is_scrollok() */
#endif

#include "cursesInit.h"
#include "dataClassify.h"
#include "irc.h"
#include "printtext.h"
#include "statusbar.h"
#include "strHand.h"
#include "terminal.h"
#include "theme.h"

char g_user_modes[100] = "";

static PANEL *statusbar_pan = NULL;

static void
apply_statusbar_options(WINDOW *win)
{
    if (is_scrollok(win)) {
	(void) scrollok(win, false);
    }
}

static std::string
get_chanmodes()
{
    PIRC_WINDOW win;
    std::string str("");

    if ((win = g_active_window) != NULL) {
	if (strings_match_ignore_case(win->label, g_status_window_label)) {
	    str.append(Theme("slogan"));
	} else if (is_irc_channel(win->label)) {
	    str.append(win->label);
	    str.append("(");
	    str.append(win->chanmodes);
	    str.append(")");
	} else {
	    str.append(win->label);
	}
    }

    return str;
}

static std::string
get_nick_and_server()
{
    std::string str("");

    if (g_my_nickname && g_server_hostname) {
	str.append(g_my_nickname);
	str.append("(");
	str.append(g_user_modes[0] == ':' ? &g_user_modes[1] : &g_user_modes[0]);
	str.append(")");
	str.append("@");
	str.append(g_server_hostname);
    }

    return str;
}

static short int
get_pair_num()
{
    short int fg, bg;
    short int pair_n;

    fg = theme_color("statusbar_fg", COLOR_WHITE);
    bg = theme_color("statusbar_bg", COLOR_BLACK);

    if ((pair_n = color_pair_find(fg, bg)) != -1) {
	return pair_n;
    }

    return 0;
}

void
statusbar_init(void)
{
    statusbar_pan = term_new_panel(1, 0, LINES - 2, 0);
    apply_statusbar_options(panel_window(statusbar_pan));
}

void
statusbar_deinit(void)
{
	term_remove_panel(statusbar_pan);
}

void
statusbar_show(void)
{
	if (statusbar_pan)
		(void) show_panel(statusbar_pan);
}

void
statusbar_hide(void)
{
	if (statusbar_pan)
		(void) hide_panel(statusbar_pan);
}

void
statusbar_recreate(int rows, int cols)
{
	struct term_window_size newsize(1, cols, rows - 2, 0);

	statusbar_pan = term_resize_panel(statusbar_pan, &newsize);
	apply_statusbar_options(panel_window(statusbar_pan));
	statusbar_update_display_beta();
}

void
statusbar_top_panel(void)
{
	if (statusbar_pan)
		(void) top_panel(statusbar_pan);
}

void
statusbar_update_display_beta(void)
{
	WINDOW			*win = panel_window(statusbar_pan);
	char			*out_s;
	const chtype		 blank = ' ';
	const std::string	 lb(Theme("statusbar_leftBracket"));
	const std::string	 rb(Theme("statusbar_rightBracket"));
	short int		 pair_n = get_pair_num();
	std::string		 str(Theme("statusbar_spec"));

	if (term_is_too_small())
		return;

	(void) werase(win);
	(void) wbkgd(win, (blank | COLOR_PAIR(pair_n) | A_NORMAL));

#ifndef _lint
	(void) str.append(" ");
	(void) str.append(lb);
	(void) str.append(std::to_string(g_active_window->refnum));
	(void) str.append("(");
	(void) str.append(std::to_string(g_ntotal_windows));
	(void) str.append(")");
	(void) str.append(rb);
#endif

	(void) str.append(" ");
	(void) str.append(lb).append(get_nick_and_server()).append(rb);

	(void) str.append(" ");
	(void) str.append(lb).append(get_chanmodes()).append(rb);

	(void) str.append(" ");
	(void) str.append(lb);
	(void) str.append("Log: ").append(g_active_window->logging ? "Yes"
	    : "No");
	(void) str.append(rb);

	(void) str.append(" ");
	(void) str.append(g_active_window->scroll_mode ? "-- MORE --" : "");

	out_s = sw_strdup(str.c_str());
	printtext_puts(win, (g_no_colors ? squeeze_text_deco(out_s) : out_s),
	    -1, -1, NULL);
	free(out_s);

	statusbar_top_panel();
}
