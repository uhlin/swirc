/* Read user input
   Copyright (C) 2012-2018 Markus Uhlin. All rights reserved.

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

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
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
#include "io-loop.h"
#include "libUtils.h"
#include "printtext.h"
#include "readline.h"
#include "readlineAPI.h"
#include "strHand.h"
#include "terminal.h"

#include "commands/misc.h"

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

/**
 * Get active panelwindow
 */
WINDOW *
readline_get_active_pwin(void)
{
    if (readline_pan1 == NULL || readline_pan2 == NULL)
	return NULL;

    return ((panel_state == PANEL1_ACTIVE)
	    ? panel_window(readline_pan1)
	    : panel_window(readline_pan2));
}

/**
 * Initiate a new readline session
 */
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

/**
 * Destroy readline session
 */
static void
session_destroy(volatile struct readline_session_context *ctx)
{
    if (ctx) {
	free(ctx->buffer);
	free(ctx->prompt);
	free((struct readline_session_context *) ctx);
    }
}

/**
 * Check if low limit is set
 */
static SW_INLINE bool
loLim_isset(WINDOW *win, int prompt_size)
{
    return (term_get_pos(win).curx <= prompt_size);
}

/**
 * Check if high limit is set
 */
static SW_INLINE bool
hiLim_isset(WINDOW *win)
{
    return (term_get_pos(win).curx >= COLS - 1);
}

/**
 * Write the command-prompt.
 */
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

/**
 * When swapping between panels: computes the new window entry.
 */
static void
compute_new_window_entry(volatile struct readline_session_context *ctx,
			 bool fwd)
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

/**
 * Swaps between the 2 panels depending on whether the low or high
 * limit is set.
 */
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

/**
 * Key left
 */
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

/**
 * Key right
 */
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

/**
 * Key backspace.
 */
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
	ctx->buffer[--ctx->n_insert] = 0L;
	yx = term_get_pos(ctx->act);

	if (wmove(ctx->act, yx.cury, yx.curx - 1) == ERR ||
	    wdelch(ctx->act) == ERR || wclrtoeol(ctx->act) == ERR) {
	    readline_error(EPERM, "wmove, wdelch or wclrtoeol");
	}

	readline_winsnstr(ctx->act, &ctx->buffer[ctx->bufpos], -1);
    } else { /* not insert_mode */
	ctx->buffer[--ctx->bufpos] = 0L;
	ctx->n_insert--;
	yx = term_get_pos(ctx->act);

	if (wmove(ctx->act, yx.cury, yx.curx - 1) == ERR ||
	    wdelch(ctx->act) == ERR) {
	    readline_error(EPERM, "wmove or wdelch");
	}
    }

    update_panels();
    (void) doupdate();
}

/**
 * Handles what happens if the delete key is pressed.
 */
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
    ctx->buffer[--ctx->n_insert] = 0L;

    if (wdelch(ctx->act) == ERR || wclrtoeol(ctx->act) == ERR) {
	readline_error(EPERM, "wdelch or wclrtoeol");
    }

    readline_winsnstr(ctx->act, &ctx->buffer[ctx->bufpos], -1);

    update_panels();
    (void) doupdate();
}

/**
 * Regular handling of a key-press.
 */
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

/**
 * Convert a wide-character-string to a multibyte-character-string.
 *
 * @param buf In data
 * @return The converted part (dynamically allocated)
 */
#if defined(UNIX)
/* ---- */
/* UNIX */
/* ---- */
static char *
finalize_out_string(const wchar_t *buf)
{
    const size_t CONVERT_FAILED = (size_t) -1;
    const size_t SZ = size_product(wcslen(buf), MB_LEN_MAX) + 1;
    char *out = xmalloc(SZ);
    size_t bytes_convert = 0;

    errno = 0;
    if ((bytes_convert = wcstombs(out, buf, SZ - 1)) == CONVERT_FAILED) {
	err_log(errno, "in finalize_out_string: wcstombs failed");
	BZERO(out, SZ);
	return out;
    } else if (bytes_convert == SZ - 1)
	out[SZ - 1] = '\0';
    return out;
}
#elif defined(WIN32)
/* ----- */
/* WIN32 */
/* ----- */
static char *
finalize_out_string(const wchar_t *buf)
{
    const int sz = size_to_int(size_product(wcslen(buf), MB_LEN_MAX) + 1);
    char *out = xmalloc(sz);

    errno = 0;
    if (WideCharToMultiByte(CP_UTF8, 0, buf, -1, out, sz, NULL, NULL) > 0)
	return out;
    err_log(errno, "in finalize_out_string: WideCharToMultiByte failed");
    BZERO(out, sz);
    return out;
}
#endif

