/* Read user input
   Copyright (C) 2012-2017 Markus Uhlin. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   - Neither the name of the author nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"

#include <limits.h>
#include <wctype.h>

#include "assertAPI.h"
#include "config.h"
#if defined(WIN32) && defined(PDC_EXP_EXTRAS)
#include "curses-funcs.h" /* is_scrollok() etc */
#endif
#include "errHand.h"
#include "libUtils.h"
#include "printtext.h"
#include "readline.h"
#include "readlineAPI.h"
#include "strHand.h"
#include "terminal.h"

/* Enum definitions
   ================ */

enum readline_active_panel {
    PANEL1_ACTIVE,
    PANEL2_ACTIVE
};

/* Objects with external linkage
   ============================= */

jmp_buf g_readline_loc_info;
bool    g_readline_loop;
bool    g_resize_requested;

bool g_hist_next = false;
bool g_hist_prev = false;

/* Objects with internal linkage
   ============================= */

static PANEL				*readline_pan1       = NULL;
static PANEL				*readline_pan2       = NULL;
static bool				 disable_beeps       = false;
static const int			 readline_buffersize = 2700;
static enum readline_active_panel	 panel_state	     = PANEL1_ACTIVE;

static struct readline_session_context *
new_session(const char *prompt)
{
    struct readline_session_context *ctx = xcalloc(sizeof *ctx, 1);

    ctx->buffer       = xcalloc(readline_buffersize + 1, sizeof (wchar_t));
    ctx->bufpos       = 0;
    ctx->n_insert     = 0;
    ctx->insert_mode  = false;
    ctx->no_bufspc    = false;
    ctx->prompt       = sw_strdup(prompt);
    ctx->prompt_size  = (int) strlen(prompt);
    ctx->act          = panel_window(readline_pan1);

    panel_state = PANEL1_ACTIVE;
    readline_top_panel();

    return ctx;
}

static void
session_destroy(volatile struct readline_session_context *ctx)
{
    free_not_null(ctx->buffer);
    free_not_null(ctx->prompt);
    if (ctx)
	free((struct readline_session_context *) ctx);
}

static SW_INLINE bool
loLim_isset(WINDOW *win, int prompt_size)
{
    return (term_get_pos(win).curx <= prompt_size);
}

static SW_INLINE bool
hiLim_isset(WINDOW *win)
{
    return (term_get_pos(win).curx >= COLS - 1);
}

static void
write_cmdprompt(WINDOW *win, char *prompt, int size)
{
    if (werase(win) == ERR) {
	readline_error(0, "werase");
    }

    if (waddnstr(win, prompt, size) == ERR) {
	readline_error(0, "waddnstr");
    }

    update_panels();
    (void) doupdate();
}

static void
compute_new_window_entry(volatile struct readline_session_context *ctx, bool fwd)
{
    int		 diff, buf_index;
    wchar_t	*str1, *str2;

    if (fwd) {
	diff	  = COLS / 2;
	buf_index = int_diff(ctx->bufpos, diff);

	if (COLS % 2 == 0) {
	    buf_index += 1;
	}

	str1 = &ctx->buffer[buf_index];
	str2 = &ctx->buffer[ctx->bufpos];
    } else {
	diff = int_diff(COLS / 2, ctx->prompt_size);

	if ((buf_index = int_diff(ctx->bufpos, diff)) < 0) {
	    readline_error(ERANGE, "compute_new_window_entry");
	}

	str1 = &ctx->buffer[buf_index];
	str2 = &ctx->buffer[ctx->bufpos];
    }

    if (ctx->insert_mode) {
	if (ctx->bufpos > 0) {
	    readline_waddnstr(ctx->act, str1, str2 - str1);
	}

	readline_winsnstr(ctx->act, str2, -1);
    } else {
	readline_waddnstr(ctx->act, str1, -1);
    }

    update_panels();
    (void) doupdate();
}

