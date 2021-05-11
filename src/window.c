/* Window functions
   Copyright (C) 2012-2021 Markus Uhlin. All rights reserved.

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
#if defined(UNIX) && USE_LIBNOTIFY
#include <libnotify/notify.h>
#endif
#include "commands/misc.h"
#ifdef WIN32
#include "compat/stdlib.h" /* arc4random() */
#endif

/* names.h wants this header before itself */
#include "irc.h"
#include "events/names.h"

#include "assertAPI.h"
#include "config.h"
#if defined(WIN32) && defined(PDC_EXP_EXTRAS)
#include "curses-funcs.h"	/* is_scrollok() etc */
#endif
#include "dataClassify.h"
#include "errHand.h"
#include "io-loop.h"		/* get_prompt() */
#include "libUtils.h"
#include "main.h"
#include "network.h"
#include "nicklist.h"
#include "printtext.h"		/* includes window.h */
#include "readline.h"		/* readline_top_panel() */
#include "statusbar.h"
#include "strHand.h"
#include "terminal.h"
#include "titlebar.h"

#define FOREACH_HASH_TABLE_ENTRY()\
	for (PIRC_WINDOW *entry_p = &hash_table[0];\
	     entry_p < &hash_table[ARRAY_SIZE(hash_table)];\
	     entry_p++)
#define FOREACH_WINDOW_IN_ENTRY()\
	for (PIRC_WINDOW window = *entry_p;\
	     !isNull(window);\
	     window = window->next)
#define IS_AT_TOP \
	(window->saved_size > 0 && window->saved_size == window->scroll_count)
#define SCROLL_OFFSET 6

/* Structure definitions
   ===================== */

struct hInstall_context {
	char	*label;
	char	*title;
	PANEL	*pan;
	int	 refnum;
};

/* Objects with external linkage
   ============================= */

const char	g_status_window_label[] = "(status)";
PIRC_WINDOW	g_status_window = NULL;
PIRC_WINDOW	g_active_window = NULL;
int		g_ntotal_windows = 0;

/* Objects with internal linkage
   ============================= */

static PIRC_WINDOW hash_table[200] = { NULL };

/* -------------------------------------------------- */

/**
 * Apply window options
 */
static void
apply_window_options(WINDOW *win)
{
	if (!is_scrollok(win))
		(void) scrollok(win, true);
}

static unsigned int
hash(const char *label)
{
	char c;
	char *label_copy = strToLower(sw_strdup(label));
	char *label_p = label_copy;
	unsigned int hashval = 0;
	unsigned int tmp;

	while ((c = *label_p++) != '\0') {
		hashval = (hashval << 4) + c;
		tmp = hashval & 0xf0000000;

		if (tmp) {
			hashval ^= (tmp >> 24);
			hashval ^= tmp;
		}
	}

	free(label_copy);
	return (hashval % ARRAY_SIZE(hash_table));
}

/**
 * spawn_chat_window() helper
 */
static PIRC_WINDOW
hInstall(const struct hInstall_context *ctx)
{
	PIRC_WINDOW	entry;
	unsigned int	hashval;

	entry      = xcalloc(sizeof *entry, 1);
	entry->pan = ctx->pan;

	for (PNAMES *n_ent = &entry->names_hash[0];
	     n_ent < &entry->names_hash[NAMES_HASH_TABLE_SIZE];
	     n_ent++) {
		*n_ent = NULL;
	}

	entry->buf                  = textBuf_new();
	entry->logging              = false;
	entry->received_chancreated = false;
	entry->received_chanmodes   = false;
	entry->received_names       = false;
	entry->scroll_mode          = false;
	BZERO(entry->chanmodes, ARRAY_SIZE(entry->chanmodes));

	entry->label = sw_strdup(ctx->label);
	entry->title = ((ctx->title == NULL || strings_match(ctx->title, ""))
			? NULL
			: sw_strdup(ctx->title));

	entry->num_owners   = 0;
	entry->num_superops = 0;
	entry->num_ops      = 0;
	entry->num_halfops  = 0;
	entry->num_voices   = 0;
	entry->num_normal   = 0;
	entry->num_total    = 0;

	entry->nicklist.pan        = NULL;
	entry->nicklist.scroll_pos = 0;
	entry->nicklist.width      = 0;

	entry->refnum       = ctx->refnum;
	entry->saved_size   = 0;
	entry->scroll_count = 0;

	hashval             = hash(ctx->label);
	entry->next         = hash_table[hashval];
	hash_table[hashval] = entry;

	g_ntotal_windows++;

	return entry;
}

