/* nicklist.cpp
   Copyright (C) 2021-2022 Markus Uhlin. All rights reserved.

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

#include <cstring>
#include <list>
#include <string>

#include "assertAPI.h"
#include "dataClassify.h"
#include "errHand.h"
#include "irc.h"
#include "libUtils.h"
#include "nicklist.h"
#include "printtext.h"
#include "readline.h"
#include "statusbar.h"
#include "strHand.h"
#include "terminal.h"
#include "theme.h"

const int	g_nicklist_maxnick = 16;
const int	g_nicklist_scroll_amount = 10;

static bool
can_scroll(const IRC_WINDOW *win)
{
	if (win == NULL || !is_irc_channel(win->label) ||
	    !win->received_names || win->nicklist.pan == NULL ||
	    win->nicklist.width <= 0 || term_is_too_small())
		return false;
	return true;
}

static bool
cmp_fn(const std::string &nick1, const std::string &nick2)
{
	size_t i = 1;

	/*
	 * nick1
	 */
	switch (nick1.at(0)) {
	case '~':
		if (nick2.at(0) == '&' || nick2.at(0) == '@' ||
		    nick2.at(0) == '%' || nick2.at(0) == '+' ||
		    nick2.at(0) == ' ')
			return true;
		break;
	case '&':
		if (nick2.at(0) == '@' || nick2.at(0) == '%' ||
		    nick2.at(0) == '+' || nick2.at(0) == ' ')
			return true;
		break;
	case '@':
		if (nick2.at(0) == '%' || nick2.at(0) == '+' ||
		    nick2.at(0) == ' ')
			return true;
		break;
	case '%':
		if (nick2.at(0) == '+' || nick2.at(0) == ' ')
			return true;
		break;
	case '+':
		if (nick2.at(0) == ' ')
			return true;
		break;
	case ' ':
	default:
		break;
	}

	/*
	 * nick2
	 */
	switch (nick2.at(0)) {
	case '~':
		if (nick1.at(0) == '&' || nick1.at(0) == '@' ||
		    nick1.at(0) == '%' || nick1.at(0) == '+' ||
		    nick1.at(0) == ' ')
			return false;
		break;
	case '&':
		if (nick1.at(0) == '@' || nick1.at(0) == '%' ||
		    nick1.at(0) == '+' || nick1.at(0) == ' ')
			return false;
		break;
	case '@':
		if (nick1.at(0) == '%' || nick1.at(0) == '+' ||
		    nick1.at(0) == ' ')
			return false;
		break;
	case '%':
		if (nick1.at(0) == '+' || nick1.at(0) == ' ')
			return false;
		break;
	case '+':
		if (nick1.at(0) == ' ')
			return false;
		break;
	case ' ':
	default:
		break;
	}

	while (i < nick1.length() && i < nick2.length()) {
		int c1, c2;

		c1 = sw_isupper(nick1[i]) ? tolower(nick1[i]) : nick1[i];
		c2 = sw_isupper(nick2[i]) ? tolower(nick2[i]) : nick2[i];
		if (c1 < c2)
			return true;
		else if (c1 > c2)
			return false;
		i ++;
	}

	return (nick1.length() < nick2.length() ? true : false);
}

static void
draw_hook()
{
	statusbar_top_panel();
	readline_top_panel();
}

static std::list<std::string>
get_list(const IRC_WINDOW *window, const bool sort)
{
	std::list<std::string> list;

	for (size_t i = 0; i < ARRAY_SIZE(window->names_hash); i++) {
		for (PNAMES names = window->names_hash[i];
		    names != NULL;
		    names = names->next) {
			char c;

			if (names->is_owner)
				c = '~';
			else if (names->is_superop)
				c = '&';
			else if (names->is_op)
				c = '@';
			else if (names->is_halfop)
				c = '%';
			else if (names->is_voice)
				c = '+';
			else
				c = ' ';

			std::string str("");
			str.push_back(c);
			str.append(names->nick);
			list.push_back(str);
		}
	}

	if (sort)
		list.sort(cmp_fn);
	return list;
}

static void
addvline(WINDOW *win, short int bg)
{
	bool state = false;
	struct integer_context color("nicklist_vline_color", 0, 99, 0);

	printtext_set_color(win, &state, static_cast<short int>
	    (theme_integer(&color)), bg);
	(void) waddch(win, ACS_VLINE);
}

static void
addnick(WINDOW *win, short int bg, const char *nick) noexcept
{
	bool state1 = false;
	bool state2 = false;
	const char *cp;
	struct integer_context priv_color("nicklist_privilege_color", 0, 99, 0);
	struct integer_context *nick_color = nullptr;

	printtext_set_color(win, &state1, static_cast<short int>
	    (theme_integer(&priv_color)), bg);
	(void) waddch(win, *nick);

	cp = &nick[1];
	if (g_my_nickname && strings_match_ignore_case(cp, g_my_nickname))
		nick_color = new integer_context("nicklist_my_nick_color",
		    0, 99, 0);
	else
		nick_color = new integer_context("nicklist_nick_color",
		    0, 99, 0);
	printtext_set_color(win, &state2, static_cast<short int>
	    (theme_integer(nick_color)), bg);
	delete nick_color;
	(void) waddstr(win, cp);
}

static void
resetattrs(WINDOW *win)
{
	(void) wattrset(win, A_NORMAL);
}