static void
magic_swap_panels(volatile struct readline_session_context *ctx, bool fwd)
{
    if (panel_state == PANEL1_ACTIVE) {
	ctx->act = panel_window(readline_pan2);
	(void) top_panel(readline_pan2);
	panel_state = PANEL2_ACTIVE;
    } else if (panel_state == PANEL2_ACTIVE) {
	ctx->act = panel_window(readline_pan1);
	(void) top_panel(readline_pan1);
	panel_state = PANEL1_ACTIVE;
    } else {
	sw_assert_not_reached();
    }

    write_cmdprompt(ctx->act, ctx->prompt, ctx->prompt_size);
    compute_new_window_entry(ctx, fwd);
}

static void
case_key_left(volatile struct readline_session_context *ctx)
{
    struct current_cursor_pos yx;

    if (ctx->bufpos == 0) {
	if (!disable_beeps) {
	    term_beep();
	}

	return;
    }

    if (loLim_isset(ctx->act, ctx->prompt_size)) {
	magic_swap_panels(ctx, false);
    }

    ctx->bufpos--;
    yx = term_get_pos(ctx->act);

    if (wmove(ctx->act, yx.cury, yx.curx - 1) == ERR) {
	readline_error(EPERM, "wmove");
    }

    update_panels();
    (void) doupdate();
}

static void
case_key_right(volatile struct readline_session_context *ctx)
{
    struct current_cursor_pos yx;

    if (!ctx->insert_mode) {
	if (!disable_beeps) {
	    term_beep();
	}

	return;
    }

    if (hiLim_isset(ctx->act)) {
	magic_swap_panels(ctx, true);
    }

    ctx->bufpos++;
    yx = term_get_pos(ctx->act);

    if (wmove(ctx->act, yx.cury, yx.curx + 1) == ERR) {
	readline_error(EPERM, "wmove");
    }

    update_panels();
    (void) doupdate();
}

static void
case_key_backspace(volatile struct readline_session_context *ctx)
{
    struct current_cursor_pos yx;

    if (ctx->bufpos == 0) {
	if (!disable_beeps) {
	    term_beep();
	}

	return;
    }

    if (loLim_isset(ctx->act, ctx->prompt_size)) {
	magic_swap_panels(ctx, false);
    }

    if (ctx->insert_mode) {
	wchar_t *ptr = &ctx->buffer[ctx->bufpos--];

	(void) wmemmove(ptr - 1, ptr, wcslen(ptr));
	ctx->buffer[--ctx->n_insert] = 0;
	yx = term_get_pos(ctx->act);

	if (wmove(ctx->act, yx.cury, yx.curx - 1) == ERR || wdelch(ctx->act) == ERR || wclrtoeol(ctx->act) == ERR) {
	    readline_error(EPERM, "wmove, wdelch or wclrtoeol");
	}

	readline_winsnstr(ctx->act, &ctx->buffer[ctx->bufpos], -1);
    } else { /* not insert_mode */
	ctx->buffer[--ctx->bufpos] = 0;
	ctx->n_insert--;
	yx = term_get_pos(ctx->act);

	if (wmove(ctx->act, yx.cury, yx.curx - 1) == ERR || wdelch(ctx->act) == ERR) {
	    readline_error(EPERM, "wmove or wdelch");
	}
    }

    update_panels();
    (void) doupdate();
}

static void
case_key_dc(volatile struct readline_session_context *ctx)
{
    wchar_t	*ptr;
    const int	 this_index = ctx->bufpos + 1;

    if (!ctx->insert_mode) {
	if (!disable_beeps) {
	    term_beep();
	}

	return;
    }

    ptr = &ctx->buffer[this_index];
    (void) wmemmove(ptr - 1, ptr, wcslen(ptr));
    ctx->buffer[--ctx->n_insert] = 0;

    if (wdelch(ctx->act) == ERR || wclrtoeol(ctx->act) == ERR) {
	readline_error(EPERM, "wdelch or wclrtoeol");
    }

    readline_winsnstr(ctx->act, &ctx->buffer[ctx->bufpos], -1);

    update_panels();
    (void) doupdate();
}

