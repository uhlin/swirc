#ifndef TERM_UNIX_H
#define TERM_UNIX_H

#if SOLARIS
#define BSD_COMP 1
#endif

#include <sys/ioctl.h> /* want winsize */

/*lint -printf(1, term_set_title) */

struct winsize	term_get_size(void);
void		term_restore_title(void);
void		term_set_title(const char *fmt, ...);

/* Inline function definitions
   =========================== */

#if OS_X || BSD
#include <panel.h>
#elif LINUX
#include <ncursesw/panel.h>
#elif WIN32
#include "pdcurses/panel.h"
#else
#error "Cannot determine panel header file!"
#endif

static SW_INLINE void
term_set_attr(WINDOW *win, attr_t at)
{
    if (win != NULL) {
	win->_attrs = at;
    }
}

#endif
