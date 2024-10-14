/* Channel related events
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
#include <time.h>

#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../irc.h"
#include "../libUtils.h"
#include "../main.h"
#include "../netsplit.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "channel.h"
#include "i18n.h"
#include "names.h"

#define SHUTDOWN_IRC_CONNECTION_BEHAVIOR 0
#define TM_STRUCT_MSG "unable to retrieve tm structure"

#define log_unwanted_pm_state() \
	err_log(EINVAL, "%s: error: neither +/-", __func__)

struct quit_context {
	CSTRING		message;
	CSTRING		nick, user, host;
};

static void	auto_op(CSTRING, CSTRING) NONNULL;
static void	handle_quit(PIRC_WINDOW, PPRINTTEXT_CONTEXT,
		    struct quit_context *) NONNULL;
static bool	is_netsplit(CSTRING, std::string &, std::string &) NONNULL;
static void	maintain_channel_stats(const char *, const char *) NONNULL;

/* event_chan_hp: 328

   Example:
     :irc.server.com 328 <recipient> <channel> :www.channel.homepage.com */
void
event_chan_hp(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	try {
		char	*channel, *homepage;
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 2) != 2)
			throw std::runtime_error("strFeed");

		/* unused */
		(void) strtok_r(compo->params, "\n", &state);

		if ((channel = strtok_r(NULL, "\n", &state)) == NULL)
			throw std::runtime_error("unable to get channel");
		else if ((homepage = strtok_r(NULL, "\n", &state)) == NULL)
			throw std::runtime_error("unable to get homepage");
		else if (*homepage == ':')
			homepage++;

		printtext_context_init(&ctx, NULL, TYPE_SPEC1, true);

		if ((ctx.window = window_by_label(channel)) == NULL)
			throw std::runtime_error("window lookup error");

		printtext(&ctx, _("Homepage for %s%s%s%c%s: %s"),
		    LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT,
		    homepage);
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN,
		    true);
		printtext(&ctx, "%s(%s): error: %s", __func__, compo->command,
		    e.what());
	}
}

static void
auto_op(CSTRING channel, CSTRING nick)
{
	if (g_am_irc_op &&
	    config_bool("auto_op_yourself", true) &&
	    (net_send("MODE %s +o %s", channel, nick) < 0 ||
	    net_send("SAMODE %s +o %s", channel, nick) < 0))
		throw std::runtime_error("cannot send");
}

static void
join_perform_some_tasks(CSTRING channel, CSTRING nick, CSTRING account,
    CSTRING rl_name)
{
	PNAMES n = NULL;

	if (account != NULL && *account == ':')
		account++;
	if (rl_name != NULL && *rl_name == ':')
		rl_name++;

	if (!is_irc_channel(channel) ||
	    strpbrk(channel + 1, g_forbidden_chan_name_chars) != NULL) {
		throw std::runtime_error("bogus irc channel");
	} else if (strings_match_ignore_case(nick, g_my_nickname)) {
		if (spawn_chat_window(channel, "No title.") != 0)
			throw std::runtime_error("cannot spawn chat window");
		auto_op(channel, nick);
	} else if (event_names_htbl_insert(nick, channel) != OK) {
		throw std::runtime_error("unable to add user to channel list");
	} else if (account != NULL && rl_name != NULL &&
	    (n = event_names_htbl_lookup(nick, channel)) != NULL) {
		if (n->account)
			free(n->account);
		if (n->rl_name)
			free(n->rl_name);
		n->account = sw_strdup(account);
		n->rl_name = sw_strdup(rl_name);
	}
}

static void
chk_split(CSTRING nick, CSTRING channel, netsplit *&split)
{
	if (!netsplit_db_empty() &&
	    (split = netsplit_find(nick, channel)) != NULL) {
		if (split->remove_nick(nick)) {
			if (!split->join_begun())
				split->set_join_time(time(NULL));
		} else {
			throw std::runtime_error("failed to remove nick from "
			    "split");
		}
	}
}

/* event_join

   Examples:
     :<nick>!<user>@<host> JOIN <channel>
     :<nick>!<user>@<host> JOIN <channel> <account> :<real name>
     :<nick>!<user>@<host> JOIN <channel> * :<real name> */
