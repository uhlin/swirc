/* Read user input
   Copyright (C) 2012-2022 Markus Uhlin. All rights reserved.

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
#include <locale.h>
#include <wctype.h>

#include "assertAPI.h"
#include "atomicops.h"
#include "config.h"
#if defined(WIN32) && defined(PDC_EXP_EXTRAS)
#include "curses-funcs.h" /* is_scrollok() etc */
#endif
#include "dataClassify.h"
#include "errHand.h"
#include "io-loop.h"
#include "libUtils.h"
#include "log.h"
#include "nicklist.h"
#include "printtext.h"
#include "readline.h"
#include "readlineAPI.h"
#include "readlineTabCompletion.h"
#include "statusbar.h"
#include "strHand.h"
#include "terminal.h"

#include "commands/misc.h"

/****************************************************************
*                                                               *
*  -------------- Objects with external linkage --------------  *
*                                                               *
****************************************************************/

PREADLINE_POS g_readline_pos = NULL;

bool		g_readline_loop = false;
bool		g_resize_requested = false;
jmp_buf		g_readline_loc_info;

bool	g_hist_next = false;
bool	g_hist_prev = false;

/****************************************************************
*                                                               *
*  -------------- Objects with internal linkage --------------  *
*                                                               *
****************************************************************/

static PANEL			*readline_pan1 = NULL;
static PANEL			*readline_pan2 = NULL;
static const int		 readline_bufsize = 2000;
static rl_active_panel_t	 panel_state = PANEL1_ACTIVE;

/****************************************************************
*                                                               *
*  ---------------------    Functions    ---------------------  *
*                                                               *
****************************************************************/

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

#if defined(UNIX)
#define WAIT_TIME_MILLISEC 100
#elif defined(WIN32)
#define WAIT_TIME_MILLISEC 0
#endif

	wtimeout(win, WAIT_TIME_MILLISEC);
}

/**
 * Check if low limit is set
 */
static inline bool
loLim_isset(WINDOW *win, int prompt_size)
{
	return (term_get_pos(win).curx <= prompt_size);
}

/**
 * Check if high limit is set
 */
static inline bool
hiLim_isset(WINDOW *win)
{
	return (term_get_pos(win).curx >= (COLS - 1));
}

/**
 * Write the command-prompt.
 */
static void
write_cmdprompt(WINDOW *win, const char *prompt, int size)
{
	int ret;

	mutex_lock(&g_puts_mutex);
	ret = werase(win);
	mutex_unlock(&g_puts_mutex);

	if (ret == ERR)
		readline_error(0, "write_cmdprompt: werase");

	printtext_puts(win, prompt, -1, -1, NULL);
	(void) size; /* unused. provided for compatibility. */
}

/**
 * When swapping between panels: computes the new window entry.
 */