static void hUndef(PIRC_WINDOW) PTR_ARGS_NONNULL;

static void
hUndef(PIRC_WINDOW entry)
{
	PIRC_WINDOW *indirect = addrof(hash_table[hash(entry->label)]);

	while (*indirect != entry)
		indirect = & ((*indirect)->next);
	*indirect = entry->next;

	term_remove_panel(entry->pan);
	event_names_htbl_remove_all(entry);
	textBuf_destroy(entry->buf);

	free(entry->label);
	free(entry->title);

	if (nicklist_destroy(entry) != 0)
		debug("hUndef: nicklist_destroy: error");

	free(entry);
	g_ntotal_windows--;
}

/**
 * Reassign reference numbers (refnums) for all open windows
 */
static void
reassign_window_refnums(void)
{
	int ref_count = 1;

	FOREACH_HASH_TABLE_ENTRY() {
		FOREACH_WINDOW_IN_ENTRY() {
			/*
			 * skip status window and assign new num
			 */
			if (!strings_match_ignore_case(window->label,
			    g_status_window_label))
				window->refnum = ++ref_count;
		}
	}

	sw_assert(g_status_window->refnum == 1);
	sw_assert(ref_count == g_ntotal_windows);
}

static bool
shouldLimitOutputYesNoRandom(void)
{
#if defined(BSD) || defined(WIN32)
	const uint32_t value = arc4random() % 2;
#else
	const int value = rand() % 2;
#endif

	return (value != 0 ? true : false);
}

/**
 * Redraw a window
 */
static void
window_redraw(PIRC_WINDOW window, const int rows, const int pos,
	      bool limit_output)
{
	PTEXTBUF_ELMT element = NULL;
	WINDOW *pwin = panel_window(window->pan);
	int i = 0;
	int rep_count = 0;

	if ((element = textBuf_get_element_by_pos(window->buf,
	    pos < 0 ? 0 : pos)) == NULL)
		return; /* Nothing stored in the buffer */
	if (werase(pwin) != ERR)
		update_panels();
	if (limit_output) {
		while (element != NULL && i < rows) {
			printtext_puts(pwin, element->text, element->indent,
			    (rows - i), &rep_count);
			element = element->next;
			i += rep_count;
		}
	} else {
		while (element != NULL && i < rows) {
			printtext_puts(pwin, element->text, element->indent, -1,
			    NULL);
			element = element->next;
			++ i;
		}
	}

	statusbar_update_display_beta();
	readline_top_panel();
}

/**
 * Recreate one window with given rows and cols
 */