void
event_join(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	try {
		CSTRING		 channel, account, rl_name;
		CSTRING		 nick, user, host;
		STRING		 prefix = NULL;
		STRING		 state[2];
		netsplit	*split = NULL;

		if (compo == NULL)
			throw std::runtime_error("no components");
		else if (compo->prefix == NULL)
			throw std::runtime_error("no prefix");

		channel = account = rl_name = NULL;
		prefix = &compo->prefix[1];
		state[0] = state[1] = const_cast<STRING>("");

		if ((nick = strtok_r(prefix, "!@", &state[0])) == NULL)
			throw std::runtime_error("no nickname");
		if ((user = strtok_r(NULL, "!@", &state[0])) == NULL)
			user = "<no user>";
		if ((host = strtok_r(NULL, "!@", &state[0])) == NULL)
			host = "<no host>";

		const int num = strFeed(compo->params, 2);
		UNUSED_VAR(num);

		channel = strtok_r(compo->params, "\n", &state[1]);
		account = strtok_r(NULL, "\n", &state[1]);
		rl_name = strtok_r(NULL, "\n", &state[1]);

		if (channel == NULL)
			throw std::runtime_error("no channel");
		else if (*channel == ':')
			channel++;

		join_perform_some_tasks(channel, nick, account, rl_name);
		chk_split(nick, channel, split);

		if (split == NULL && config_bool("joins_parts_quits", true)) {
			printtext_context_init(&ctx, NULL, TYPE_SPEC1_SPEC2,
			    true);

			if ((ctx.window = window_by_label(channel)) == NULL)
				throw std::runtime_error("window lookup error");

			printtext(&ctx, _("%s%s%c %s%s@%s%s has joined %s%s%c"),
			    COLOR1, nick, NORMAL,
			    LEFT_BRKT, user, host, RIGHT_BRKT,
			    COLOR2, channel, NORMAL);
		}
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_active_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ctx, "%s(%s): fatal: %s", __func__, compo->command,
		    e.what());
#if SHUTDOWN_IRC_CONNECTION_BEHAVIOR
		printtext(&ctx, "Shutting down IRC connection...");
		net_request_disconnect();
#endif
	}
}

/* event_kick

   Examples:
     :<nick>!<user>@<host> KICK <channel> <victim> :<reason> */
void
event_kick(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	try {
		CSTRING channel, victim, reason;
		CSTRING nick, user, host;
		STRING prefix = NULL;
		STRING state1 = const_cast<STRING>("");
		STRING state2 = const_cast<STRING>("");

		if (compo == NULL)
			throw std::runtime_error("no components");
		else if (compo->prefix == NULL)
			throw std::runtime_error("no prefix");

		prefix = &compo->prefix[1];

		if ((nick = strtok_r(prefix, "!@", &state1)) == NULL)
			throw std::runtime_error("no nickname");
		if ((user = strtok_r(NULL, "!@", &state1)) == NULL)
			user = "<no user>";
		if ((host = strtok_r(NULL, "!@", &state1)) == NULL)
			host = "<no host>";

		UNUSED_VAR(user);
		UNUSED_VAR(host);

		(void) strFeed(compo->params, 2);

		if ((channel = strtok_r(compo->params, "\n", &state2)) == NULL)
			throw std::runtime_error("no channel");
		else if ((victim = strtok_r(NULL, "\n", &state2)) == NULL)
			throw std::runtime_error("no victim");

		const bool has_reason = ((reason = strtok_r(NULL, "\n",
		    &state2)) != NULL);
		if (has_reason && *reason == ':')
			reason++;

		if (strings_match_ignore_case(victim, g_my_nickname)) {
			if (config_bool("kick_close_window", true) &&
			    destroy_chat_window(channel) != 0) {
				throw std::runtime_error("failed to destroy "
				    "chat window");
			} else {
				PIRC_WINDOW	win;

				win = window_by_label(channel);
				event_names_htbl_remove_all(win);
			}
		} else if (event_names_htbl_remove(victim, channel) != OK) {
			throw std::runtime_error("failed to remove victim from "
			    "channel");
		}

		printtext_context_init(&ctx, NULL, TYPE_SPEC1_SPEC2, true);

		if ((ctx.window = window_by_label(channel)) == NULL)
			ctx.window = g_active_window;

		printtext(&ctx, _("%s was kicked from %s%s%c by %s%s%c %s%s%s"),
		    victim, COLOR2, channel, NORMAL, COLOR2, nick, NORMAL,
		    LEFT_BRKT, (has_reason ? reason : ""), RIGHT_BRKT);
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_active_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ctx, "%s(%s): fatal: %s", __func__, compo->command,
		    e.what());
#if SHUTDOWN_IRC_CONNECTION_BEHAVIOR
		printtext(&ctx, "Shutting down IRC connection...");
		net_request_disconnect();
#endif
	}
}