static void
compute_new_window_entry(const volatile struct readline_session_context *ctx,
			 bool fwd)
{
	int bufindex, diff;
	wchar_t *str1, *str2;

	if (fwd) {
		diff = (COLS / 2);
		bufindex = int_diff(ctx->bufpos, diff);

		if ((COLS % 2) == 0)
			bufindex += 1;

		str1 = &ctx->buffer[bufindex];
		str2 = &ctx->buffer[ctx->bufpos];
	} else {
		diff = int_diff(COLS / 2, ctx->prompt_size);

		if ((bufindex = int_diff(ctx->bufpos, diff)) < 0) {
			readline_error(ERANGE, "compute_new_window_entry");
			/* NOTREACHED */
		}

		str1 = &ctx->buffer[bufindex];
		str2 = &ctx->buffer[ctx->bufpos];
	}

	if (ctx->insert_mode) {
		if (ctx->bufpos > 0)
			readline_waddnstr(ctx->act, str1, (str2 - str1));
		readline_winsnstr(ctx->act, str2, -1);
	} else {
		readline_waddnstr(ctx->act, str1, -1);
	}

	mutex_lock(&g_puts_mutex);
	(void) wrefresh(ctx->act);
	mutex_unlock(&g_puts_mutex);
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
 * Key backspace.
 */
static void
case_key_backspace(volatile struct readline_session_context *ctx)
{
	int		 ret[3];
	wchar_t		*ptr;

	if (ctx->bufpos == 0) {
		term_beep();
		return;
	}
	if (loLim_isset(ctx->act, ctx->prompt_size))
		magic_swap_panels(ctx, false);
	if (ctx->insert_mode) {
		ptr = &ctx->buffer[ctx->bufpos--];
		(void) wmemmove(ptr - 1, ptr, wcslen(ptr));
		ctx->buffer[--ctx->n_insert] = 0L;

		mutex_lock(&g_puts_mutex);
		ret[0] = wmove(ctx->act, term_get_pos(ctx->act).cury,
		    term_get_pos(ctx->act).curx - 1);
		ret[1] = wdelch(ctx->act);
		ret[2] = wclrtoeol(ctx->act);
		mutex_unlock(&g_puts_mutex);

		if (ret[0] == ERR)
			readline_error(EIO, "case_key_backspace: wmove");
		else if (ret[1] == ERR)
			readline_error(EIO, "case_key_backspace: wdelch");
		else if (ret[2] == ERR)
			readline_error(EIO, "case_key_backspace: wclrtoeol");

		readline_winsnstr(ctx->act, &ctx->buffer[ctx->bufpos], -1);
	} else {
		/*
		 * Not insert mode
		 */

		ctx->buffer[--ctx->bufpos] = 0L;
		ctx->n_insert--;

		mutex_lock(&g_puts_mutex);
		ret[0] = wmove(ctx->act, term_get_pos(ctx->act).cury,
		    term_get_pos(ctx->act).curx - 1);
		ret[1] = wdelch(ctx->act);
		mutex_unlock(&g_puts_mutex);

		if (ret[0] == ERR)
			readline_error(EIO, "case_key_backspace: wmove");
		else if (ret[1] == ERR)
			readline_error(EIO, "case_key_backspace: wdelch");
	}

	mutex_lock(&g_puts_mutex);
	(void) wrefresh(ctx->act);
	mutex_unlock(&g_puts_mutex);
}

/**
 * Handles what happens if the delete key is pressed.
 */
static void
case_key_dc(volatile struct readline_session_context *ctx)
{
	const int	 i = ctx->bufpos + 1;
	int		 ret[2];
	wchar_t		*ptr;

	if (!ctx->insert_mode) {
		term_beep();
		return;
	}

	ptr = &ctx->buffer[i];
	(void) wmemmove(ptr - 1, ptr, wcslen(ptr));
	ctx->buffer[--ctx->n_insert] = 0L;

	mutex_lock(&g_puts_mutex);
	ret[0] = wdelch(ctx->act);
	ret[1] = wclrtoeol(ctx->act);
	mutex_unlock(&g_puts_mutex);

	if (ret[0] == ERR)
		readline_error(EIO, "case_key_dc: wdelch");
	else if (ret[1] == ERR)
		readline_error(EIO, "case_key_dc: wclrtoeol");

	readline_winsnstr(ctx->act, &ctx->buffer[ctx->bufpos], -1);

	mutex_lock(&g_puts_mutex);
	(void) wrefresh(ctx->act);
	mutex_unlock(&g_puts_mutex);
}

/**
 * Key left
 */
static void
case_key_left(volatile struct readline_session_context *ctx)
{
	struct current_cursor_pos yx;

	if (ctx->bufpos == 0) {
		term_beep();
		return;
	}

	if (loLim_isset(ctx->act, ctx->prompt_size))
		magic_swap_panels(ctx, false);

	mutex_lock(&g_puts_mutex);
	ctx->bufpos--;
	yx = term_get_pos(ctx->act);
	if (wmove(ctx->act, yx.cury, yx.curx - 1) == ERR) {
		mutex_unlock(&g_puts_mutex);
		readline_error(EIO, "wmove");
		/* NOTREACHED */
	}
	(void) wrefresh(ctx->act);
	mutex_unlock(&g_puts_mutex);
}

/**
 * Key right
 */
static void
case_key_right(volatile struct readline_session_context *ctx)
{
	struct current_cursor_pos yx;

	if (!ctx->insert_mode) {
		term_beep();
		return;
	}

	if (hiLim_isset(ctx->act))
		magic_swap_panels(ctx, true);

	mutex_lock(&g_puts_mutex);
	ctx->bufpos++;
	yx = term_get_pos(ctx->act);
	if (wmove(ctx->act, yx.cury, yx.curx + 1) == ERR) {
		mutex_unlock(&g_puts_mutex);
		readline_error(EIO, "wmove");
		/* NOTREACHED */
	}
	(void) wrefresh(ctx->act);
	mutex_unlock(&g_puts_mutex);
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
	const size_t	 size = size_product(wcslen(buf) + 1, MB_LEN_MAX);
	char		*out = xmalloc(size);
	size_t		 bytes_convert;

	errno = 0;

	if ((bytes_convert = wcstombs(out, buf, size - 1)) ==
	    g_conversion_failed) {
		err_log(errno, "finalize_out_string: wcstombs");
		BZERO(out, size);
		return out;
	} else if (bytes_convert == (size - 1)) {
		out[size - 1] = '\0';
	}

	return out;
}
#elif defined(WIN32)
/* ----- */
/* WIN32 */
/* ----- */
static char *
finalize_out_string(const wchar_t *buf)
{
	const int size = size_to_int(size_product(wcslen(buf) + 1, MB_LEN_MAX));
	char *out = xmalloc(size);

	errno = 0;

	if (WideCharToMultiByte(CP_UTF8, 0, buf, -1, out, size, NULL, NULL) > 0)
		return out;

	err_log(errno, "finalize_out_string: WideCharToMultiByte");
	BZERO(out, size);
	return out;
}
#endif

/**
 * Regular handling of a key-press.
 */
static void
handle_key(volatile struct readline_session_context *ctx, wint_t wc)
{
	if (ctx->no_bufspc) {
		term_beep();
		return;
	}
	if (hiLim_isset(ctx->act))
		magic_swap_panels(ctx, true);
	if (ctx->insert_mode) {
		int		 ret;
		wchar_t		*ptr;

		ptr = &ctx->buffer[ctx->bufpos];
		(void) wmemmove(ptr + 1, ptr, wcslen(ptr));
		*ptr = wc;

		ctx->bufpos++;
		ctx->n_insert++;

		readline_winsch(ctx->act, wc);

		mutex_lock(&g_puts_mutex);
		ret = wmove(ctx->act, term_get_pos(ctx->act).cury,
		    term_get_pos(ctx->act).curx + 1);
		mutex_unlock(&g_puts_mutex);

		if (ret == ERR)
			readline_error(0, "handle_key: wmove");
	} else {
		ctx->buffer[ctx->bufpos] = wc;
		ctx->bufpos++;
		ctx->n_insert++;
		readline_waddch(ctx->act, wc);
	}
	mutex_lock(&g_puts_mutex);
	(void) wrefresh(ctx->act);
	mutex_unlock(&g_puts_mutex);
}

static void
handle_mouse(void)
{
	MEVENT	mouse;

	if (getmouse(&mouse) != OK)
		return;
	else if (mouse.bstate & BUTTON4_PRESSED)
		window_scroll_up(g_active_window, 1);
#ifdef BUTTON5_PRESSED
	else if (mouse.bstate & BUTTON5_PRESSED)
		window_scroll_down(g_active_window, 1);
#else
	else if (mouse.bstate == 0x8000000)
		window_scroll_down(g_active_window, 1);
#endif
}

static inline bool
isInCirculationMode(const TAB_COMPLETION *tc)
{
	return (tc->isInCirculationModeForHelp ||
	    tc->isInCirculationModeForQuery ||
	    tc->isInCirculationModeForSettings ||
	    tc->isInCirculationModeForWhois ||
	    tc->isInCirculationModeForZncCmds ||
	    tc->isInCirculationModeForCmds ||
	    tc->isInCirculationModeForChanUsers);
}

/**
 * Initiate a new readline session
 */
static struct readline_session_context *
new_session(const char *prompt)
{
	char *prompt_copy = sw_strdup(prompt);
	struct readline_session_context *ctx = xcalloc(sizeof *ctx, 1);

	ctx->act         = panel_window(readline_pan1);
	ctx->buffer      = xcalloc(readline_bufsize + 1, sizeof(wchar_t));
	ctx->bufpos      = 0;
	ctx->insert_mode = false;
	ctx->n_insert    = 0;
	ctx->no_bufspc   = false;
	ctx->prompt      = sw_strdup(prompt);
	ctx->prompt_size = (int) strlen(squeeze_text_deco(prompt_copy));
	ctx->tc          = readline_tab_comp_ctx_new();

	free(prompt_copy);

	panel_state = PANEL1_ACTIVE;
	readline_top_panel();
	return ctx;
}

static void
output_help(void)
{
	PRINTTEXT_CONTEXT ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC_NONE, true);

	printtext(&ctx, "--------------- Keys ---------------");
	printtext(&ctx, " ");
	printtext(&ctx, "CTRL+a     Move to beginning of line");
	printtext(&ctx, "CTRL+e     Move to end of line");
	printtext(&ctx, "CTRL+b     Move cursor backward");
	printtext(&ctx, "CTRL+f     Move cursor forward");
	printtext(&ctx, "CTRL+d     Delete");
	printtext(&ctx, "CTRL+g     Clear readline input");
	printtext(&ctx, "CTRL+l     Toggle logging on/off");
	printtext(&ctx, "CTRL+n     Next window");
	printtext(&ctx, "CTRL+p     Previous window");
	printtext(&ctx, "PG UP      Scroll up");
	printtext(&ctx, "PG DOWN    Scroll down");
	printtext(&ctx, "Up arrow   History previous");
	printtext(&ctx, "Down arrow History next");
	printtext(&ctx, "F3         Scroll nicklist up");
	printtext(&ctx, "F4         Scroll nicklist down");
	printtext(&ctx, "F11        Close window");
	printtext(&ctx, "F12        Close all private conversations");
	printtext(&ctx, " ");
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
		readline_tab_comp_ctx_destroy(ctx->tc);
		free((struct readline_session_context *) ctx);
	}
}

