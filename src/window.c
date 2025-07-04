/* Window functions
   Copyright (C) 2012-2025 Markus Uhlin. All rights reserved.

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
#include "atomicops.h"
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
	     window != NULL;\
	     window = window->next)

#define IS_AT_TOP \
	(window->saved_size > 0 && window->saved_size == window->scroll_count)

#define hash(str) hash_djb_g(str, true, ARRAY_SIZE(hash_table))

/* Structure definitions
   ===================== */

struct hInstall_context {
	STRING	 label;
	STRING	 title;
	PANEL	*pan;
	int	 refnum;
};

/* Objects with external linkage
   ============================= */

PIRC_WINDOW	g_active_window = NULL;
PIRC_WINDOW	g_status_window = NULL;
const char	g_status_window_label[10] = "(status)";
const int	g_scroll_amount = 6;
_Atomic(int)	g_ntotal_windows = 0;
volatile bool	g_redrawing_window = false;

#if defined(UNIX)
pthread_mutex_t		g_actwin_mtx;
pthread_mutex_t		g_win_htbl_mtx;
#elif defined(WIN32)
HANDLE			g_actwin_mtx;
HANDLE			g_win_htbl_mtx;
#endif

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

static errno_t
change_window(PIRC_WINDOW window)
{
	WINDOW *pwin;

	if (window == NULL)
		return ENOENT; /* window not found */
	else if (window == g_active_window)
		return 0; /* window already active */
	else if (top_panel(window->pan) == ERR)
		return EPERM;

	if (window->nicklist.pan != NULL)
		(void) top_panel(window->nicklist.pan);

	mutex_lock(&g_actwin_mtx);
	g_active_window = window;
	mutex_unlock(&g_actwin_mtx);
	titlebar(" %s ", (window->title != NULL ? window->title : ""));
	statusbar_update();

	if ((pwin = readline_get_active_pwin()) != NULL) {
		STRING prompt;

		(void) werase(pwin);
		prompt = get_prompt();
		printtext_puts(pwin, prompt, -1, -1, NULL);
		free(prompt);
	}

	readline_top_panel();

	return 0;
}

static int
first_page_up(PIRC_WINDOW window)
{
	PTEXTBUF_ELMT	 element;
	WINDOW		*tmp;
	const int	 goal = (LINES - 3);
	int		 elt_count = 0;
	int		 i = 0;
	int		 rep_count;

	element = textBuf_tail(window->buf);

	if ((tmp = dupwin(panel_window(window->pan))) == NULL)
		err_exit(ENOMEM, "%s: dupwin", __func__);

	(void) atomic_swap_bool(&g_redrawing_window, true);
	(void) curs_set(0);
	while (element != NULL && i < goal) {
		printtext_puts(tmp, element->text, element->indent, -1,
		    &rep_count);
		element = element->prev;
		elt_count++;
		i += rep_count;
	} /* while */
	(void) curs_set(1);
	(void) atomic_swap_bool(&g_redrawing_window, false);

	if (delwin(tmp) == ERR)
		err_exit(EINVAL, "%s: delwin", __func__);
	return elt_count;
}