static void
chg_status_for_owner(plus_minus_state_t pm_state, const char *nick,
    const char *channel)
{
	/* On ircd-seven +q means quiet and works like +b (ban user),
	 * but allows matching users to join the channel. However: on
	 * InspIRCd setting +q makes user channel owner. This should
	 * fix the problem. */
	if (!is_valid_nickname(nick))
		return;

	switch (pm_state) {
	case STATE_PLUS:
		if (names_htbl_modify::owner(nick, channel, true) != OK)
			err_log(0, "%s (+)", __func__);
		break;
	case STATE_MINUS:
		if (names_htbl_modify::owner(nick, channel, false) != OK)
			err_log(0, "%s (-)", __func__);
		break;
	case STATE_NEITHER_PM:
	default:
		log_unwanted_pm_state();
		break;
	}
}

static void
chg_status_for_superop(plus_minus_state_t pm_state, const char *nick,
    const char *channel)
{
	switch (pm_state) {
	case STATE_PLUS:
		if (names_htbl_modify::superop(nick, channel, true) != OK)
			err_log(0, "%s (+)", __func__);
		break;
	case STATE_MINUS:
		if (names_htbl_modify::superop(nick, channel, false) != OK)
			err_log(0, "%s (-)", __func__);
		break;
	case STATE_NEITHER_PM:
	default:
		log_unwanted_pm_state();
		break;
	}
}

static void
chg_status_for_op(plus_minus_state_t pm_state, const char *nick,
    const char *channel)
{
	switch (pm_state) {
	case STATE_PLUS:
		if (names_htbl_modify::op(nick, channel, true) != OK)
			err_log(0, "%s (+)", __func__);
		break;
	case STATE_MINUS:
		if (names_htbl_modify::op(nick, channel, false) != OK)
			err_log(0, "%s (-)", __func__);
		break;
	case STATE_NEITHER_PM:
	default:
		log_unwanted_pm_state();
		break;
	}
}

static void
chg_status_for_halfop(plus_minus_state_t pm_state, const char *nick,
    const char *channel)
{
	switch (pm_state) {
	case STATE_PLUS:
		if (names_htbl_modify::halfop(nick, channel, true) != OK)
			err_log(0, "%s (+)", __func__);
		break;
	case STATE_MINUS:
		if (names_htbl_modify::halfop(nick, channel, false) != OK)
			err_log(0, "%s (-)", __func__);
		break;
	case STATE_NEITHER_PM:
	default:
		log_unwanted_pm_state();
		break;
	}
}

static void
chg_status_for_voice(plus_minus_state_t pm_state, const char *nick,
    const char *channel)
{
	switch (pm_state) {
	case STATE_PLUS:
		if (names_htbl_modify::voice(nick, channel, true) != OK)
			err_log(0, "%s (+)", __func__);
		break;
	case STATE_MINUS:
		if (names_htbl_modify::voice(nick, channel, false) != OK)
			err_log(0, "%s (-)", __func__);
		break;
	case STATE_NEITHER_PM:
	default:
		log_unwanted_pm_state();
		break;
	}
}

/*
 * Example input: +vvv nick1 nick2 nick3
 */
