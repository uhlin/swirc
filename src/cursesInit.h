#ifndef CURSES_INIT_H
#define CURSES_INIT_H

__SWIRC_BEGIN_DECLS
extern bool		g_no_colors;
extern short int	g_initialized_pairs;

/* returns OK or ERR */
int curses_init(void);
__SWIRC_END_DECLS

#endif
