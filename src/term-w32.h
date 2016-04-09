#ifndef TERM_W32_H
#define TERM_W32_H

#include <windows.h>

struct winsize {
    SHORT ws_row;
    SHORT ws_col;
};

/*lint -printf(1, term_set_title) */

struct winsize	term_get_size(void);
void		term_restore_title(void);
void		term_set_title(const char *fmt, ...);

/* Inline function definitions
   =========================== */

#if OS_X || BSD
#include <curses.h>
#elif LINUX
#include <ncursesw/curses.h>
#elif WIN32
#include "pdcurses/curses.h"
#else
#error "Cannot determine curses header file!"
#endif

static SW_INLINE void
term_set_attr(WINDOW *win, chtype at)
{
    if (win != NULL) {
	win->_attrs = at;
    }
}

#endif