static int
get_dynamic_scroll_amount(PIRC_WINDOW window, plus_minus_t pm)
{
	PTEXTBUF_ELMT	 element;
	WINDOW		*tmp;
	const int	 overlap = 3;
	int		 amount = 0;
	int		 goal = (LINES - 3);
	int		 i = 0;
	int		 pos;
	int		 rep_count;

	goal -= overlap;
	pos = int_diff(window->saved_size, window->scroll_count);

	if ((element = textBuf_get_element_by_pos(window->buf, pos)) == NULL) {
		debug("error getting element by position");
		return g_scroll_amount;
	} else if ((tmp = dupwin(panel_window(window->pan))) == NULL) {
		err_exit(ENOMEM, "%s: dupwin", __func__);
	}

	(void) atomic_swap_bool(&g_redrawing_window, true);
	(void) curs_set(0);
	while (element != NULL && i < goal) {
		printtext_puts(tmp, element->text, element->indent, -1,
		    &rep_count);
		if (pm == PLUS)
			element = element->prev;
		else if (pm == MINUS)
			element = element->next;
		else
			sw_assert_not_reached();
		amount++;
		i += rep_count;
	} /* while */
	(void) curs_set(1);
	(void) atomic_swap_bool(&g_redrawing_window, false);

	if (delwin(tmp) == ERR)
		err_exit(EINVAL, "%s: delwin", __func__);
	if (i > goal && amount > 1)
		amount -= 1;
	return amount;
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
	BZERO(entry->chanmodes, sizeof entry->chanmodes);

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

static void hUndef(PIRC_WINDOW) NONNULL;

static void
hUndef(PIRC_WINDOW entry)
{
	PIRC_WINDOW *indirect;

	indirect = addrof(hash_table[hash(entry->label)]);
	while (*indirect != entry)
		indirect = addrof((*indirect)->next);
	*indirect = entry->next;

	term_remove_panel(entry->pan);
	event_names_htbl_remove_all(entry);
	textBuf_destroy(entry->buf);

	free(entry->label);
	free(entry->title);

	if (nicklist_destroy(entry) != 0)
		debug("%s: nicklist_destroy: error", __func__);

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

	mutex_lock(&g_win_htbl_mtx);

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

	mutex_unlock(&g_win_htbl_mtx);

	sw_assert(g_status_window->refnum == 1);
	sw_assert(ref_count == g_ntotal_windows);
}

#if 0
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
#endif

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
		(void) wnoutrefresh(pwin);
	if (limit_output) {
		(void) atomic_swap_bool(&g_redrawing_window, true);
		(void) curs_set(0);
		while (element != NULL && i < rows) {
			printtext_puts(pwin, element->text, element->indent,
			    (rows - i), &rep_count);
			element = element->next;
			i += rep_count;
		}
		(void) curs_set(1);
		(void) atomic_swap_bool(&g_redrawing_window, false);
	} else {
		(void) atomic_swap_bool(&g_redrawing_window, true);
		(void) curs_set(0);
		while (element != NULL && i < rows) {
			printtext_puts(pwin, element->text, element->indent, -1,
			    NULL);
			element = element->next;
			++i;
		}
		(void) curs_set(1);
		(void) atomic_swap_bool(&g_redrawing_window, false);
	}

	statusbar_update();
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
			debug("%s: nicklist_draw: error", __func__);
	}

	/*
	 * draw main window
	 */
	if (window->scroll_mode) {
		if (!(window->scroll_count > HEIGHT)) {
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

static void
create_mutexes(void)
{
	mutex_new(&g_actwin_mtx);
	mutex_new(&g_win_htbl_mtx);
}

void
windowSystem_init(void)
{
#if defined(UNIX)
	static pthread_once_t	init_done = PTHREAD_ONCE_INIT;
#elif defined(WIN32)
	static init_once_t	init_done = ONCE_INITIALIZER;
#endif

#if defined(UNIX) && USE_LIBNOTIFY
	if (!notify_init("Swirc IRC client"))
		err_log(0, "%s: notify_init: error", __func__);
#endif

#if defined(UNIX)
	if ((errno = pthread_once(&init_done, create_mutexes)) != 0)
		err_sys("%s: pthread_once", __func__);
#elif defined(WIN32)
	if ((errno = init_once(&init_done, create_mutexes)) != 0)
		err_sys("%s: init_once", __func__);
#endif

	mutex_lock(&g_win_htbl_mtx);
	FOREACH_HASH_TABLE_ENTRY() {
		*entry_p = NULL;
	}
	mutex_unlock(&g_win_htbl_mtx);

	g_status_window = g_active_window = NULL;
	g_ntotal_windows = 0;

	if ((errno = spawn_chat_window(g_status_window_label, "")) != 0)
		err_sys("%s: spawn_chat_window", __func__);
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

	mutex_lock(&g_win_htbl_mtx);
	FOREACH_HASH_TABLE_ENTRY() {
		for (p = *entry_p; p != NULL; p = tmp) {
			tmp = p->next;
			hUndef(p);
		}
	}
	mutex_unlock(&g_win_htbl_mtx);

#if defined(UNIX) && USE_LIBNOTIFY
	notify_uninit();
#endif
}

static void
set_nick_as_title(STRING title, CSTRING nick, size_t size)
{
	if (sw_strcpy(title, nick, size) != 0)
		BZERO(title, size);
}

CSTRING
make_window_title(CSTRING nick)
{
	static char title[400] = { '\0' };

	if (!atomic_load_bool(&g_on_air) ||
	    !config_bool("extended_join", true) ||
	    !g_ircv3_extensions ||
	    g_icb_mode) {
		set_nick_as_title(title, nick, sizeof title);
		return addrof(title[0]);
	}

	FOREACH_HASH_TABLE_ENTRY() {
		FOREACH_WINDOW_IN_ENTRY() {
			PNAMES n;

			if (!is_irc_channel(window->label))
				continue;
			n = event_names_htbl_lookup(nick, window->label);
			if (n == NULL ||
			    n->account == NULL ||
			    n->rl_name == NULL)
				continue;
			(void) snprintf(title, sizeof title, "%s (%s): %s",
			    nick, n->account, n->rl_name);
			return addrof(title[0]);
		}
	}

	set_nick_as_title(title, nick, sizeof title);
	return addrof(title[0]);
}

/**
 * Return the window identified by the given @label -- or NULL on
 * error
 */
PIRC_WINDOW
window_by_label(CSTRING label)
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

PTEXTBUF
get_list_of_matching_channels(CSTRING search_var)
{
	PTEXTBUF	matches = textBuf_new();
	const size_t	varlen = strlen(search_var);

	FOREACH_HASH_TABLE_ENTRY() {
		FOREACH_WINDOW_IN_ENTRY() {
			if (is_irc_channel(window->label) &&
			    !strncasecmp(search_var, window->label, varlen)) {
				textBuf_emplace_back(__func__, matches,
				    window->label, 0);
			}
		}
	}
	if (textBuf_size(matches) == 0) {
		textBuf_destroy(matches);
		return NULL;
	}
	return matches;
}

PTEXTBUF
get_list_of_matching_queries(CSTRING search_var)
{
	PTEXTBUF	matches = textBuf_new();
	const size_t	varlen = strlen(search_var);

	FOREACH_HASH_TABLE_ENTRY() {
		FOREACH_WINDOW_IN_ENTRY() {
			if (!is_irc_channel(window->label) &&
			    window != g_status_window &&
			    !strncasecmp(search_var, window->label, varlen)) {
				textBuf_emplace_back(__func__, matches,
				    window->label, 0);
			}
		}
	}
	if (textBuf_size(matches) == 0) {
		textBuf_destroy(matches);
		return NULL;
	}
	return matches;
}

/**
 * Change window to the one specified by @label
 */
errno_t
change_window_by_label(CSTRING label)
{
	return change_window(window_by_label(label));
}

/**
 * Change window to the one specified by @refnum
 */
errno_t
change_window_by_refnum(int refnum)
{
	return change_window(window_by_refnum(refnum));
}

/**
 * Destroy a chat window
 */
errno_t
destroy_chat_window(CSTRING label)
{
	PIRC_WINDOW window;

	if (label == NULL || strings_match(label, "") ||
	    strings_match_ignore_case(label, g_status_window_label))
		return EINVAL;
	else if ((window = window_by_label(label)) == NULL)
		return ENOENT;

	mutex_lock(&g_win_htbl_mtx);
	hUndef(window);
	mutex_unlock(&g_win_htbl_mtx);
	reassign_window_refnums();
	const errno_t ret = change_window_by_refnum(g_ntotal_windows);
	(void) ret;
	sw_assert_perror(ret);

	return 0;
}

/**
 * Spawn a chat window with given label and title
 */
errno_t
spawn_chat_window(CSTRING label, CSTRING title)
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

	inst_ctx.label  = (STRING) label;
	inst_ctx.title  = (STRING) title;
	inst_ctx.pan    = term_new_panel(LINES - 2, 0, 1, 0);
	inst_ctx.refnum = g_ntotal_windows + 1;

	mutex_lock(&g_win_htbl_mtx);
	entry = hInstall(&inst_ctx);
	mutex_unlock(&g_win_htbl_mtx);
	apply_window_options(panel_window(entry->pan));
	const errno_t ret = change_window_by_label(entry->label);
	(void) ret;
	sw_assert_perror(ret);

	/*
	 * send whois
	 */
	if (atomic_load_bool(&g_on_air) &&
	    !is_irc_channel(entry->label) &&
	    !g_icb_mode)
		cmd_whois(entry->label);

	return 0;
}

/**
 * Set new window title
 */
void
new_window_title(CSTRING label, CSTRING title)
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
				    sizeof window->chanmodes);
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
	mutex_lock(&g_win_htbl_mtx);
	FOREACH_HASH_TABLE_ENTRY() {
		FOREACH_WINDOW_IN_ENTRY() {
			if (is_irc_channel(window->label))
				(void) net_send("JOIN %s", window->label);
		}
	}
	mutex_unlock(&g_win_htbl_mtx);
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
window_scroll_down(PIRC_WINDOW window, const int amount)
{
	const int HEIGHT = LINES - 3;

	if (window == NULL || !window->scroll_mode) {
		term_beep();
		return;
	}

	if (amount <= 0) {
		window->scroll_count -= get_dynamic_scroll_amount(window,
		    MINUS);
	} else {
		window->scroll_count -= amount;
	}

	if (!(window->scroll_count > HEIGHT)) {
		window->saved_size = 0;
		window->scroll_count = 0;
		window->scroll_mode = false;
		window_redraw(window, HEIGHT,
		    textBuf_size(window->buf) - HEIGHT, false);
		return;
	}

	window_redraw(window, HEIGHT,
	    (window->saved_size - window->scroll_count),
	    true);
}