static char *
process(volatile struct readline_session_context *ctx)
{
	char *out;
	static const int sleep_time_milliseconds = 90;
	wchar_t *buf_p = &g_push_back_buf[0];

	write_cmdprompt(ctx->act, ctx->prompt, ctx->prompt_size);

	do {
		wint_t wc = 0L;

		ctx->insert_mode = (ctx->bufpos != ctx->n_insert);
		ctx->no_bufspc = (ctx->n_insert + 1 >= readline_bufsize);

		if (*buf_p == L'\0' && g_readline_pos != NULL) {
			if (g_readline_pos->x != ctx->bufpos ||
			    g_readline_pos->y != ctx->n_insert) {
				g_readline_pos->x = ctx->bufpos;
				g_readline_pos->y = ctx->n_insert;
				statusbar_update_display_beta();
				readline_top_panel();
			}
		}

		if (*buf_p != L'\0') {
			wc = *buf_p++;
		} else {
			int ret;

			mutex_lock(&g_puts_mutex);
			ret = wget_wch(ctx->act, &wc);
			mutex_unlock(&g_puts_mutex);

			if (ret == ERR) {
				(void) napms(sleep_time_milliseconds);
				continue;
			}
		}

		switch (wc) {
		case CTRL_A: {
			while (ctx->bufpos != 0) {
				case_key_left(ctx);
				ctx->insert_mode =
				    (ctx->bufpos != ctx->n_insert);
			}

			if (isInCirculationMode(ctx->tc))
				readline_tab_comp_ctx_reset(ctx->tc);
			break;
		} /* ---------- CTRL+A ---------- */
		case CTRL_E: {
			while (ctx->insert_mode) {
				case_key_right(ctx);
				ctx->insert_mode =
				    (ctx->bufpos != ctx->n_insert);
			}

			if (isInCirculationMode(ctx->tc))
				readline_tab_comp_ctx_reset(ctx->tc);
			break;
		} /* ---------- CTRL+E ---------- */
		case CTRL_L:
			log_toggle_on_off();
			break;
		case MY_KEY_DLE:
			window_select_prev();
			session_destroy(ctx);
			return NULL; /* CTRL+P */
		case MY_KEY_SO:
			window_select_next();
			session_destroy(ctx);
			return NULL; /* CTRL+N */
		case KEY_DOWN:
			g_hist_next = true;
			session_destroy(ctx);
			return NULL;
		case KEY_UP:
			g_hist_prev = true;
			session_destroy(ctx);
			return NULL;
		case KEY_LEFT:
		case MY_KEY_STX: {
			case_key_left(ctx);

			if (isInCirculationMode(ctx->tc))
				readline_tab_comp_ctx_reset(ctx->tc);
			break;
		} /* ---------- KEY_LEFT ---------- */
		case KEY_RIGHT:
		case MY_KEY_ACK: {
			case_key_right(ctx);

			if (isInCirculationMode(ctx->tc))
				readline_tab_comp_ctx_reset(ctx->tc);
			break;
		} /* ---------- KEY_RIGHT ---------- */
		case KEY_BACKSPACE:
		case MY_KEY_BS: {
			case_key_backspace(ctx);

			if (isInCirculationMode(ctx->tc))
				readline_tab_comp_ctx_reset(ctx->tc);
			break;
		} /* ---------- KEY_BACKSPACE ---------- */
		case KEY_F(1):
			output_help();
			break;
		case KEY_F(3):
			nicklist_scroll_up(g_active_window);
			break;
		case KEY_F(4):
			nicklist_scroll_down(g_active_window);
			break;
		case KEY_F(5):
		case BLINK:
			handle_key(ctx, btowc(BLINK));
			break;
		case KEY_F(6):
		case BOLD_ALIAS:
			handle_key(ctx, btowc(BOLD));
			break;
		case KEY_F(7):
		case COLOR:
			handle_key(ctx, btowc(COLOR));
			break;
		case KEY_F(8):
		case NORMAL:
			handle_key(ctx, btowc(NORMAL));
			break;
		case KEY_F(9):
		case REVERSE:
			handle_key(ctx, btowc(REVERSE));
			break;
		case KEY_F(10):
		case UNDERLINE:
			handle_key(ctx, btowc(UNDERLINE));
			break;
		case KEY_F(11):
			cmd_close("");
			session_destroy(ctx);
			return NULL;
		case KEY_F(12):
			window_close_all_priv_conv();
			session_destroy(ctx);
			return NULL;
		case KEY_DC:
		case MY_KEY_EOT: {
			case_key_dc(ctx);

			if (isInCirculationMode(ctx->tc))
				readline_tab_comp_ctx_reset(ctx->tc);
			break;
		} /* ---------- KEY_DC ---------- */
		case KEY_MOUSE:
			if (config_bool("mouse", false))
				handle_mouse();
			break;
		case KEY_NPAGE:
			window_scroll_down(g_active_window, -1);
			break;
		case KEY_PPAGE:
			window_scroll_up(g_active_window, -1);
			break;
		case '\t':
			readline_handle_tab(ctx);
			(void) napms(sleep_time_milliseconds);
			break;
		case '\n':
		case KEY_ENTER:
		case WINDOWS_KEY_ENTER:
			g_readline_loop = false;
			break;
		case KEY_RESIZE:
		case MY_KEY_RESIZE:
			g_resize_requested = true;
			/* FALLTHROUGH */
		case '\a':
			session_destroy(ctx);
			return NULL;
		default:
			if (iswprint(wc))
				handle_key(ctx, wc);
			break;
		}
	} while (g_readline_loop);

	write_cmdprompt(ctx->act, "", 0);

	if (ctx->n_insert > 0) {
		ctx->buffer[ctx->n_insert] = 0L;
	} else {
		session_destroy(ctx);
		return NULL;
	}

	out = finalize_out_string(ctx->buffer);
	session_destroy(ctx);
	return out;
}

