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

#ifdef __cplusplus
	term_window_size()
	{
		this->rows = 0;
		this->cols = 0;
		this->start_row = 0;
		this->start_col = 0;
	}

	term_window_size(int rows, int cols, int start_row, int start_col)
	{
		this->rows = rows;
		this->cols = cols;
		this->start_row = start_row;
		this->start_col = start_col;
	}
#endif
};

struct current_cursor_pos {
	int cury; /* row */
	int curx; /* col */
};

__SWIRC_BEGIN_DECLS
extern volatile bool g_resizing_term;

void	 term_init(void);
void	 term_deinit(void);

void	 term_beep(void);
struct current_cursor_pos
	 term_get_pos(WINDOW *);
bool	 term_is_too_small(void);
PANEL	*term_new_panel(int rows, int cols, int start_row, int start_col);
void	 term_remove_panel(PANEL *);
void	 term_resize_all(void);
PANEL	*term_resize_panel(PANEL *, const struct term_window_size *);
__SWIRC_END_DECLS

#endif