/**
 * Apply window-options to the readline panels.
 *
 * @param win Window
 * @return Void
 */
static void
apply_readline_options(WINDOW *win)
{
#define KEYPAD(win, b)   ((void) keypad(win, b))
#define NODELAY(win, b)  ((void) nodelay(win, b))
#define SCROLLOK(win, b) ((void) scrollok(win, b))
    if (!is_keypad(win)) {
	KEYPAD(win, 1);
    }
    if (is_nodelay(win)) {
	NODELAY(win, 0);
    }
    if (is_scrollok(win)) {
	SCROLLOK(win, 0);
    }
}

/**
 * Read user input.
 *
 * @param prompt Prompt
 * @return The read line (dynamically allocated) or NULL
 */
char *
readline(const char *prompt)
{
    char *out;
    const int sleep_time_milliseconds = 90;
    volatile struct readline_session_context *ctx;
    wchar_t *buf_p = &g_push_back_buf[0];

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

	if (*buf_p) {
	    wc = *buf_p++;
	} else if (wget_wch(ctx->act, &wc) == ERR) {
	    (void) napms(sleep_time_milliseconds);
	    continue;
	}

	mutex_lock(&g_puts_mutex);

	switch (wc) {
	case CTRL_A:
	    while (ctx->bufpos != 0) {
		case_key_left(ctx);
		ctx->insert_mode = (ctx->bufpos != ctx->n_insert);
	    }
	    break;
	case CTRL_E:
	    while (ctx->insert_mode) {
		case_key_right(ctx);
		ctx->insert_mode = (ctx->bufpos != ctx->n_insert);
	    }
	    break;
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
	case KEY_F(5): case BLINK:
	    handle_key(ctx, btowc(BLINK));
	    break;
	case KEY_F(6): case BOLD_ALIAS:
	    handle_key(ctx, btowc(BOLD));
	    break;
	case KEY_F(7): case COLOR:
	    handle_key(ctx, btowc(COLOR));
	    break;
	case KEY_F(8): case NORMAL:
	    handle_key(ctx, btowc(NORMAL));
	    break;
	case KEY_F(9): case REVERSE:
	    handle_key(ctx, btowc(REVERSE));
	    break;
	case KEY_F(10): case UNDERLINE:
	    handle_key(ctx, btowc(UNDERLINE));
	    break;
	case KEY_F(11):
	    cmd_close("");
	    break;
	case KEY_F(12):
	    window_close_all_priv_conv();
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
	case '\n': case KEY_ENTER: case WINDOWS_KEY_ENTER:
	    g_readline_loop = false;
	    break;
	case KEY_RESIZE:
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
	ctx->buffer[ctx->n_insert] = 0L;
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

/**
 * De-initialize readline
 */
void
readline_deinit(void)
{
    term_remove_panel(readline_pan1);
    term_remove_panel(readline_pan2);
}

/**
 * Initialize readline (done before usage)
 */
void
readline_init(void)
{
    readline_pan1 = term_new_panel(1, 0, LINES - 1, 0);
    readline_pan2 = term_new_panel(1, 0, LINES - 1, 0);

    apply_readline_options(panel_window(readline_pan1));
    apply_readline_options(panel_window(readline_pan2));

    disable_beeps = config_bool_unparse("disable_beeps", false);
}

/**
 * Recreate the 2 readline panels with respect to the screen size.
 *
 * @param rows Size of the screen in rows
 * @param cols Size of the screen in cols
 * @return Void
 */
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

/**
 * Puts the currently active readline panel on the top of all panels
 * in the stack.
 */
void
readline_top_panel(void)
{
    if (panel_state == PANEL1_ACTIVE) {
	if (readline_pan1)
	    (void) top_panel(readline_pan1);
    } else if (panel_state == PANEL2_ACTIVE) {
	if (readline_pan2)
	    (void) top_panel(readline_pan2);
    } else {
	sw_assert_not_reached();
    }

    update_panels();
    (void) doupdate();
}