static void
report_wheel_events(void)
{
#ifdef BUTTON5_PRESSED
	(void) mousemask((BUTTON4_PRESSED | BUTTON5_PRESSED), NULL);
#else
	(void) mousemask(BUTTON4_PRESSED, NULL);
#endif
}

/**
 * Initialize readline (done before usage)
 */
void
readline_init(void)
{
	g_readline_pos = xcalloc(sizeof *g_readline_pos, 1);
	g_readline_pos->x = -1;
	g_readline_pos->y = -1;

	readline_pan1 = term_new_panel(1, 0, LINES - 1, 0);
	readline_pan2 = term_new_panel(1, 0, LINES - 1, 0);

	apply_readline_options(panel_window(readline_pan1));
	apply_readline_options(panel_window(readline_pan2));

	readline_mouse_init();
}

/**
 * De-initialize readline
 */
void
readline_deinit(void)
{
	free(g_readline_pos);
	g_readline_pos = NULL;

	term_remove_panel(readline_pan1);
	term_remove_panel(readline_pan2);
}

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
 * Read user input.
 *
 * @param prompt Prompt
 * @return The read line (dynamically allocated) or NULL
 */
char *
readline(const char *prompt)
{
	volatile struct readline_session_context *ctx = NULL;

	switch (setjmp(g_readline_loc_info)) {
	case READLINE_PROCESS:
		g_readline_loop = true;
		g_resize_requested = false;
		g_hist_next = false;
		g_hist_prev = false;
		ctx = new_session(prompt);
		return process(ctx);
	}

	session_destroy(ctx);
	return NULL;
}

