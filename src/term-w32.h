#ifndef TERM_W32_H
#define TERM_W32_H

#include <windows.h>

struct winsize {
	SHORT	ws_row;
	SHORT	ws_col;
};

/*lint -printf(1, term_set_title) */

__SWIRC_BEGIN_DECLS
bool		is_term_resized(int, int);
struct winsize	term_get_size(void);
void		term_restore_title(void);
void		term_set_title(const char *, ...);
__SWIRC_END_DECLS

/* Inline function definitions
   =========================== */

#if defined(PANEL_HDR)
#include PANEL_HDR
#elif UNIX
#include <panel.h>
#elif WIN32
#include "pdcurses/panel.h"
#else
#error Cannot determine panel header file!
#endif

static SW_INLINE void
term_set_attr(WINDOW *win, chtype at)
{
	if (win != NULL)
		win->_attrs = at;
}

#endif
