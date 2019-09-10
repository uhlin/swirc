/* Initialization of the Ncurses library
   Copyright (C) 2012-2019 Markus Uhlin. All rights reserved.

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

#include "curses-funcs.h"
#include "cursesInit.h"
#include "errHand.h"
#include "strHand.h"
#include "theme.h"

bool		g_no_colors         = false;
short int	g_initialized_pairs = -1;

/**
 * Initialize color pairs by calling init_pair()
 *
 * @return The no. of pairs that have been initialized. And on error
 * it returns a negative value.
 */
static short int
init_color_pairs()
{
    short int pair_n = 0;
    short int colors[] = {
	COLOR_BLACK,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_YELLOW,
	COLOR_BLUE,
	COLOR_MAGENTA,
	COLOR_CYAN,
	COLOR_WHITE
    };
    const size_t numColors = ARRAY_SIZE(colors);
    short int *fg, *bg;

    /* Initialize black on black */
    if (init_pair(++pair_n, colors[0], colors[0]) == ERR) {
	err_msg("Could not initialize pair %hd", pair_n);
	return -1;
    }

    /* Initialize a color on the default background of the terminal */
    if (theme_bool_unparse("term_use_default_colors", true)) {
	short int *psi;

	for (psi = &colors[0]; psi < &colors[numColors]; psi++) {
	    if (init_pair(++pair_n, *psi, -1) == ERR) {
		err_msg("Could not initialize pair %hd", pair_n);
		return -1;
	    }
	}
    }

    for (fg = &colors[0]; fg < &colors[numColors]; fg++) {
	for (bg = &colors[0]; bg < &colors[numColors]; bg++) {
	    if (*fg != *bg && init_pair(++pair_n, *fg, *bg) == ERR) {
		if (pair_n == 64) {
		    /* The pair number is 64. The terminal that is
		     * being used most likely lack support for pairs
		     * >= 64. However: don't return -1 to indicate an
		     * error. */
		    return 63;
		} else {
		    char *fg_name = sw_strdup(strColor(*fg));
		    char *bg_name = sw_strdup(strColor(*bg));

		    err_msg("Could not initialize pair %hd (%s, %s)",
			    pair_n, fg_name, bg_name);

		    free(fg_name);
		    free(bg_name);
		    return -1;
		}
	    }
	}
    }

    for (size_t n = 1; n < numColors; n++) {
	if (init_pair(++pair_n, colors[n], colors[n]) == ERR)
	    return (pair_n - 1);
    }

    return pair_n;
}

/**
 * Initialization of the Ncurses library (done before usage)
 */
int
curses_init(void)
{
    (void) wrefresh(initscr());

    g_cursesMode  = true;
    g_endwin_fn   = endwin;
    g_doupdate_fn = doupdate;

    if (!theme_bool_unparse("term_enable_colors", true) || !has_colors() ||
	start_color() == ERR) {
	g_no_colors = true;
    } else {
	if (theme_bool_unparse("term_use_default_colors", true) &&
	    use_default_colors() != OK) {
	    err_msg("use_default_colors() ran unsuccessful!\n"
		"Troubleshooting: set option term_use_default_colors to NO.");
	    return ERR;
	}
    }

    if (cbreak() == ERR) {
	err_msg("Could not enter terminal cbreak mode");
	return ERR;
    }

    if (noecho() == ERR) {
	err_msg("Unable to turn echoing off!");
	return ERR;
    }

    if (!g_no_colors && (g_initialized_pairs = init_color_pairs()) < 0) {
	return ERR;
    }

    return OK;
}
