/* Handle event names (353) and event EOF names (366)
   Copyright (C) 2015-2024 Markus Uhlin. All rights reserved.

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

#include <stdexcept>

#include "../dataClassify.h"
#include "../errHand.h"
#include "../irc.h"
#include "../libUtils.h"
#include "../main.h"
#include "../network.h"
#include "../nicklist.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../strdup_printf.h"
#include "../theme.h"

#include "names.h"

#define hash(str) hash_djb_g(str, true, NAMES_HASH_TABLE_SIZE)

/****************************************************************
*                                                               *
*  ------------------ Structure definitions ------------------  *
*                                                               *
****************************************************************/

struct hInstall_context {
	STRING	 channel;
	STRING	 nick;

	bool	 is_owner;
	bool	 is_superop;
	bool	 is_op;
	bool	 is_halfop;
	bool	 is_voice;

	hInstall_context()
	    : channel(NULL)
	    , nick(NULL)
	    , is_owner(false)
	    , is_superop(false)
	    , is_op(false)
	    , is_halfop(false)
	    , is_voice(false)
	{
		/* null */;
	}

	hInstall_context(STRING p_channel, STRING p_nick, const char c)
	    : channel(p_channel)
	    , nick(p_nick)
	    , is_owner(c == '~')
	    , is_superop(c == '&')
	    , is_op(c == '@')
	    , is_halfop(c == '%')
	    , is_voice(c == '+')
	{
		/* null */;
	}

	hInstall_context(STRING p_channel, STRING p_nick, CSTRING privs)
	    : channel(p_channel)
	    , nick(p_nick)
	    , is_owner(false)
	    , is_superop(false)
	    , is_op(false)
	    , is_halfop(false)
	    , is_voice(false)
	{
		for (const char *cp = privs; *cp != '\0'; cp++) {
			if (*cp == '~')
				this->is_owner = true;
			else if (*cp == '&')
				this->is_superop = true;
			else if (*cp == '@')
				this->is_op = true;
			else if (*cp == '%')
				this->is_halfop = true;
			else if (*cp == '+')
				this->is_voice = true;
			else {
				err_log(0, "%s: invalid privilege '%c' "
				    "(privs: %s)", __func__, *cp, privs);
			}
		}
	}
};

/****************************************************************
*                                                               *
*  -------------- Objects with internal linkage --------------  *
*                                                               *
****************************************************************/

static char names_channel[1000] = { '\0' };

/****************************************************************
*                                                               *
*  ---------------------    Functions    ---------------------  *
*                                                               *
****************************************************************/

static void
add_match(PTEXTBUF matches, CSTRING user)
{
	static chararray_t msg = "get_list_of_matching_channel_users: "
	    "textBuf_ins_next";

	if (textBuf_size(matches) != 0) {
		if ((errno = textBuf_ins_next(matches, textBuf_tail(matches),
		    user, -1)) != 0)
			err_sys("%s", msg);
	} else {
		if ((errno = textBuf_ins_next(matches, NULL, user, -1)) != 0)
			err_sys("%s", msg);
	}
}

static inline bool
already_is_in_names_hash(CSTRING nick, PIRC_WINDOW window)
{
	for (PNAMES names = window->names_hash[hash(nick)];
	    names != NULL;
	    names = names->next) {
		if (strings_match_ignore_case(nick, names->nick))
			return true;
	}

	return false;
}

/*
 * Non-strict checking.
 * But we cannot allow:
 *     ~&@%+
 */
static inline bool
name_chars_ok(CSTRING name)
{
	static chararray_t chars =
	    "!#$'()*,-./0123456789:;<=>?"
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
	    "abcdefghijklmnopqrstuvwxyz{|}";

	for (const char *cp = name; *cp != '\0'; cp++) {
		if (strchr(chars, *cp) == NULL)
			return false;
	}

	return true;
}

static inline bool
name_len_ok(CSTRING name)
{
	static const size_t maxlen = 45;

	return (xstrnlen(name, maxlen + 1) <= maxlen);
}

