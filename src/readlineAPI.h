#ifndef READLINE_API_H
#define READLINE_API_H

#if defined(CURSES_HDR)
#include CURSES_HDR
#elif UNIX
#include <curses.h>
#elif WIN32
#include "pdcurses/curses.h"
#else
#error "Cannot determine curses header file!"
#endif

/*lint -sem(readline_error, r_no) doesn't return because of longjmp() */

SW_NORET void readline_error(int error, const char *msg);

void	readline_mvwaddch (WINDOW *, int row, int col, wint_t wc);
void	readline_mvwinsch (WINDOW *, int row, int col, wint_t wc);
void	readline_waddch   (WINDOW *, wint_t wc);
void	readline_waddnstr (WINDOW *, const wchar_t *s, ptrdiff_t n);
void	readline_winsch   (WINDOW *, wint_t wc);
void	readline_winsnstr (WINDOW *, const wchar_t *s, ptrdiff_t n);

#endif