static void
maintain_channel_stats(const char *channel, const char *input)
{
	char			*input_copy;
	char			*nicks[15] = { NULL };
	char			*state = const_cast<char *>("");
	const char		*modes = "";
	plus_minus_state_t	 pm_state = STATE_NEITHER_PM;
	size_t			 ar_i = 0;
	size_t			 nicks_assigned = 0;
	static const size_t	 ar_sz = ARRAY_SIZE(nicks);

	input_copy = sw_strdup(input);

	if ((modes = strtok_r(input_copy, " ", &state)) == NULL) {
		err_log(EINVAL, "%s", __func__);
		free(input_copy);
		return;
	}

	for (nicks_assigned = 0;; nicks_assigned++) {
		const char	*token;

		if ((token = strtok_r(NULL, " ", &state)) != NULL &&
		    nicks_assigned < ar_sz)
			nicks[nicks_assigned] = sw_strdup(token);
		else
			break;
	}

	for (const char *cp = modes; *cp && ar_i < nicks_assigned; cp++) {
		switch (*cp) {
		case '+':
			pm_state = STATE_PLUS;
			break;
		case '-':
			pm_state = STATE_MINUS;
			break;
		case 'I': /* set/remove an invitation mask */
		case 'b': /* set/remove ban mask           */
		case 'e': /* set/remove an exception mask  */
		case 'k': /* set/remove the channel key    */
			ar_i++;
			break;
		case 'j': /* join throttle */
		case 'l': /* set/remove the user limit to channel */
			if (pm_state == STATE_PLUS)
				ar_i++;
			break;
		case 'q':
			chg_status_for_owner(pm_state, nicks[ar_i++], channel);
			break;
		case 'a':
			chg_status_for_superop(pm_state, nicks[ar_i++], channel);
			break;
		case 'o':
			chg_status_for_op(pm_state, nicks[ar_i++], channel);
			break;
		case 'h':
			chg_status_for_halfop(pm_state, nicks[ar_i++], channel);
			break;
		case 'v':
			chg_status_for_voice(pm_state, nicks[ar_i++], channel);
			break;
		}
	}

	if (strspn(modes, "+-Ibeqaohv") != strlen(modes) && net_send("MODE %s",
	    channel) < 0)
		err_log(ENOTCONN, "%s: net_send", __func__);

	free(input_copy);

	/*
	 * Destroy the array...
	 */
	for (char **ar_p = &nicks[0]; ar_p < &nicks[ar_sz]; ar_p++) {
		free(*ar_p);
		*ar_p = NULL;
	}
}

/* event_mode

   Examples:
     :<nick>!<user>@<host> MODE <channel> +v <nick>
     :<my nick> MODE <my nick> :+i */
void
event_mode(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT ctx;
	char *next_token_copy = NULL;

	try {
		char		*channel, *next_token;
		char		*cp = NULL;
		char		*prefix = NULL;
		char		*state1 = const_cast<char *>("");
		char		*state2 = const_cast<char *>("");
		const char	*nick, *user, *host;

		if (compo->prefix == NULL)
			throw std::runtime_error("no prefix");

		prefix = &compo->prefix[1];

		if ((nick = strtok_r(prefix, "!@", &state1)) == NULL) {
			throw std::runtime_error("unable to get nickname in "
			    "prefix");
		} else if (strFeed(compo->params, 1) != 1) {
			throw std::runtime_error("strFeed");
		}

		if ((user = strtok_r(NULL, "!@", &state1)) == NULL)
			user = "";
		if ((host = strtok_r(NULL, "!@", &state1)) == NULL)
			host = "";
		UNUSED_VAR(user);
		UNUSED_VAR(host);

		if ((channel = strtok_r(compo->params, "\n", &state2)) == NULL ||
		    (next_token = strtok_r(NULL, "\n", &state2)) == NULL) {
			throw std::runtime_error("insufficient data");
		} else if (*next_token == ':') {
			next_token ++;
		} else if ((cp = strstr(next_token, " :")) != NULL) {
			*++cp = ' ';
			(void) memmove(cp - 1, cp, strlen(cp) + 1);
		}

		next_token_copy = sw_strdup(next_token);
		(void) trim(next_token_copy);

		printtext_context_init(&ctx, g_status_window, TYPE_SPEC1, true);

		if (strings_match_ignore_case(channel, g_my_nickname)) {
			/*
			 * User mode
			 */

			printtext(&ctx, _("Mode change %s%s%s for user %c%s%c"),
			    LEFT_BRKT, next_token_copy, RIGHT_BRKT,
			    BOLD, g_my_nickname, BOLD);

			if (net_send("MODE %s", g_my_nickname) < 0)
				throw std::runtime_error("cannot send");
		} else if (is_irc_channel(channel) &&
		    (ctx.window = window_by_label(channel)) != NULL) {
			printtext(&ctx, _("mode/%s%s%s%c%s %s%s%s by %s%s%c"),
			    LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT,
			    LEFT_BRKT, next_token_copy, RIGHT_BRKT,
			    COLOR2, nick, NORMAL);

			maintain_channel_stats(channel, next_token_copy);
		} else {
			throw std::runtime_error("unhandled else branch");
		}
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN,
		    true);
		printtext(&ctx, "%s(%s): error: %s", __func__, compo->command,
		    e.what());
	}

	free(next_token_copy);
}