static void
window_recreate(PIRC_WINDOW window, int rows, int cols)
{
	const int HEIGHT = rows - 3;

	if (!is_irc_channel(window->label)) {
		struct term_window_size newsize = {
			.rows = rows - 2,
			.cols = cols,
			.start_row = 1,
			.start_col = 0,
		};

		window->pan = term_resize_panel(window->pan, &newsize);
		apply_window_options(panel_window(window->pan));
	} else {
		/*
		 * irc channel
		 */

		int width;
		const bool width_changed =
		    (width = nicklist_get_width(window)) !=
		    window->nicklist.width;

		if (width_changed)
			window->nicklist.width = width;

		struct term_window_size nicklist_dim;
		nicklist_dim.rows = rows - 2;
		nicklist_dim.cols = width;
		nicklist_dim.start_row = 1;
		nicklist_dim.start_col = cols - width;

		if (window->nicklist.pan) {
			window->nicklist.pan =
			    term_resize_panel(window->nicklist.pan,
					      &nicklist_dim);
		} else {
			window->nicklist.pan =
			    term_new_panel(nicklist_dim.rows, nicklist_dim.cols,
			        nicklist_dim.start_row, nicklist_dim.start_col);
		}

		struct term_window_size channel_dim;
		channel_dim.rows = rows - 2;
		channel_dim.cols = MAX(cols, width) - MIN(cols, width);
		channel_dim.start_row = 1;
		channel_dim.start_col = 0;

		window->pan = term_resize_panel(window->pan, &channel_dim);

		apply_window_options(panel_window(window->nicklist.pan));
		apply_window_options(panel_window(window->pan));

		if (nicklist_draw(window, rows) != 0)
			debug("window_recreate: nicklist_draw: error");
	}

	/*
	 * draw main window
	 */
	if (window->scroll_mode) {
		if (! (window->scroll_count > HEIGHT)) {
			window->saved_size = 0;
			window->scroll_count = 0;
			window->scroll_mode = false;
			window_redraw(window, HEIGHT,
			    textBuf_size(window->buf) - HEIGHT, false);
		} else {
			window_redraw(window, HEIGHT,
			    (window->saved_size - window->scroll_count), true);
		}
		return;
	}

	window_redraw(window, HEIGHT, textBuf_size(window->buf) - HEIGHT,
	    false);
}

void
windowSystem_init(void)
{
#if defined(UNIX) && USE_LIBNOTIFY
	if (!notify_init("Swirc IRC client"))
		err_log(0, "windowSystem_init: notify_init: error");
#endif

	FOREACH_HASH_TABLE_ENTRY() {
		*entry_p = NULL;
	}

	g_status_window = g_active_window = NULL;
	g_ntotal_windows = 0;

	if ((errno = spawn_chat_window(g_status_window_label, "")) != 0)
		err_sys("windowSystem_init: spawn_chat_window");
	else if ((g_status_window = window_by_label(g_status_window_label)) ==
		 NULL) {
		err_quit("Unable to locate the status window\n"
		    "Shouldn't happen.");
	}
}

void
windowSystem_deinit(void)
{
	PIRC_WINDOW p, tmp;

	FOREACH_HASH_TABLE_ENTRY() {
		for (p = *entry_p; p != NULL; p = tmp) {
			tmp = p->next;
			hUndef(p);
		}
	}

#if defined(UNIX) && USE_LIBNOTIFY
	notify_uninit();
#endif
}

/**
 * Return the window identified by the given @label -- or NULL on
 * error
 */
PIRC_WINDOW
window_by_label(const char *label)
{
	PIRC_WINDOW window;

	if (label == NULL || strings_match(label, ""))
		return NULL;

	for (window = hash_table[hash(label)]; window != NULL;
	     window = window->next) {
		if (strings_match_ignore_case(label, window->label))
			return window;
	}

	return NULL;
}

/**
 * Return the window identified by the given reference number
 * (@refnum) -- or NULL on error
 */
PIRC_WINDOW
window_by_refnum(int refnum)
{
	if (refnum < 1 || refnum > g_ntotal_windows)
		return NULL;
	FOREACH_HASH_TABLE_ENTRY() {
		FOREACH_WINDOW_IN_ENTRY() {
			if (refnum == window->refnum)
				return window;
		}
	}
	return NULL;
}

/**
 * Change window to the one specified by @label
 */
int
change_window_by_label(const char *label)
{
	PIRC_WINDOW window = NULL;

	if ((window = window_by_label(label)) == NULL)
		return ENOENT; /* window not found */
	else if (window == g_active_window)
		return 0; /* window already active */
	else if (top_panel(window->pan) == ERR)
		return EPERM; /* top_panel() error */

	if (window->nicklist.pan != NULL)
		(void) top_panel(window->nicklist.pan);

	WINDOW *pwin = readline_get_active_pwin();
	char *prompt = NULL;

	g_active_window = window;
	titlebar(" %s ", (window->title != NULL ? window->title : ""));
	statusbar_update_display_beta();

	if (pwin) {
		(void) werase(pwin);
		prompt = get_prompt();
		printtext_puts(pwin, prompt, -1, -1, NULL);
		free(prompt);
	}

	readline_top_panel();
	(void) ungetch('\a');

	return 0;
}

