#ifndef READLINE_API_H
#define READLINE_API_H

#if defined(CURSES_HDR)
#include CURSES_HDR
#elif UNIX
#include <curses.h>
#elif WIN32
#include "pdcurses/curses.h"
#else
#error Cannot determine Curses header file!
#endif

//lint -printf(2, readline_ferror)
//lint -sem(readline_error, r_no) doesn't return because of longjmp()
//lint -sem(readline_ferror, r_no) likewise

__SWIRC_BEGIN_DECLS
NORETURN void readline_error(int, CSTRING);
NORETURN void readline_ferror(int, CSTRING, ...) PRINTFLIKE(2);

void	readline_mvwaddch(WINDOW *, int row, int col, wint_t);
void	readline_mvwinsch(WINDOW *, int row, int col, wint_t);
void	readline_waddch(WINDOW *, wint_t);
void	readline_waddnstr(WINDOW *, const wchar_t *, ptrdiff_t)
	    NONNULL;
void	readline_winsch(WINDOW *, wint_t);
void	readline_winsnstr(WINDOW *, const wchar_t *, ptrdiff_t)
	    NONNULL;

int	readline_wcwidth(const wchar_t, const int);
int	readline_wcswidth(const wchar_t *, const int);
__SWIRC_END_DECLS

#endif