static int
RemoveAndInsertNick(const char *old_nick, const char *new_nick,
    const char *label)
{
	PNAMES	p;
	bool	is_owner, is_superop, is_op, is_halfop, is_voice;

	if ((p = event_names_htbl_lookup(old_nick, label)) == NULL)
		return ERR; /* non-fatal: old_nick not found on channel */

	is_owner	= p->is_owner;
	is_superop	= p->is_superop;
	is_op		= p->is_op;
	is_halfop	= p->is_halfop;
	is_voice	= p->is_voice;

	if (event_names_htbl_remove(old_nick, label) != OK) {
		err_log(0, "%s: event_names_htbl_remove", __func__);
		return ERR;
	} else if (event_names_htbl_insert(new_nick, label) != OK) {
		err_log(0, "%s: event_names_htbl_insert", __func__);
		return ERR;
	}

	/*
	 * Reverse order...
	 */
	if (is_voice)
		chg_status_for_voice(STATE_PLUS, new_nick, label);
	if (is_halfop)
		chg_status_for_halfop(STATE_PLUS, new_nick, label);
	if (is_op)
		chg_status_for_op(STATE_PLUS, new_nick, label);
	if (is_superop)
		chg_status_for_superop(STATE_PLUS, new_nick, label);
	if (is_owner)
		chg_status_for_owner(STATE_PLUS, new_nick, label);
	return OK;
}

/* event_nick

   Example:
     :<nick>!<user>@<host> NICK :<new nick> */
void
event_nick(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	try {
		CSTRING new_nick;
		CSTRING nick, user, host;
		STRING prefix = NULL;
		STRING state = const_cast<STRING>("");

		if (compo->prefix == NULL)
			throw std::runtime_error("no prefix");

		new_nick = (*(compo->params) == ':' ? &compo->params[1] :
		    &compo->params[0]);
		prefix = &compo->prefix[1];

		if ((nick = strtok_r(prefix, "!@", &state)) == NULL)
			throw std::runtime_error("no nickname");

		if ((user = strtok_r(NULL, "!@", &state)) == NULL)
			user = "";
		if ((host = strtok_r(NULL, "!@", &state)) == NULL)
			host = "";
		UNUSED_VAR(user);
		UNUSED_VAR(host);

		printtext_context_init(&ctx, NULL, TYPE_SPEC1_SPEC2, true);

		for (int i = 1; i <= g_ntotal_windows; i++) {
			PIRC_WINDOW	window;

			if ((window = window_by_refnum(i)) != NULL &&
			    is_irc_channel(window->label) &&
			    RemoveAndInsertNick(nick, new_nick, window->label)
			    == OK) {
				ctx.window = window;

				printtext(&ctx, _("%s%s%c is now known as %s "
				    "%s%s%c"), COLOR2, nick, NORMAL, THE_SPEC2,
				    COLOR1, new_nick, NORMAL);
			}
		}

		if (strings_match_ignore_case(nick, g_my_nickname))
			irc_set_my_nickname(new_nick);
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN,
		    true);
		printtext(&ctx, "%s(%s): error: %s", __func__, compo->command,
		    e.what());
	}
}

/* event_part

   Examples:
     :<nick>!<user>@<host> PART <channel>
     :<nick>!<user>@<host> PART <channel> :<message> */