/**
 * Change window to the one specified by @refnum
 */
int
change_window_by_refnum(int refnum)
{
	PIRC_WINDOW window = NULL;

	if ((window = window_by_refnum(refnum)) == NULL)
		return ENOENT;
	else if (window == g_active_window)
		return 0;
	else if (top_panel(window->pan) == ERR)
		return EPERM;

	if (window->nicklist.pan != NULL)
		(void) top_panel(window->nicklist.pan);

	WINDOW *pwin = readline_get_active_pwin();
	char *prompt = NULL;

	g_active_window = window;
	titlebar(" %s ", (window->title != NULL ? window->title : ""));
	statusbar_update_display_beta();

	if (pwin) {
		(void) werase(pwin);
		prompt = get_prompt();
		printtext_puts(pwin, prompt, -1, -1, NULL);
		free(prompt);
	}

	readline_top_panel();
	(void) ungetch('\a');

	return 0;
}

/**
 * Destroy a chat window
 */
int
destroy_chat_window(const char *label)
{
	PIRC_WINDOW window;

	if (label == NULL || strings_match(label, "") ||
	    strings_match_ignore_case(label, g_status_window_label))
		return EINVAL;
	else if ((window = window_by_label(label)) == NULL)
		return ENOENT;

	hUndef(window);
	reassign_window_refnums();
	const int ret = change_window_by_refnum(g_ntotal_windows);
	(void) ret;
	sw_assert_perror(ret);

	return 0;
}

/**
 * Spawn a chat window with given label and title
 */
int
spawn_chat_window(const char *label, const char *title)
{
	const int ntotalp1 = g_ntotal_windows + 1;
	struct integer_context intctx = {
		.setting_name = "max_chat_windows",
		.lo_limit = 10,
		.hi_limit = 200,
		.fallback_default = 60,
	};

	if (label == NULL || strings_match(label, ""))
		return EINVAL; /* a label is required */
	else if (window_by_label(label) != NULL)
		return 0; /* window already exists  --  reuse it */
	else if (ntotalp1 > config_integer(&intctx))
		return ENOSPC;

	struct hInstall_context inst_ctx;
	PIRC_WINDOW entry;

	inst_ctx.label  = (char *) label;
	inst_ctx.title  = (char *) title;
	inst_ctx.pan    = term_new_panel(LINES - 2, 0, 1, 0);
	inst_ctx.refnum = g_ntotal_windows + 1;

	entry = hInstall(&inst_ctx);
	apply_window_options(panel_window(entry->pan));
	const int ret = change_window_by_label(entry->label);
	(void) ret;
	sw_assert_perror(ret);

	/*
	 * send whois
	 */
	if (g_on_air && !is_irc_channel(entry->label) && !g_icb_mode)
		cmd_whois(entry->label);

	return 0;
}

/**
 * Set new window title
 */
void
new_window_title(const char *label, const char *title)
{
	PIRC_WINDOW window;

	if ((window = window_by_label(label)) == NULL ||
	    title == NULL || strings_match(title, ""))
		return;
	free(window->title);
	window->title = sw_strdup(title);
	if (window == g_active_window)
		titlebar(" %s ", title);
}

/**
 * Close all private conversations
 */
void
window_close_all_priv_conv(void)
{
	char	*priv_conv[200] = { NULL };
	size_t	 pc_assigned = 0;

	FOREACH_HASH_TABLE_ENTRY() {
		FOREACH_WINDOW_IN_ENTRY() {
			if (window == g_status_window ||
			    is_irc_channel(window->label))
				continue;
			if (window->label &&
			    pc_assigned < ARRAY_SIZE(priv_conv)) {
				priv_conv[pc_assigned++] =
				    sw_strdup(window->label);
			}
			if (pc_assigned == ARRAY_SIZE(priv_conv))
				goto out_of_both_loops;
		}
	}

  out_of_both_loops:

	if (pc_assigned == 0) {
		(void) napms(222);
		return;
	}

	for (char **ar_p = &priv_conv[0]; ar_p < &priv_conv[pc_assigned];
	     ar_p++) {
		(void) destroy_chat_window(*ar_p);
		free(*ar_p);
	}
}