static void
printnick(WINDOW *win, const int row, const int col, const char *nick)
{
	short int bg;
	struct integer_context term_bg("term_background", 0, 15, 1);

	bg = (theme_bool("term_use_default_colors", true) ? -1 : static_cast
	    <short int>(theme_integer(&term_bg)));

	(void) wmove(win, row, col);
	addvline(win, bg);
	if (nick)
		addnick(win, bg, nick);
	resetattrs(win);
}

static void
list_fits_yes(PIRC_WINDOW win, WINDOW *nl_win, const int HEIGHT,
    std::list<std::string> &list)
{
	std::list<std::string>::iterator it;
	int count;

	(void) werase(nl_win);
	win->nicklist.scroll_pos = 0;

	it = list.begin();
	count = 0;

	while (it != list.end() && count < HEIGHT) {
		printnick(nl_win, count, 0, it->c_str());
		++it;
		++count;
	}

	while (count < HEIGHT) {
		printnick(nl_win, count, 0, NULL);
		count++;
	}

	draw_hook();
}

static void
list_fits_no(PIRC_WINDOW win, WINDOW *nl_win, const int HEIGHT,
    std::list<std::string> &list)
{
	std::list<std::string>::iterator it;
	int count;

	(void) werase(nl_win);

	if (win->nicklist.scroll_pos < 0) {
		win->nicklist.scroll_pos = 0;
	} else {
		it = list.begin();
		count = 0;

		std::advance(it, win->nicklist.scroll_pos);

		for (; it != list.end(); ++it)
			count++;
		if (count < HEIGHT)
			win->nicklist.scroll_pos -= (HEIGHT - count);
	}

	it = list.begin();
	count = 0;

	if (win->nicklist.scroll_pos)
		std::advance(it, win->nicklist.scroll_pos);

	while (it != list.end() && count < HEIGHT) {
		printnick(nl_win, count, 0, it->c_str());
		++it;
		++count;
	}

	draw_hook();
}

int
nicklist_new(PIRC_WINDOW win)
{
	const int width = nicklist_get_width(win);

	win->nicklist.pan		= NULL;
	win->nicklist.scroll_pos	= 0;
	win->nicklist.width		= width;

	window_recreate_exported(win, LINES, COLS);
	return 0;
}

int
nicklist_destroy(PIRC_WINDOW win)
{
	if (win->nicklist.pan)
		term_remove_panel(win->nicklist.pan);
	win->nicklist.pan = NULL;
	win->nicklist.scroll_pos = 0;
	win->nicklist.width = 0;
	return 0;
}

int
nicklist_draw(PIRC_WINDOW win, const int rows)
{
	WINDOW *nl_win;

	if (win == NULL || rows < 0 || !win->received_names ||
	    win->nicklist.pan == NULL) {
		return -1;
	} else if (term_is_too_small()) {
		(void) napms(30);
		return -1;
	}

	const int HEIGHT = (rows - 3);
	std::list<std::string> list(get_list(win, true));

	if ((nl_win = panel_window(win->nicklist.pan)) == NULL || HEIGHT < 0 ||
	    list.size() != static_cast<unsigned int>(win->num_total))
		return -1;

	const bool list_fits = !(win->num_total > HEIGHT);

	if (list_fits) {
		mutex_lock(&g_puts_mutex);
		list_fits_yes(win, nl_win, HEIGHT, list);
		mutex_unlock(&g_puts_mutex);
		return 0;
	} else {
		mutex_lock(&g_puts_mutex);
		list_fits_no(win, nl_win, HEIGHT, list);
		mutex_unlock(&g_puts_mutex);
		return 0;
	}

	/*NOTREACHED*/ sw_assert_not_reached();
	/*NOTREACHED*/ return -1;
}

int
nicklist_get_width(const PIRC_WINDOW window)
{
	size_t len = 0;

	for (size_t i = 0; i < ARRAY_SIZE(window->names_hash); i++) {
		for (PNAMES names = window->names_hash[i];
		    names != NULL;
		    names = names->next) {
			if (strlen(names->nick) > len)
				len = strlen(names->nick);
		}
	}

	if (len > g_nicklist_maxnick)
		len = g_nicklist_maxnick;
	len += 2; /* +2 for 'ACS_VLINE' and privilege (~&@%+) */
	return size_to_int(len);
}

void
nicklist_scroll_down(PIRC_WINDOW win)
{
	if (!can_scroll(win)) {
		term_beep();
		(void) napms(30);
		return;
	}

	win->nicklist.scroll_pos += g_nicklist_scroll_amount;

	if (nicklist_draw(win, LINES) != 0)
		debug("nicklist_scroll_down: nicklist_draw: error");
}

void
nicklist_scroll_up(PIRC_WINDOW win)
{
	if (!can_scroll(win)) {
		term_beep();
		(void) napms(30);
		return;
	}

	win->nicklist.scroll_pos -= g_nicklist_scroll_amount;

	if (nicklist_draw(win, LINES) != 0)
		debug("nicklist_scroll_up: nicklist_draw: error");
}

int
nicklist_update(PIRC_WINDOW win)
{
	if (win == NULL || !is_irc_channel(win->label) || !win->received_names)
		return -1;

	const bool width_changed = (nicklist_get_width(win) !=
	    win->nicklist.width);

	if (!width_changed)
		return nicklist_draw(win, LINES);

	window_recreate_exported(win, LINES, COLS);
	return 0;
}