void
event_part(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	try {
		char		*prefix = NULL;
		char		*state1 = const_cast<char *>("");
		char		*state2 = const_cast<char *>("");
		const char	*channel, *message;
		const char	*nick, *user, *host;

		if (compo->prefix == NULL)
			throw std::runtime_error("no prefix!");

		prefix = &compo->prefix[1];

		if ((nick = strtok_r(prefix, "!@", &state1)) == NULL)
			throw std::runtime_error("unable to get nickname");
		if ((user = strtok_r(NULL, "!@", &state1)) == NULL)
			user = "<no user>";
		if ((host = strtok_r(NULL, "!@", &state1)) == NULL)
			host = "<no host>";

		const bool has_message = strFeed(compo->params, 1) == 1;

		if ((channel = strtok_r(compo->params, "\n", &state2)) == NULL)
			throw std::runtime_error("unable to get channel");
		else if (*channel == ':')
			channel++;

		message = strtok_r(NULL, "\n", &state2);

		if (strings_match_ignore_case(nick, g_my_nickname)) {
			if (destroy_chat_window(channel) != 0) {
				throw std::runtime_error("failed to destroy "
				    "chat window");
			}

			return;
		} else {
			if (event_names_htbl_remove(nick, channel) != OK) {
				throw std::runtime_error("failed to remove "
				    "user from channel list");
			}
		}

		if (config_bool("joins_parts_quits", true)) {
			printtext_context_init(&ctx, NULL, TYPE_SPEC1_SPEC2,
			    true);

			if ((ctx.window = window_by_label(channel)) == NULL)
				throw std::runtime_error("window lookup error");
			if (!has_message || message == NULL)
				message = "";
			if (has_message && message != NULL && *message == ':')
				message++;

			printtext(&ctx, _("%s%s%c %s%s@%s%s has left %s%s%c "
			    "%s%s%s"),
			    COLOR2, nick, NORMAL,
			    LEFT_BRKT, user, host, RIGHT_BRKT,
			    COLOR2, channel, NORMAL,
			    LEFT_BRKT, message, RIGHT_BRKT);
		}
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_active_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ctx, "%s(%s): fatal: %s", __func__, compo->command,
		    e.what());
#if SHUTDOWN_IRC_CONNECTION_BEHAVIOR
		printtext(&ctx, "Shutting down IRC connection...");
		net_request_disconnect();
#endif
	}
}

static bool
is_valid_server(const char *str)
{
	int			dots = 0;
	static const char	serv_chars[] =
	    "abcdefghijklmnopqrstuvwxyz.0123456789-ABCDEFGHIJKLMNOPQRSTUVWXYZ*";
	static const size_t	maxlen = 255;

	if (str == NULL || strings_match(str, "") ||
	    xstrnlen(str, maxlen + 1) > maxlen)
		return false;

	for (const char *cp = &str[0]; *cp != '\0'; cp++) {
		if (strchr(serv_chars, *cp) == NULL)
			return false;
		if (*cp == '.')
			dots++;
	}

	immutable_cp_t last = &str[strlen(str) - 1];

	if (strstr(str, "..") != NULL || strstr(str, "**") != NULL)
		return false;
	else if (*str == '.' || *str == '-')
		return false;
	else if (*last == '.' || *last == '-')
		return false;
	return (dots > 0 ? true : false);
}

/**
 * Checks for a netsplit by looking at the quit message.
 *
 * @param[in] msg Quit message
 * @param[out] serv1 Server one
 * @param[out] serv2 Server two
 * @return True-/falsehood
 */
static bool
is_netsplit(CSTRING msg, std::string &serv1, std::string &serv2)
{
	CSTRING     token[2];
	STRING      last = const_cast<STRING>("");
	STRING      msg_copy = NULL;

	if (g_icb_mode ||
	    strncmp(msg, "Quit: ", 6) == STRINGS_MATCH) {
		serv1.assign("");
		serv2.assign("");
		return false;
	} else if (strings_match_ignore_case(msg, "*.net *.split")) {
		serv1.assign("*.net");
		serv2.assign("*.split");
		return true;
	}

	msg_copy = sw_strdup(msg);

	if (strFeed(msg_copy, 2) != 1 ||
	    (token[0] = strtok_r(msg_copy, "\n", &last)) == NULL ||
	    (token[1] = strtok_r(NULL, "\n", &last)) == NULL ||
	    !is_valid_server(token[0]) ||
	    !is_valid_server(token[1])) {
		free(msg_copy);
		serv1.assign("");
		serv2.assign("");
		return false;
	}

	serv1.assign(token[0]);
	serv2.assign(token[1]);
	free(msg_copy);
	return true;
}

static void
handle_quit(PIRC_WINDOW window, PPRINTTEXT_CONTEXT ptext_ctx,
    struct quit_context *ctx)
{
	bool		ret;
	std::string	host[2];

	ret = is_netsplit(ctx->message, host[0], host[1]);