static int
hInstall(const struct hInstall_context *ctx)
{
	PIRC_WINDOW	window;
	PNAMES		names;
	unsigned int	hashval;

	if (ctx == NULL || ctx->channel == NULL) {
		return ERR;
	} else if ((window = window_by_label(ctx->channel)) == NULL) {
		debug("%s: %s: cannot find window labelled \"%s\"", __FILE__,
		    __func__, ctx->channel);
		return ERR;
	} else if (ctx->nick == NULL || strings_match(ctx->nick, "")) {
		debug("%s: %s: no nickname (channel=%s)", __FILE__, __func__,
		    ctx->channel);
		return ERR;
	} else if (!name_len_ok(ctx->nick)) {
		debug("%s: %s: name too long!", __FILE__, __func__);
		return ERR;
	} else if (!name_chars_ok(ctx->nick)) {
		debug("%s: %s: name is invalid", __FILE__, __func__);
		return ERR;
	} else if (already_is_in_names_hash(ctx->nick, window)) {
		debug("%s: %s: busy nickname: \"%s\" (channel=%s)", __FILE__,
		    __func__, ctx->nick, ctx->channel);
		return ERR;
	}

	names = static_cast<PNAMES>(xcalloc(sizeof *names, 1));
	names->nick		= sw_strdup(ctx->nick);
	names->account		= NULL;
	names->rl_name		= NULL;
	names->is_owner		= ctx->is_owner;
	names->is_superop	= ctx->is_superop;
	names->is_op		= ctx->is_op;
	names->is_halfop	= ctx->is_halfop;
	names->is_voice		= ctx->is_voice;

	hashval = hash(ctx->nick);
	names->next = window->names_hash[hashval];
	window->names_hash[hashval] = names;

	if (ctx->is_owner)
		window->num_owners++;
	else if (ctx->is_superop)
		window->num_superops++;
	else if (ctx->is_op)
		window->num_ops++;
	else if (ctx->is_halfop)
		window->num_halfops++;
	else if (ctx->is_voice)
		window->num_voices++;
	else
		window->num_normal++;
	window->num_total++;
	return OK;
}

static void
hUndef(PIRC_WINDOW window, PNAMES entry)
{
	PNAMES *indirect;

	if (window == NULL || entry == NULL || entry->nick == NULL ||
	    strings_match(entry->nick, ""))
		return;

	indirect = addrof(window->names_hash[hash(entry->nick)]);

	while (*indirect != entry)
		indirect = addrof((*indirect)->next);

	*indirect = entry->next;
	free(entry->nick);
	free(entry->account);
	free(entry->rl_name);

	if (entry->is_owner)
		window->num_owners--;
	else if (entry->is_superop)
		window->num_superops--;
	else if (entry->is_op)
		window->num_ops--;
	else if (entry->is_halfop)
		window->num_halfops--;
	else if (entry->is_voice)
		window->num_voices--;
	else
		window->num_normal--;

	window->num_total--;
	free(entry);
}

static STRING
get_bold_int(const int i)
{
	return strdup_printf("%c%d%c", BOLD, i, BOLD);
}

static void
output_statistics(PRINTTEXT_CONTEXT ctx, CSTRING channel,
    const IRC_WINDOW *window)
{
	STRING str;
	STRING num_total;
	STRING num_ops, num_halfops, num_voices, num_normal;

	str = strdup_printf("%s%s%s%c%s", LEFT_BRKT, COLOR1, channel, NORMAL,
	    RIGHT_BRKT);
	num_total	= get_bold_int(window->num_total);
	num_ops		= get_bold_int(window->num_ops);
	num_halfops	= get_bold_int(window->num_halfops);
	num_voices	= get_bold_int(window->num_voices);
	num_normal	= get_bold_int(window->num_normal);

	ctx.spec_type = TYPE_SPEC1;
	printtext(&ctx, "%s: Total of %s nicks "
	    "%s%s ops, %s halfops, %s voices, %s normal%s", str, num_total,
	    LEFT_BRKT, num_ops, num_halfops, num_voices, num_normal,
	    RIGHT_BRKT);
	free(str);
	free(num_total);
	free(num_ops);
	free(num_halfops);
	free(num_voices);
	free(num_normal);

	if (window->num_owners) {
		printtext(&ctx, "-- Additionally: %c%d%c channel owner%s",
		    BOLD, window->num_owners, BOLD,
		    (window->num_owners > 1 ? "s" : ""));
	}
	if (window->num_superops) {
		printtext(&ctx, "-- Additionally: %c%d%c superop%s",
		    BOLD, window->num_superops, BOLD,
		    (window->num_superops > 1 ? "s" : ""));
	}
}

static void
reset_counters(PIRC_WINDOW window)
{
	window->num_owners	= 0;
	window->num_superops	= 0;
	window->num_ops		= 0;
	window->num_halfops	= 0;
	window->num_voices	= 0;
	window->num_normal	= 0;
	window->num_total	= 0;
}

/*
 * usage: /stats [channel]
 */
