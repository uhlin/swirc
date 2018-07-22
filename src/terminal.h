#ifndef TERMINAL_H
#define TERMINAL_H

#if defined(UNIX)
#include "term-unix.h"
#elif defined(WIN32)
#include "term-w32.h"
#endif

struct term_window_size {
    int rows;
    int cols;
    int start_row;
    int start_col;
};

struct current_cursor_pos {
    int cury; /* row */
    int curx; /* col */
};

/*lint -sem(term_new_panel, pure) */

#ifdef __cplusplus
extern "C" {
#endif

PANEL	*term_new_panel(int rows, int cols, int start_row, int start_col) PURE;
PANEL	*term_resize_panel(PANEL *, struct term_window_size *);

struct current_cursor_pos term_get_pos(WINDOW *);

void    term_beep(void);
void    term_deinit(void);
void    term_init(void);
void    term_remove_panel(PANEL *);
void    term_resize_all(void);

#ifdef __cplusplus
}
#endif

#endif
