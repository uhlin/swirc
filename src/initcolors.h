#ifndef INITCOLORS_H
#define INITCOLORS_H

#if defined(CURSES_HDR)
#include CURSES_HDR
#elif UNIX
#include <curses.h>
#elif WIN32
#include "pdcurses/curses.h"
#else
#error Cannot determine curses header file!
#endif

typedef struct {
    short int r;
    short int g;
    short int b;
} rgb_t;

__SWIRC_BEGIN_DECLS
void initcolors(void);
__SWIRC_END_DECLS

#endif