void
cmd_stats(CSTRING data)
{
	PIRC_WINDOW		win;
	PRINTTEXT_CONTEXT	ptext_ctx;
	static chararray_t	cmd = "/stats";

	if (strings_match(data, "")) {
		if (!is_irc_channel(ACTWINLABEL))
			printtext_print("err", "%s: not an irc channel", cmd);
		else if ((win = window_by_label(ACTWINLABEL)) == NULL)
			printtext_print("err", "%s: no such channel", cmd);
		else {
			printtext_context_init(&ptext_ctx, win, TYPE_SPEC3,
			    true);
			output_statistics(ptext_ctx, ACTWINLABEL, win);
		}
	} else {
		if (!is_irc_channel(data))
			printtext_print("err", "%s: bogus irc channel", cmd);
		else if ((win = window_by_label(data)) == NULL)
			printtext_print("err", "%s: no such channel", cmd);
		else {
			printtext_context_init(&ptext_ctx, g_active_window,
			    TYPE_SPEC3, true);
			output_statistics(ptext_ctx, data, win);
		}
	}
}

PTEXTBUF
get_list_of_matching_channel_users(CSTRING chan, CSTRING search_var)
{
	PIRC_WINDOW	window;
	PTEXTBUF	matches;
	size_t		varlen;

	if ((window = window_by_label(chan)) == NULL)
		return NULL;

	matches = textBuf_new();
	varlen = strlen(search_var);

	for (size_t n = 0; n < ARRAY_SIZE(window->names_hash); n++) {
		for (PNAMES names = window->names_hash[n];
		    names != NULL;
		    names = names->next) {
			if (!strncmp(search_var, names->nick, varlen))
				add_match(matches, names->nick);
		}
	}

	if (textBuf_size(matches) == 0) {
		textBuf_destroy(matches);
		return NULL;
	}

	return matches;
}

/**
 * Initialize the module
 */
void
event_names_init(void)
{
	BZERO(names_channel, sizeof names_channel);
}

/**
 * Deinitialize the module
 */
void
event_names_deinit(void)
{
	window_foreach_destroy_names();
}

PNAMES
event_names_htbl_lookup(CSTRING nick, CSTRING channel)
{
	PIRC_WINDOW	window;
	PNAMES		names;

	if (nick == NULL || strings_match(nick, "") ||
	    (window = window_by_label(channel)) == NULL)
		return NULL;

	for (names = window->names_hash[hash(nick)];
	    names != NULL;
	    names = names->next) {
		if (strings_match_ignore_case(nick, names->nick))
			return names;
	}

	return NULL;
}

int
event_names_htbl_insert(CSTRING nick, CSTRING channel)
{
	if (nick == NULL || strings_match(nick, ""))
		return ERR;

	struct hInstall_context ctx; /* calls constructor */

	ctx.channel	= const_cast<STRING>(channel);
	ctx.nick	= const_cast<STRING>(nick);

	if (hInstall(&ctx) == ERR)
		return ERR;
	if (nicklist_update(window_by_label(channel)) != 0)
		debug("event_names_htbl_insert: nicklist_update: error");
	return OK;
}

int
event_names_htbl_remove(CSTRING nick, CSTRING channel)
{
	PIRC_WINDOW	window;
	PNAMES		names;

	if (nick == NULL || strings_match(nick, "") ||
	    (window = window_by_label(channel)) == NULL)
		return ERR;

	for (names = window->names_hash[hash(nick)];
	    names != NULL;
	    names = names->next) {
		if (strings_match_ignore_case(nick, names->nick)) {
			hUndef(window, names);

			if (nicklist_update(window) != 0) {
				debug("event_names_htbl_remove: "
				    "nicklist_update: error");
			}

			return OK;
		}
	}

	return ERR;
}

/* event_eof_names: 366

   Example:
     :irc.server.com 366 <recipient> <channel> :End of /NAMES list. */
void
event_eof_names(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT ptext_ctx;

	try {
		PIRC_WINDOW win = NULL;
		STRING channel, eof_msg;
		STRING state = const_cast<STRING>("");

		if (strFeed(compo->params, 2) != 2)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);

		channel = strtok_r(NULL, "\n", &state);
		eof_msg = strtok_r(NULL, "\n", &state);

		if (channel == NULL) {
			throw std::runtime_error("null channel");
		} else if (!is_irc_channel(channel) || strpbrk(channel + 1,
		    g_forbidden_chan_name_chars) != NULL) {
			throw std::runtime_error("invalid channel");
		} else if (eof_msg == NULL) {
			throw std::runtime_error("null message");
		} else if (!strings_match_ignore_case(channel, names_channel)) {
			throw std::runtime_error("unable to parse names of two "
			    "(or more) channels simultaneously");
		} else {
			BZERO(names_channel, sizeof names_channel);
		}

		if ((win = window_by_label(channel)) == NULL) {
			throw std::runtime_error("window lookup error");
		} else if (win->received_names) {
			err_log(0, "warning: server sent event 366 "
			    "(RPL_ENDOFNAMES): already received names for "
			    "channel %s", channel);
			return;
		} else {
			win->received_names = true;
		}

		if (nicklist_new(win) != 0)
			debug("event_eof_names: cannot create nicklist");
		if (!g_icb_mode)
			(void) net_send("MODE %s", channel);

		printtext_context_init(&ptext_ctx, win, TYPE_SPEC3, true);
		output_statistics(ptext_ctx, channel, win);
		return;
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ptext_ctx, g_active_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ptext_ctx, "event_eof_names: fatal: %s", e.what());