static void
handle_key(volatile struct readline_session_context *ctx, wint_t wc)
{
    if (ctx->no_bufspc) {
	if (!disable_beeps) {
	    term_beep();
	}

	return;
    }

    if (hiLim_isset(ctx->act)) {
	magic_swap_panels(ctx, true);
    }

    if (ctx->insert_mode) {
	wchar_t				*ptr = &ctx->buffer[ctx->bufpos];
	struct current_cursor_pos	 yx;

	(void) wmemmove(ptr + 1, ptr, wcslen(ptr));
	*ptr = wc;
	ctx->bufpos++, ctx->n_insert++;
	readline_winsch(ctx->act, wc);
	yx = term_get_pos(ctx->act);

	if (wmove(ctx->act, yx.cury, yx.curx + 1) == ERR) {
	    readline_error(EPERM, "wmove");
	}
    } else {
	ctx->buffer[ctx->bufpos] = wc;
	ctx->bufpos++, ctx->n_insert++;
	readline_waddch(ctx->act, wc);
    }

    update_panels();
    (void) doupdate();
}

static char *
finalize_out_string(const wchar_t *buf)
{
    mbstate_t		 ps;
    size_t		 bytes_convert;
    const size_t	 size = size_product(wcslen(buf), MB_LEN_MAX) + 1;
    char		*out  = xmalloc(size);

    BZERO(&ps, sizeof (mbstate_t));

#ifdef HAVE_BCI
    if ((errno = wcsrtombs_s(&bytes_convert, out, size, &buf, size - 1, &ps)) != 0) {
	err_sys("wcsrtombs_s");
    }
#else
    if ((bytes_convert = wcsrtombs(out, &buf, size - 1, &ps)) == ((size_t) -1)) {
	err_sys("wcsrtombs");
    } else if (bytes_convert == size - 1) { /* unlikely */
	out[size - 1] = 0;
    } else {
	;
    }
#endif

    return (out);
}

static void
apply_readline_options(WINDOW *win)
{
#define KEYPAD(win, b)   ((void) keypad(win, b))
#define NODELAY(win, b)  ((void) nodelay(win, b))
#define SCROLLOK(win, b) ((void) scrollok(win, b))
    if (!is_keypad(win)) {
	KEYPAD(win, 1);
    }
    if (!is_nodelay(win)) {
	NODELAY(win, 1);
    }
    if (is_scrollok(win)) {
	SCROLLOK(win, 0);
    }
}

void
readline_init(void)
{
    readline_pan1 = term_new_panel(1, 0, LINES - 1, 0);
    readline_pan2 = term_new_panel(1, 0, LINES - 1, 0);

    apply_readline_options(panel_window(readline_pan1));
    apply_readline_options(panel_window(readline_pan2));

    disable_beeps = config_bool_unparse("disable_beeps", false);
}

void
readline_deinit(void)
{
    term_remove_panel(readline_pan1);
    term_remove_panel(readline_pan2);
}