/**
 * Scroll up
 */
void
window_scroll_up(PIRC_WINDOW window, const int amount)
{
	const int MIN_SIZE = LINES - 3;

	if (window == NULL || MIN_SIZE < 0 ||
	    !(textBuf_size(window->buf) > MIN_SIZE) || IS_AT_TOP) {
		term_beep();
		return;
	}

	if (!window->scroll_mode) {
		window->saved_size = textBuf_size(window->buf);
		window->scroll_mode = true;
	}

	if (window->scroll_count > window->saved_size) {
		/*
		 * past top
		 */
		window->scroll_count = window->saved_size;
	} else {
		if (window->scroll_count == 0) {
			/*
			 * first page up
			 */
			window->scroll_count += first_page_up(window);
		}

		if (amount <= 0) {
			window->scroll_count +=
			    get_dynamic_scroll_amount(window, PLUS);
		} else {
			window->scroll_count += amount;
		}

		if (window->scroll_count > window->saved_size)
			window->scroll_count = window->saved_size;
	}

	if (IS_AT_TOP) {
		window_redraw(window, MIN_SIZE, 0, true);
	} else {
		window_redraw(window, MIN_SIZE,
		    (window->saved_size - window->scroll_count),
		    true);
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

static wchar_t *
get_label(CSTRING label)
{
	STRING		str = sw_strdup(label);
	size_t		bytes_convert;
	static wchar_t	wcs[20] = { L'\0' };

	bytes_convert = xmbstowcs(addrof(wcs[0]), squeeze_text_deco(str),
	    ARRAY_SIZE(wcs) - 1);
	wcs[ARRAY_SIZE(wcs) - 1] = L'\0';
	free(str);

	if (bytes_convert == g_conversion_failed)
		wmemset(wcs, 0L, ARRAY_SIZE(wcs));
	return addrof(wcs[0]);
}

static void
print_win(PIRC_WINDOW win)
{
	CSTRING logging = (win->logging ? "Yes" : "No");

	printtext_print("none", "%6d %-20ls %7s",
	    win->refnum, get_label(win->label), logging);
}

void
windows_list_all(void)
{
	PIRC_WINDOW	window;
	int		i = 1;

	printtext_print("none", "%6s %-20s %7s", "Refnum", "Label", "Logging");

	while ((window = window_by_refnum(i++)) != NULL)
		print_win(window);
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