	if (ret) {
		netsplit *split;
		struct netsplit_context ns_ctx(window->label,
		    host[0].c_str(), host[1].c_str());

		if ((split = netsplit_get_split(&ns_ctx)) == NULL) {
			if (netsplit_create(&ns_ctx, ctx->nick) == ERR) {
				err_log(0, "%s: netsplit_create() error",
				    __func__);
			}
		} else {
			const std::string str(ctx->nick);

			if (!split->join_begun()) {
#if defined(__cplusplus) && __cplusplus >= 201103L
				split->nicks.emplace_back(str);
#else
				split->nicks.push_back(str);
#endif
			} else {
				err_log(0, "%s: netjoin already begun (%s)",
				    __func__, ns_ctx.chan);
			}
		}
	}

	if (!ret && config_bool("joins_parts_quits", true)) {
		ptext_ctx->window = window;
		printtext(ptext_ctx, _("%s%s%c %s%s@%s%s has quit %s%s%s"),
		    COLOR2, ctx->nick, NORMAL,
		    LEFT_BRKT, ctx->user, ctx->host, RIGHT_BRKT,
		    LEFT_BRKT, ctx->message, RIGHT_BRKT);
	}
}

/* event_quit

   Example:
     :<nick>!<user>@<host> QUIT :<message> */
void
event_quit(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	try {
		CSTRING message;
		CSTRING nick, user, host;
		STRING prefix;
		STRING state = const_cast<STRING>("");

		if (compo->prefix == NULL)
			throw std::runtime_error("no prefix");

		message = (*(compo->params) == ':' ? &compo->params[1] :
		    &compo->params[0]);
		prefix = &compo->prefix[1];

		if ((nick = strtok_r(prefix, "!@", &state)) == NULL)
			throw std::runtime_error("unable to get nickname");
		if ((user = strtok_r(NULL, "!@", &state)) == NULL)
			user = "<no user>";
		if ((host = strtok_r(NULL, "!@", &state)) == NULL)
			host = "<no host>";

		printtext_context_init(&ctx, NULL, TYPE_SPEC1_SPEC2, true);

		for (int i = 1; i <= g_ntotal_windows; i++) {
			PIRC_WINDOW	window;

			if ((window = window_by_refnum(i)) != NULL &&
			    is_irc_channel(window->label) &&
			    event_names_htbl_remove(nick, window->label) ==
			    OK) {
				struct quit_context quit_ctx;

				quit_ctx.message = message;
				quit_ctx.nick = nick;
				quit_ctx.user = user;
				quit_ctx.host = host;

				handle_quit(window, &ctx, &quit_ctx);
			}
		}
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN,
		    true);
		printtext(&ctx, "%s(%s): error: %s", __func__, compo->command,
		    e.what());
	}
}

/* event_topic: 332

   Example:
     :irc.server.com 332 <recipient> <channel> :This is the topic. */
void
event_topic(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	try {
		char	*channel, *topic;
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 2) != 2)
			throw std::runtime_error("strFeed");

		/* unused */
		(void) strtok_r(compo->params, "\n", &state);

		if ((channel = strtok_r(NULL, "\n", &state)) == NULL)
			throw std::runtime_error("unable to get channel");
		else if ((topic = strtok_r(NULL, "\n", &state)) == NULL)
			throw std::runtime_error("unable to get topic");
		else if (*topic == ':')
			topic++;

		printtext_context_init(&ctx, NULL, TYPE_SPEC1, true);

		if ((ctx.window = window_by_label(channel)) == NULL)
			throw std::runtime_error("window lookup error");

		new_window_title(channel, topic);
		printtext(&ctx, _("Topic for %s%s%s%c%s: %s"),
		    LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT,
		    topic);
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN,
		    true);
		printtext(&ctx, "%s(%s): error: %s", __func__, compo->command,
		    e.what());
	}
}

/* event_topic_chg

   Example:
     :<nick>!<user>@<host> TOPIC <channel> :New topic */