/**
 * Destroy names (free them), in all open windows...
 */
void
window_foreach_destroy_names(void)
{
	FOREACH_HASH_TABLE_ENTRY() {
		FOREACH_WINDOW_IN_ENTRY() {
			if (is_irc_channel(window->label)) {
				event_names_htbl_remove_all(window);
				/*
				 * TODO: Investigate if the code below
				 * should be moved too.
				 */
				BZERO(window->chanmodes,
				    ARRAY_SIZE(window->chanmodes));
				window->received_chanmodes = false;
				window->received_chancreated = false;
			}
		}
	}
}

/**
 * Rejoin all IRC channels
 */
void
window_foreach_rejoin_all_channels(void)
{
	FOREACH_HASH_TABLE_ENTRY() {
		FOREACH_WINDOW_IN_ENTRY() {
			if (is_irc_channel(window->label))
				(void) net_send("JOIN %s", window->label);
		}
	}
}

void
window_recreate_exported(PIRC_WINDOW window, int rows, int cols)
{
	window_recreate(window, rows, cols);
}

/**
 * Scroll down
 */
void
window_scroll_down(PIRC_WINDOW window)
{
	const int HEIGHT = LINES - 3;

	if (window == NULL || !window->scroll_mode) {
		term_beep();
		return;
	}

	window->scroll_count -= SCROLL_OFFSET;

	if (! (window->scroll_count > HEIGHT)) {
		window->saved_size = 0;
		window->scroll_count = 0;
		window->scroll_mode = false;
		window_redraw(window, HEIGHT,
		    textBuf_size(window->buf) - HEIGHT, false);
		return;
	}

	window_redraw(window, HEIGHT,
	    (window->saved_size - window->scroll_count),
	    shouldLimitOutputYesNoRandom());
}

/**
 * Scroll up
 */
void
window_scroll_up(PIRC_WINDOW window)
{
	const int MIN_SIZE = LINES - 3;

	if (window == NULL || MIN_SIZE < 0 ||
	    !(textBuf_size(window->buf) > MIN_SIZE) || IS_AT_TOP) {
		term_beep();
		return;
	}

	if (! (window->scroll_mode)) {
		window->saved_size = textBuf_size(window->buf);
		window->scroll_mode = true;
	}

	if (window->scroll_count > window->saved_size) /* past top */
		window->scroll_count = window->saved_size;
	else {
		if (window->scroll_count == 0) /* first page up */
			window->scroll_count += MIN_SIZE;

		window->scroll_count += SCROLL_OFFSET;

		if (window->scroll_count > window->saved_size)
			window->scroll_count = window->saved_size;
	}

	if (IS_AT_TOP)
		window_redraw(window, MIN_SIZE, 0, true);
	else {
		window_redraw(window, MIN_SIZE,
		    (window->saved_size - window->scroll_count),
		    shouldLimitOutputYesNoRandom());
	}
}

/**
 * Switch to the active window plus 1
 */
void
window_select_next(void)
{
	if (g_active_window == NULL)
		return;

	const int refnum_next = g_active_window->refnum + 1;

	if (window_by_refnum(refnum_next) != NULL)
		(void) change_window_by_refnum(refnum_next);
}

/**
 * Switch to the active window minus 1
 */
void
window_select_prev(void)
{
	if (g_active_window == NULL)
		return;

	const int refnum_prev = g_active_window->refnum - 1;

	if (window_by_refnum(refnum_prev) != NULL)
		(void) change_window_by_refnum(refnum_prev);
}

/**
 * Recreate all open windows by calling window_recreate() on each
 */
void
windows_recreate_all(int rows, int cols)
{
	FOREACH_HASH_TABLE_ENTRY() {
		FOREACH_WINDOW_IN_ENTRY() {
			window_recreate(window, rows, cols);
		}
	}
}
