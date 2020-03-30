/* Initialization of the Ncurses library
   Copyright (C) 2012-2020 Markus Uhlin. All rights reserved.

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
#include "initcolors.h"
#include "strHand.h"
#include "theme.h"

bool		g_no_colors         = false;
short int	g_initialized_pairs = -1;

static const short int colors[] = {
    COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
    COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE,

#if 0
    LIGHT_RED,
    LIGHT_GREEN,
    LIGHT_CYAN,
    LIGHT_BLUE,
    PINK,
#endif
    GREY,
    LIGHT_GREY,
};

static const short int ext_colors[] = {
    52,   94, 100,  58,  22,  29,  23,  24,  17,  54,  53,  89, /* 16-27 */
    88,  130, 142,  64,  28,  35,  30,  25,  18,  91,  90, 125, /* 28-39 */
    124, 166, 184, 106,  34,  49,  37,  33,  19, 129, 127, 161, /* 40-51 */
    196, 208, 226, 154,  46,  86,  51,  75,  21, 171, 201, 198, /* 52-63 */
    203, 215, 227, 191,  83, 122,  87, 111,  63, 177, 207, 205, /* 64-75 */
    217, 223, 229, 193, 157, 158, 159, 153, 147, 183, 219, 212, /* 76-87 */
    16,  233, 235, 237, 239, 241, 244, 247, 250, 254, 231       /* 88-98 */
};

static const size_t numColors = ARRAY_SIZE(colors);
static const size_t numExtended = ARRAY_SIZE(ext_colors);

static int
init_fg_on_bg_case1(short int *pair_n)
{
#if LINUX
    FOREACH_FOREGROUND_EXTENDED() {
	FOREACH_BACKGROUND_ANSI() {
	    if (init_pair(++ (*pair_n), *fg, *bg) == ERR)
		return ERR;
	}
    }
#else
    (void) pair_n;
#endif

    return OK;
}

static int
init_fg_on_bg_case2(short int *pair_n)
{
#if LINUX
    FOREACH_FOREGROUND_ANSI() {
	FOREACH_BACKGROUND_EXTENDED() {
	    if (init_pair(++ (*pair_n), *fg, *bg) == ERR)
		return ERR;
	}
    }
#else
    (void) pair_n;
#endif

    return OK;
}

static int
init_extended_colors(short int *pair_n)
{
#if LINUX
    FOREACH_FOREGROUND_EXTENDED() {
	FOREACH_BACKGROUND_EXTENDED() {
	    if (*fg != *bg && init_pair(++ (*pair_n), *fg, *bg) == ERR)
		return ERR;
	}
    }
#else
    (void) pair_n;
#endif

    return OK;
}

static int
init_more_pairs(short int *pair_n)
{
    if (theme_bool_unparse("term_use_default_colors", true)) {
	for (const short int *psi = &ext_colors[0]; psi < &ext_colors[numExtended]; psi++) {
	    if (init_pair(++ (*pair_n), *psi, -1) == ERR)
		return ERR;
	}
    }

    if (init_fg_on_bg_case1(pair_n) == ERR ||
	init_fg_on_bg_case2(pair_n) == ERR)
	return ERR;

    return init_extended_colors(pair_n);
}

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

    if (COLORS >= 16 && can_change_color()) {
	if (init_color(GREY, 498,498,498) == ERR)
	    err_log(0, "init_color_pairs: init_color: GREY error");
	if (init_color(LIGHT_GREY, 824,824,824) == ERR)
	    err_log(0, "init_color_pairs: init_color: LIGHT_GREY error");
	initcolors();
    }

    /* Initialize black on black */
    if (init_pair(++pair_n, colors[0], colors[0]) == ERR) {
	err_msg("Could not initialize pair %hd", pair_n);
	return -1;
    }

    /* Initialize a color on the default background of the terminal */
    if (theme_bool_unparse("term_use_default_colors", true)) {
	for (const short int *psi = &colors[0];
	     psi < &colors[COLORS >= 16 && can_change_color() ? numColors : 8];
	     psi++) {
	    if (init_pair(++pair_n, *psi, -1) == ERR) {
		err_msg("Could not initialize pair %hd", pair_n);
		return -1;
	    }
	}
    }

    FOREACH_FOREGROUND_ANSI() {
	FOREACH_BACKGROUND_ANSI() {
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

    if (COLORS >= 256) {
#if UNIX
	if (init_more_pairs(&pair_n) == ERR)
	    return (pair_n - 1);
#elif WIN32
	FOREACH_FOREGROUND_EXTENDED() {
	    if (init_pair(++pair_n, *fg, COLOR_BLACK) == ERR)
		return (pair_n - 1);
	}
#endif
    }

    debug("init_color_pairs: all ok: %hd initialized pairs", pair_n);
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