void
event_topic_chg(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	try {
		char		*prefix = NULL;
		char		*state1 = const_cast<char *>("");
		char		*state2 = const_cast<char *>("");
		const char	*channel, *new_topic;
		const char	*nick, *user, *host;

		if (compo->prefix == NULL)
			throw std::runtime_error("no prefix");

		prefix = &compo->prefix[1];

		if ((nick = strtok_r(prefix, "!@", &state1)) == NULL)
			throw std::runtime_error("no nickname");

		if ((user = strtok_r(NULL, "!@", &state1)) == NULL)
			user = "";
		if ((host = strtok_r(NULL, "!@", &state1)) == NULL)
			host = "";
		UNUSED_VAR(user);
		UNUSED_VAR(host);

		if (strFeed(compo->params, 1) != 1)
			throw std::runtime_error("strFeed");

		if ((channel = strtok_r(compo->params, "\n", &state2)) == NULL)
			throw std::runtime_error("unable to get channel");
		else if ((new_topic = strtok_r(NULL, "\n", &state2)) == NULL)
			throw std::runtime_error("unable to get new topic");
		else if (*new_topic == ':')
			new_topic++;

		printtext_context_init(&ctx, NULL, TYPE_SPEC1, true);

		if ((ctx.window = window_by_label(channel)) == NULL)
			throw std::runtime_error("window lookup error");

		new_window_title(channel, new_topic);
		printtext(&ctx, _("%c%s%c changed the topic of %c%s%c to: %s"),
		    BOLD, nick, BOLD,
		    BOLD, channel, BOLD,
		    new_topic);
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN,
		    true);
		printtext(&ctx, "%s(%s): error: %s", __func__, compo->command,
		    e.what());
	}
}

/* event_topic_creator: 333

   Examples:
     :irc.server.com 333 <recipient> <channel> <nick>!<user>@<host> <time>
     :irc.server.com 333 <recipient> <channel> <nick> <time> */
void
event_topic_creator(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT ctx;
	char *string_copy = NULL;

	try {
		char		 tbuf[100] = { '\0' };
		char		*state1 = const_cast<char *>("");
		char		*state2 = const_cast<char *>("");
		const char	*channel, *creator, *time_str;
		const char	*nick, *user, *host;
		struct tm	 result = { 0 };

		if (strFeed(compo->params, 3) != 3)
			throw std::runtime_error("strFeed");

		/* ignore */
		(void) strtok_r(compo->params, "\n", &state1);

		if ((channel = strtok_r(NULL, "\n", &state1)) == NULL)
			throw std::runtime_error("no channel");
		else if ((creator = strtok_r(NULL, "\n", &state1)) == NULL)
			throw std::runtime_error("no creator");
		else if ((time_str = strtok_r(NULL, "\n", &state1)) == NULL)
			throw std::runtime_error("no time!");
		else if (*time_str == ':')
			time_str++; /* Remove leading colon */

		printtext_context_init(&ctx, NULL, TYPE_SPEC1, true);

		if ((ctx.window = window_by_label(channel)) == NULL)
			throw std::runtime_error("window lookup error");
		else if (!is_numeric(time_str))
			throw std::runtime_error("expected numeric string");

		const time_t timestamp = static_cast<time_t>(strtol(time_str,
		    NULL, 10));

#if defined(UNIX)
		if (localtime_r(&timestamp, &result) == NULL)
			throw std::runtime_error("localtime_r: " TM_STRUCT_MSG);
#elif defined(WIN32)
		if (localtime_s(&result, &timestamp) != 0)
			throw std::runtime_error("localtime_s: " TM_STRUCT_MSG);
#endif

		if (strftime(tbuf, ARRAY_SIZE(tbuf), "%c", &result) == 0)
			throw std::runtime_error("strftime: zero return");

		string_copy = sw_strdup(creator);

		if ((nick = strtok_r(string_copy, "!@", &state2)) == NULL) {
			throw std::runtime_error("no nickname");
		} else if ((user = strtok_r(NULL, "!@", &state2)) != NULL &&
		    (host = strtok_r(NULL, "!@", &state2)) != NULL) {
			printtext(&ctx, _("Topic set by %c%s%c %s%s@%s%s "
			    "%s%s%s"),
			    BOLD, nick, BOLD,
			    LEFT_BRKT, user, host, RIGHT_BRKT,
			    LEFT_BRKT, trim(tbuf), RIGHT_BRKT);
		} else {
			printtext(&ctx, _("Topic set by %c%s%c %s%s%s"),
			    BOLD, nick, BOLD,
			    LEFT_BRKT, trim(tbuf), RIGHT_BRKT);
		}
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN,
		    true);
		printtext(&ctx, "%s(%s): error: %s", __func__, compo->command,
		    e.what());
	}

	free(string_copy);
}
