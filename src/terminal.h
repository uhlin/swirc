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
	    : rows(0)
	    , cols(0)
	    , start_row(0)
	    , start_col(0)
	{}

	term_window_size(int p_rows,
	    int p_cols,
	    int p_start_row,
	    int p_start_col)
	    : rows(p_rows)
	    , cols(p_cols)
	    , start_row(p_start_row)
	    , start_col(p_start_col)
	{
		/* null */;
	}
#endif
};

struct current_cursor_pos {
	int cury; /* row */
	int curx; /* col */

#ifdef __cplusplus
	current_cursor_pos()
	    : cury(0)
	    , curx(0)
	{}

	current_cursor_pos(const int row, const int col)
	    : cury(row)
	    , curx(col)
	{}
#endif
};

__SWIRC_BEGIN_DECLS
extern volatile bool g_resizing_term;

void	 term_init(void);
void	 term_deinit(void);

void	 term_beep(void);
struct current_cursor_pos
	 term_get_pos(WINDOW *) PTR_ARGS_NONNULL;
bool	 term_is_too_small(void);
PANEL	*term_new_panel(int rows, int cols, int start_row, int start_col);
void	 term_remove_panel(PANEL *);
void	 term_resize_all(void);
PANEL	*term_resize_panel(PANEL *, const struct term_window_size *);
__SWIRC_END_DECLS

#endif
