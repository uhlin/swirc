/* OS-independent terminal stuff
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

#include "atomicops.h"
#include "config.h"
#include "errHand.h"
#include "main.h"
#include "readline.h"
#include "statusbar.h"
#include "terminal.h"
#include "titlebar.h"
#include "window.h"

volatile bool g_resizing_term = false;

static const short int TermMinimumRows = 10;
static const short int TermMinimumCols = 30;

void
term_init(void)
{
	term_set_title("Swirc %s | Copyright (C) %s %s", g_swircVersion,
	    g_swircYear, g_swircAuthor);
}

void
term_deinit(void)
{
	term_restore_title();
}

void
term_beep(void)
{
	if (config_bool("beeps", true))
		beep();
}

struct current_cursor_pos
term_get_pos(WINDOW *win)
{
	struct current_cursor_pos yx = { 0 };

	(void) wnoutrefresh(win);

	yx.cury = (win != NULL ? win->_cury : -1); /* row */
	yx.curx = (win != NULL ? win->_curx : -1); /* col */

	return yx;
}

bool
term_is_too_small(void)
{
	struct winsize size = term_get_size();

	return (size.ws_row < TermMinimumRows || size.ws_col < TermMinimumCols);
}

PANEL *
term_new_panel(int rows, int cols, int start_row, int start_col)
{
	WINDOW	*win;
	PANEL	*pan;

	if ((win = newwin(rows, cols, start_row, start_col)) == NULL) {
		err_quit("term_new_panel: newwin(%d, %d, %d, %d): "
		    "unable to create new window", rows, cols,
		    start_row, start_col);
	} else if ((pan = new_panel(win)) == NULL) {
		err_quit("term_new_panel: new_panel error "
		    "(could not associate window with a panel)");
	}

	return pan;
}

void
term_remove_panel(PANEL *pan)
{
	WINDOW *win = panel_window(pan);

	if (del_panel(pan) == ERR)
		err_quit("term_remove_panel: del_panel error");
	else if (delwin(win) == ERR)
		err_quit("term_remove_panel: delwin error");
}

void
term_resize_all(void)
{
	struct winsize size = term_get_size();
	int rows, cols;

	if (size.ws_row < TermMinimumRows || size.ws_col < TermMinimumCols ||
	    !is_term_resized(size.ws_row, size.ws_col) ||
	    atomic_load_bool(&g_resizing_term))
		return;
	else
		(void) atomic_swap_bool(&g_resizing_term, true);

	rows = (int) size.ws_row;
	cols = (int) size.ws_col;

	if (resize_term(rows, cols) == ERR)
		err_quit("term_resize_all: ERROR resize_term()");

	(void) erase();
	(void) refresh();

	titlebar_recreate(cols);
	statusbar_recreate(rows, cols);
	windows_recreate_all(rows, cols);
	readline_recreate(rows, cols);

	update_panels();
	(void) doupdate();
	(void) atomic_swap_bool(&g_resizing_term, false);
}

PANEL *
term_resize_panel(PANEL *pan, const struct term_window_size *newsize)
{
	WINDOW	*old_window = panel_window(pan);
	WINDOW	*repl_win; /* replacement window */

	if ((repl_win = newwin(newsize->rows, newsize->cols, newsize->start_row,
	    newsize->start_col)) == NULL) {
		err_quit("term_resize_panel: newwin(%d, %d, %d, %d): "
		    "unable to create new window", newsize->rows, newsize->cols,
		    newsize->start_row, newsize->start_col);
	} else if (replace_panel(pan, repl_win) == ERR) {
		err_quit("term_resize_panel: replace_panel error");
	} else if (delwin(old_window) == ERR) {
		err_quit("term_resize_panel: delwin error");
	}

	return pan;
}
