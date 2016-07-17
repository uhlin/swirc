#ifndef CURSES_FUNCS_H
#define CURSES_FUNCS_H

#if defined(CURSES_HDR)
#include CURSES_HDR
#elif UNIX
#include <curses.h>
#elif WIN32
#include "pdcurses/curses.h"
#else
#error "Cannot determine curses header file!"
#endif

typedef int (*PTR_TO_ENDWIN)(void);
typedef int (*PTR_TO_DOUPDATE)(void);

extern bool		g_cursesMode;
extern PTR_TO_ENDWIN	g_endwin_fn;
extern PTR_TO_DOUPDATE	g_doupdate_fn;

void	escape_curses(void);
void	resume_curses(void);

#if defined(WIN32) && defined(PDC_EXP_EXTRAS)
bool	is_cleared  (const WINDOW *);
bool	is_leaveok  (const WINDOW *);
bool	is_scrollok (const WINDOW *);
bool	is_nodelay  (const WINDOW *);
bool	is_immedok  (const WINDOW *);
bool	is_syncok   (const WINDOW *);
bool	is_keypad   (const WINDOW *);
#endif

#endif