char *
readline_finalize_out_string_exported(const wchar_t *buf)
{
	return finalize_out_string(buf);
}

void
readline_handle_backspace(volatile struct readline_session_context *ctx)
{
	case_key_backspace(ctx);
}

void
readline_handle_key_exported(volatile struct readline_session_context *ctx,
    wint_t wc)
{
	handle_key(ctx, wc);
}

void
readline_mouse_init(void)
{
	if (config_bool("mouse", false)) {
		const char *str = Config("mouse_events");

		if (strings_match(str, "all") || strings_match(str, "ALL"))
			(void) mousemask(ALL_MOUSE_EVENTS, NULL);
		else if (strings_match(str, "wheel") || strings_match(str,
		    "WHEEL"))
			report_wheel_events();
		else
			err_log(EINVAL, "readline_mouse_init: 'mouse_events'");
	} else {
		(void) mousemask(0, NULL);
	}
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
		.rows = 1,
		.cols = cols,
		.start_row = (rows - 1),
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
	mutex_lock(&g_puts_mutex);

	if (panel_state == PANEL1_ACTIVE) {
		if (readline_pan1)
			(void) top_panel(readline_pan1);
	} else if (panel_state == PANEL2_ACTIVE) {
		if (readline_pan2)
			(void) top_panel(readline_pan2);
	} else {
		sw_assert_not_reached();
	}

	if (!atomic_load_bool(&g_resizing_term)) {
		update_panels();
		(void) doupdate();
	}

	mutex_unlock(&g_puts_mutex);
}
