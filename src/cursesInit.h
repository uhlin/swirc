#ifndef CURSES_INIT_H
#define CURSES_INIT_H

extern bool		g_no_colors;
extern short int	g_initialized_pairs;

#ifdef __cplusplus
extern "C" {
#endif

/* returns OK or ERR */
int curses_init(void);

#ifdef __cplusplus
}
#endif

#endif
