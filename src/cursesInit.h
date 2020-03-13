#ifndef CURSES_INIT_H
#define CURSES_INIT_H

#define FOREACH_FOREGROUND_EXTENDED()\
	for (const short int *fg = &ext_colors[0]; fg < &ext_colors[numExtended]; fg++)
#define FOREACH_BACKGROUND_EXTENDED()\
	for (const short int *bg = &ext_colors[0]; bg < &ext_colors[numExtended]; bg++)

#define FOREACH_FOREGROUND_ANSI()\
	for (const short int *fg = &colors[0]; fg < &colors[numColors]; fg++)
#define FOREACH_BACKGROUND_ANSI()\
	for (const short int *bg = &colors[0]; bg < &colors[numColors]; bg++)

__SWIRC_BEGIN_DECLS
extern bool		g_no_colors;
extern short int	g_initialized_pairs;

/* returns OK or ERR */
int curses_init(void);
__SWIRC_END_DECLS

#endif