#if IRCFUZZ_MODE
#pragma message("warning: fuzz mode is ON (not to be used in production)")
#pragma message("...omitted code...")
#else
		if (strstr(e.what(), "parse names of two (or more)") ||
		    strings_match(e.what(), "window lookup error")) {
			printtext(&ptext_ctx, "must shutdown irc connection "
			    "immediately...");
			net_kill_connection();
		}
#endif
	}
}

/* event_names: 353

   Example:
     :irc.server.com 353 <recipient> <chan type> <channel> :<nick 1> <nick 2>
                                                            [...]

   <chan type> can be either:
     @ for secret channels
     * for private channels
     = for public channels */
void
event_names(struct irc_message_compo *compo)
{
	STRING names_copy = NULL;

	try {
		PIRC_WINDOW win = NULL;
		STRING chan_type, channel, names;
		STRING state1 = const_cast<STRING>("");
		STRING state2 = const_cast<STRING>("");

		if (strFeed(compo->params, 3) != 3)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state1); /* recipient */

		chan_type	= strtok_r(NULL, "\n", &state1);
		channel		= strtok_r(NULL, "\n", &state1);
		names		= strtok_r(NULL, "\n", &state1);

		if (chan_type == NULL) {
			throw std::runtime_error("no channel type");
		} else if (channel == NULL) {
			throw std::runtime_error("no channel");
		} else if (!is_irc_channel(channel) || strpbrk(channel + 1,
		    g_forbidden_chan_name_chars) != NULL) {
			throw std::runtime_error("invalid channel");
		} else if (names == NULL) {
			throw std::runtime_error("no names");
		} else if (strings_match(names_channel, "") &&
		    sw_strcpy(names_channel, channel, ARRAY_SIZE(names_channel))
		    != 0) {
			throw std::runtime_error("unable to "
			    "store names channel");
		} else if (!strings_match_ignore_case(names_channel, channel)) {
			throw std::runtime_error("unable to parse names of two "
			    "(or more) channels simultaneously");
		} else if ((win = window_by_label(channel)) == NULL) {
			throw std::runtime_error("window lookup error");
		} else if (win->received_names) {
			err_log(0, "warning: server sent event 353 "
			    "(RPL_NAMREPLY): already received names for "
			    "channel %s", channel);
			return;
		} else {
			names_copy = sw_strdup(*names == ':' ? &names[1] :
			    &names[0]);
		}

		for (char *cp = &names_copy[0];; cp = NULL) {
			STRING	token, nick;
			char	privs[6] = { '\0' };
			size_t	ret;

			if ((token = strtok_r(cp, " ", &state2)) == NULL)
				break;

			ret = strspn(token, "~&@%+");

			if (ret >= sizeof privs) {
				debug("%s: privileges too long", __func__);
				continue;
			} else if (ret > 0) {
				memcpy(privs, token, ret);
				privs[ret] = '\0';
				nick = &token[ret];
			} else
				nick = &token[0];

			struct hInstall_context ctx(channel, nick, (ret > 0
			    ? privs : ""));

			if (hInstall(&ctx) != OK)
				continue;
		}
	} catch (const std::runtime_error &e) {
		PRINTTEXT_CONTEXT ptext_ctx;

		printtext_context_init(&ptext_ctx, g_active_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ptext_ctx, "event_names: fatal: %s", e.what());

#if IRCFUZZ_MODE
#pragma message("warning: fuzz mode is ON (not to be used in production)")
#pragma message("...omitted code...")
#else
		if (strstr(e.what(), "store names channel") ||
		    strstr(e.what(), "parse names of two (or more)")) {
			printtext(&ptext_ctx, "must shutdown irc connection "
			    "immediately...");
			net_kill_connection();
		}
#endif
	}

	free(names_copy);
}

void
event_names_htbl_remove_all(PIRC_WINDOW window)
{
	PNAMES *entry_p;
	PNAMES p, tmp;

	if (window == NULL || !is_irc_channel(window->label))
		return;
	for (entry_p = &window->names_hash[0];
	    entry_p < &window->names_hash[NAMES_HASH_TABLE_SIZE];
	    entry_p++) {
		for (p = *entry_p; p != NULL; p = tmp) {
			tmp = p->next;
			hUndef(window, p);
		}
	}

	window->received_names = false;
	reset_counters(window);
}
/* EOF */