char *
readline(const char *prompt)
{
    volatile struct readline_session_context *ctx;
    const int sleep_time_milliseconds = 90;
    char *out;

    ctx = new_session(prompt);
    if (setjmp(g_readline_loc_info) != 0) {
	session_destroy(ctx);
	mutex_unlock(&g_puts_mutex);
	return NULL;
    }

    g_readline_loop    = true;
    g_resize_requested = false;
    g_hist_next = false;
    g_hist_prev = false;

    mutex_lock(&g_puts_mutex);
    write_cmdprompt(ctx->act, ctx->prompt, ctx->prompt_size);
    mutex_unlock(&g_puts_mutex);

    do {
	wint_t wc;

	ctx->insert_mode = (ctx->bufpos != ctx->n_insert);
	ctx->no_bufspc	 = (ctx->n_insert + 1 >= readline_buffersize);

	if (wget_wch(ctx->act, &wc) == ERR) {
	    (void) napms(sleep_time_milliseconds);
	    continue;
	}

	mutex_lock(&g_puts_mutex);

	switch (wc) {
	case MY_KEY_DLE:
	    window_select_prev();
	    break; /* CTRL+P */
	case MY_KEY_SO:
	    window_select_next();
	    break; /* CTRL+N */
	case KEY_DOWN:
	    g_hist_next = true;
	    session_destroy(ctx);
	    mutex_unlock(&g_puts_mutex);
	    return NULL;
	case KEY_UP:
	    g_hist_prev = true;
	    session_destroy(ctx);
	    mutex_unlock(&g_puts_mutex);
	    return NULL;
	case KEY_LEFT: case MY_KEY_STX:
	    case_key_left(ctx);
	    break;
	case KEY_RIGHT: case MY_KEY_ACK:
	    case_key_right(ctx);
	    break;
	case KEY_BACKSPACE: case MY_KEY_BS:
	    case_key_backspace(ctx);
	    break;
	case KEY_F(5):
	    handle_key(ctx, btowc(BLINK));
	    break;
	case KEY_F(6):
	    handle_key(ctx, btowc(BOLD));
	    break;
	case KEY_F(7):
	    handle_key(ctx, btowc(COLOR));
	    break;
	case KEY_F(8):
	    handle_key(ctx, btowc(NORMAL));
	    break;
	case KEY_F(9):
	    handle_key(ctx, btowc(REVERSE));
	    break;
	case KEY_F(10):
	    handle_key(ctx, btowc(UNDERLINE));
	    break;
	case KEY_DC: case MY_KEY_EOT:
	    case_key_dc(ctx);
	    break;
	case KEY_NPAGE:
	    window_scroll_down(g_active_window);
	    break;
	case KEY_PPAGE:
	    window_scroll_up(g_active_window);
	    break;
	case '\t':
	    break;
	case KEY_ENTER: case '\n':
	    g_readline_loop = false;
	    break;
	case KEY_RESIZE:
	    break;
	case MY_KEY_RESIZE:
	    g_resize_requested = true;
	    /*FALLTHROUGH*/
	case '\a':
	    session_destroy(ctx);
	    mutex_unlock(&g_puts_mutex);
	    return NULL;
	default:
	    if (iswprint(wc)) {
		handle_key(ctx, wc);
	    }
	    break;
	}

	mutex_unlock(&g_puts_mutex);
    } while (g_readline_loop);

    if (ctx->n_insert > 0) {
	ctx->buffer[ctx->n_insert] = 0;
    } else {
	session_destroy(ctx);
	return NULL;
    }

    mutex_lock(&g_puts_mutex);
    write_cmdprompt(ctx->act, "", 0);
    mutex_unlock(&g_puts_mutex);

    out = finalize_out_string(ctx->buffer);
    session_destroy(ctx);

    return out;
}

void
readline_recreate(int rows, int cols)
{
    struct term_window_size newsize = {
	.rows      = 1,
	.cols      = cols,
	.start_row = rows - 1,
	.start_col = 0,
    };

    readline_pan1 = term_resize_panel(readline_pan1, &newsize);
    readline_pan2 = term_resize_panel(readline_pan2, &newsize);

    apply_readline_options(panel_window(readline_pan1));
    apply_readline_options(panel_window(readline_pan2));
}

void
readline_top_panel(void)
{
    if (panel_state == PANEL1_ACTIVE) {
	(void) top_panel(readline_pan1);
    } else if (panel_state == PANEL2_ACTIVE) {
	(void) top_panel(readline_pan2);
    } else {
	sw_assert_not_reached();
    }

    update_panels();
    (void) doupdate();
}
