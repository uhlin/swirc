/* Channel related events
   Copyright (C) 2015-2021 Markus Uhlin. All rights reserved.

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

#include "../assertAPI.h"
#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../irc.h"
#include "../libUtils.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "channel.h"
#include "names.h"

#define SHUTDOWN_IRC_CONNECTION_BEHAVIOR 0
#define TM_STRUCT_MSG "unable to retrieve tm structure"

/* event_chan_hp: 328

   Example:
     :irc.server.com 328 <recipient> <channel> :www.channel.homepage.com */
void
event_chan_hp(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *state = const_cast<char *>("");

	if (strFeed(compo->params, 2) != 2)
	    throw std::runtime_error("strFeed");

	/* unused */
	(void) strtok_r(compo->params, "\n", &state);

	char	*channel  = strtok_r(NULL, "\n", &state);
	char	*homepage = strtok_r(NULL, "\n", &state);

	if (channel == NULL)
	    throw std::runtime_error("unable to get channel");
	else if (homepage == NULL)
	    throw std::runtime_error("unable to get homepage");
	else if (*homepage == ':')
	    homepage++;

	printtext_context_init(&ctx, NULL, TYPE_SPEC1, true);
	if ((ctx.window = window_by_label(channel)) == NULL)
	    throw std::runtime_error("window lookup error");
	printtext(&ctx, "Homepage for %s%s%s%c%s: %s",
	    LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT, homepage);
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "event_chan_hp: error: %s", e.what());
    }
}

/* event_join

   Example:
     :<nick>!<user>@<host> JOIN <channel> */
void
event_join(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *prefix = NULL;
	char *state = const_cast<char *>("");

	if (compo == NULL)
	    throw std::runtime_error("no components");
	else if (compo->prefix == NULL)
	    throw std::runtime_error("no prefix");
	prefix = & (compo->prefix[1]);

	char	*nick = strtok_r(prefix, "!@", &state);
	char	*user = strtok_r(NULL, "!@", &state);
	char	*host = strtok_r(NULL, "!@", &state);

	if (nick == NULL)
	    throw std::runtime_error("no nickname");
	if (user == NULL)
	    user = const_cast<char *>("<no user>");
	if (host == NULL)
	    host = const_cast<char *>("<no host>");

	const char *channel = *(compo->params) == ':'
	    ? &compo->params[1]
	    : &compo->params[0];

	if (!is_irc_channel(channel) ||
	    strpbrk(channel + 1, g_forbidden_chan_name_chars) != NULL)
	    throw std::runtime_error("bogus irc channel");

	if (strings_match_ignore_case(nick, g_my_nickname)) {
	    if (spawn_chat_window(channel, "No title.") != 0)
		throw std::runtime_error("cannot spawn chat window");
	    if (g_am_irc_op && config_bool("auto_op_yourself", true)) {
		net_send("MODE %s +o %s", channel, nick);
		net_send("SAMODE %s +o %s", channel, nick);
	    }
	} else if (event_names_htbl_insert(nick, channel) != OK) {
	    throw std::runtime_error("unable to add user to channel list");
	}

	const bool joins_parts_quits =
	    config_bool("joins_parts_quits", true);

	if (joins_parts_quits) {
	    printtext_context_init(&ctx, NULL, TYPE_SPEC1_SPEC2, true);

	    if ((ctx.window = window_by_label(channel)) == NULL)
		throw std::runtime_error("window lookup error");

	    printtext(&ctx, "%s%s%c %s%s@%s%s has joined %s%s%c",
		COLOR1, nick, NORMAL, LEFT_BRKT, user, host, RIGHT_BRKT,
		COLOR2, channel, NORMAL);
	}
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "event_join: fatal: %s", e.what());
#if SHUTDOWN_IRC_CONNECTION_BEHAVIOR
	printtext(&ctx, "Shutting down IRC connection...");
	g_on_air = false;
#endif
    }
}

/* event_kick

   Examples:
     :<nick>!<user>@<host> KICK <channel> <victim> :<reason> */
void
event_kick(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *prefix = NULL;
	char *state1 = const_cast<char *>("");
	char *state2 = const_cast<char *>("");

	if (compo == NULL)
	    throw std::runtime_error("no components");
	else if (compo->prefix == NULL)
	    throw std::runtime_error("no prefix");
	prefix = & (compo->prefix[1]);

	char *nick = strtok_r(prefix, "!@", &state1);
	char *user = strtok_r(NULL, "!@", &state1);
	char *host = strtok_r(NULL, "!@", &state1);

	if (nick == NULL)
	    throw std::runtime_error("no nickname");
	if (user == NULL)
	    user = const_cast<char *>("<no user>");
	if (host == NULL)
	    host = const_cast<char *>("<no host>");

	/* unused */
	(void) user;
	(void) host;

	(void) strFeed(compo->params, 2);

	char	*channel = strtok_r(compo->params, "\n", &state2);
	char	*victim	 = strtok_r(NULL, "\n", &state2);
	char	*reason	 = strtok_r(NULL, "\n", &state2);

	if (channel == NULL)
	    throw std::runtime_error("no channel");
	else if (victim == NULL)
	    throw std::runtime_error("no victim");

	const bool has_reason = reason != NULL;
	if (has_reason && *reason == ':')
	    reason++;

	if (strings_match_ignore_case(victim, g_my_nickname)) {
	    if (config_bool("kick_close_window", true)) {
		if (destroy_chat_window(channel) != 0)
		    throw std::runtime_error("failed to destroy chat window");
	    } else {
		event_names_htbl_remove_all(window_by_label(channel));
	    }
	} else if (event_names_htbl_remove(victim, channel) != OK) {
	    throw std::runtime_error("failed to remove victim from channel");
	}

	printtext_context_init(&ctx, NULL, TYPE_SPEC1_SPEC2, true);

	if ((ctx.window = window_by_label(channel)) == NULL)
	    ctx.window = g_active_window;

	printtext(&ctx, "%s was kicked from %s%s%c by %s%s%c %s%s%s",
	    victim, COLOR2, channel, NORMAL, COLOR2, nick, NORMAL,
	    LEFT_BRKT, has_reason ? reason : "", RIGHT_BRKT);
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "event_kick: fatal: %s", e.what());
#if SHUTDOWN_IRC_CONNECTION_BEHAVIOR
	printtext(&ctx, "Shutting down IRC connection...");
	g_on_air = false;
#endif
    }
}

static void
chg_status_for_owner(plus_minus_state_t pm_state,
		     const char *nick,
		     const char *channel)
{
    /* On ircd-seven +q means quiet and works like +b (ban user), but
     * allows matching users to join the channel. However: on InspIRCd
     * setting +q makes user channel owner. This should fix the
     * problem. */
    if (!is_valid_nickname(nick))
	return;

    switch (pm_state) {
    case STATE_PLUS:
	if (event_names_htbl_modify_owner(nick, channel, true) != OK)
	    err_log(0, "In chg_status_for_owner: "
		"error: event_names_htbl_modify_owner");
	break;
    case STATE_MINUS:
	if (event_names_htbl_modify_owner(nick, channel, false) != OK)
	    err_log(0, "In chg_status_for_owner: "
		"error: event_names_htbl_modify_owner");
	break;
    case STATE_NEITHER_PM:
    default:
	sw_assert_not_reached();
    }
}

static void
chg_status_for_superop(plus_minus_state_t pm_state,
		       const char *nick,
		       const char *channel)
{
    switch (pm_state) {
    case STATE_PLUS:
	if (event_names_htbl_modify_superop(nick, channel, true) != OK)
	    err_log(0, "In chg_status_for_superop: "
		"error: event_names_htbl_modify_superop");
	break;
    case STATE_MINUS:
	if (event_names_htbl_modify_superop(nick, channel, false) != OK)
	    err_log(0, "In chg_status_for_superop: "
		"error: event_names_htbl_modify_superop");
	break;
    case STATE_NEITHER_PM:
    default:
	sw_assert_not_reached();
    }
}

static void
chg_status_for_op(plus_minus_state_t pm_state,
		  const char *nick,
		  const char *channel)
{
    switch (pm_state) {
    case STATE_PLUS:
	if (event_names_htbl_modify_op(nick, channel, true) != OK)
	    err_log(0, "In chg_status_for_op: "
		"error: event_names_htbl_modify_op");
	break;
    case STATE_MINUS:
	if (event_names_htbl_modify_op(nick, channel, false) != OK)
	    err_log(0, "In chg_status_for_op: "
		"error: event_names_htbl_modify_op");
	break;
    case STATE_NEITHER_PM:
    default:
	sw_assert_not_reached();
    }
}

static void
chg_status_for_halfop(plus_minus_state_t pm_state,
		      const char *nick,
		      const char *channel)
{
    switch (pm_state) {
    case STATE_PLUS:
	if (event_names_htbl_modify_halfop(nick, channel, true) != OK)
	    err_log(0, "In chg_status_for_halfop: "
		"error: event_names_htbl_modify_halfop");
	break;
    case STATE_MINUS:
	if (event_names_htbl_modify_halfop(nick, channel, false) != OK)
	    err_log(0, "In chg_status_for_halfop: "
		"error: event_names_htbl_modify_halfop");
	break;
    case STATE_NEITHER_PM:
    default:
	sw_assert_not_reached();
    }
}

static void
chg_status_for_voice(plus_minus_state_t pm_state,
		     const char *nick,
		     const char *channel)
{
    switch (pm_state) {
    case STATE_PLUS:
	if (event_names_htbl_modify_voice(nick, channel, true) != OK)
	    err_log(0, "In chg_status_for_voice: "
		"error: event_names_htbl_modify_voice");
	break;
    case STATE_MINUS:
	if (event_names_htbl_modify_voice(nick, channel, false) != OK)
	    err_log(0, "In chg_status_for_voice: "
		"error: event_names_htbl_modify_voice");
	break;
    case STATE_NEITHER_PM:
    default:
	sw_assert_not_reached();
    }
}

static void	maintain_channel_stats(const char *, const char *)
		    PTR_ARGS_NONNULL;

/* Example input: +vvv nick1 nick2 nick3 */
static void
maintain_channel_stats(const char *channel, const char *input)
{
    char		*nicks[15]      = { NULL };
    size_t		 ar_i           = 0;
    const size_t	 ar_sz          = ARRAY_SIZE(nicks);
    char		*input_copy     = sw_strdup(input);
    char		*modes          = const_cast<char *>("");
    size_t		 nicks_assigned = 0;
    char		*state          = const_cast<char *>("");
    plus_minus_state_t	 pm_state       = STATE_NEITHER_PM;

    if ((modes = strtok_r(input_copy, " ", &state)) == NULL) {
	err_log(EINVAL, "maintain_channel_stats: strtok_r: no modes!");
	free(input_copy);
	return;
    }

    for (nicks_assigned = 0;; nicks_assigned++) {
	char *token = strtok_r(NULL, " ", &state);

	if (token && nicks_assigned < ar_sz)
	    nicks[nicks_assigned] = sw_strdup(token);
	else
	    break;
    }

    for (char *cp = modes; *cp && ar_i < nicks_assigned; cp++) {
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

    if (strspn(modes, "+-Ibeqaohv") != strlen(modes))
	net_send("MODE %s", channel);

    free(input_copy);

    /* destroy the array */
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
	char *cp = NULL;
	char *nick = NULL;
	char *prefix = NULL;
	char *state1 = const_cast<char *>("");
	char *state2 = const_cast<char *>("");

	if (compo->prefix == NULL)
	    throw std::runtime_error("no prefix");
	prefix = & (compo->prefix[1]);

	if ((nick = strtok_r(prefix, "!@", &state1)) == NULL)
	    throw std::runtime_error("unable to get nickname in prefix");
	else if (strFeed(compo->params, 1) != 1)
	    throw std::runtime_error("strFeed");

	char *channel = strtok_r(compo->params, "\n", &state2);
	char *next_token = strtok_r(NULL, "\n", &state2);

	if (channel == NULL || next_token == NULL)
	    throw std::runtime_error("insufficient data");
	else if (*next_token == ':')
	    next_token ++;
	else if ((cp = strstr(next_token, " :")) != NULL) {
	    *++cp = ' ';
	    (void) memmove(cp - 1, cp, strlen(cp) + 1);
	}

	next_token_copy = sw_strdup(next_token);
	//squeeze(next_token_copy, ":");
	trim(next_token_copy);
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1, true);

	if (strings_match_ignore_case(nick, channel)) {
	    /*
	     * User mode
	     */
	    printtext(&ctx, "Mode change %s%s%s for user %c%s%c",
		LEFT_BRKT, next_token_copy, RIGHT_BRKT, BOLD, nick, BOLD);
	    net_send("MODE %s", nick);
	} else if (is_irc_channel(channel) &&
		   (ctx.window = window_by_label(channel)) != NULL) {
	    printtext(&ctx, "mode/%s%s%s%c%s %s%s%s by %s%s%c",
		LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT,
		LEFT_BRKT, next_token_copy, RIGHT_BRKT,
		COLOR2, nick, NORMAL);
	    maintain_channel_stats(channel, next_token_copy);
	} else {
	    throw std::runtime_error("unhandled else branch");
	}
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "event_mode: error: %s", e.what());
    }

    free(next_token_copy);
}

static int
RemoveAndInsertNick(const char *old_nick, const char *new_nick,
		    const char *label)
{
    PNAMES p;
    bool is_owner, is_superop, is_op, is_halfop, is_voice;

    if ((p = event_names_htbl_lookup(old_nick, label)) == NULL)
	return ERR; /* non-fatal: old_nick not found on channel */

    is_owner   = p->is_owner;
    is_superop = p->is_superop;
    is_op      = p->is_op;
    is_halfop  = p->is_halfop;
    is_voice   = p->is_voice;

    if (event_names_htbl_remove(old_nick, label) != OK) {
	err_log(0, "RemoveAndInsertNick: event_names_htbl_remove");
	return ERR;
    } else if (event_names_htbl_insert(new_nick, label) != OK) {
	err_log(0, "RemoveAndInsertNick: event_names_htbl_insert");
	return ERR;
    }

    /* XXX: Reverse order */
    if (is_voice)   chg_status_for_voice(STATE_PLUS, new_nick, label);
    if (is_halfop)  chg_status_for_halfop(STATE_PLUS, new_nick, label);
    if (is_op)      chg_status_for_op(STATE_PLUS, new_nick, label);
    if (is_superop) chg_status_for_superop(STATE_PLUS, new_nick, label);
    if (is_owner)   chg_status_for_owner(STATE_PLUS, new_nick, label);

    return OK;
}

/* event_nick

   Example:
     :<nick>!<user>@<host> NICK :<new nick> */
void
event_nick(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *new_nick = *(compo->params) == ':'
	    ? &compo->params[1]
	    : &compo->params[0];
	char *prefix = NULL;
	char *state = const_cast<char *>("");

	if (compo->prefix == NULL)
	    throw std::runtime_error("no prefix");
	prefix = & (compo->prefix[1]);

	char *nick = strtok_r(prefix, "!@", &state);
	char *user = strtok_r(NULL, "!@", &state);
	char *host = strtok_r(NULL, "!@", &state);

	if (nick == NULL)
	    throw std::runtime_error("no nickname");

	/* unused */
	(void) user;
	(void) host;

	printtext_context_init(&ctx, NULL, TYPE_SPEC1_SPEC2, true);

	for (int i = 1; i <= g_ntotal_windows; i++) {
	    PIRC_WINDOW window = window_by_refnum(i);

	    if (window && is_irc_channel(window->label) &&
		RemoveAndInsertNick(nick, new_nick, window->label) == OK) {
		ctx.window = window;

		printtext(&ctx, "%s%s%c is now known as %s %s%s%c",
		    COLOR2, nick, NORMAL, THE_SPEC2, COLOR1, new_nick, NORMAL);
	    }
	}

	if (strings_match_ignore_case(nick, g_my_nickname))
	    irc_set_my_nickname(new_nick);
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "event_nick: error: %s", e.what());
    }
}

/* event_part

   Examples:
     :<nick>!<user>@<host> PART <channel>
     :<nick>!<user>@<host> PART <channel> :<message> */
void
event_part(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *prefix = NULL;
	char *state1 = const_cast<char *>("");
	char *state2 = const_cast<char *>("");

	if (compo->prefix == NULL)
	    throw std::runtime_error("no prefix!");
	prefix = & (compo->prefix[1]);

	char *nick = strtok_r(prefix, "!@", &state1);
	char *user = strtok_r(NULL, "!@", &state1);
	char *host = strtok_r(NULL, "!@", &state1);

	if (nick == NULL)
	    throw std::runtime_error("unable to get nickname");

	if (user == NULL)
	    user = const_cast<char *>("<no user>");
	if (host == NULL)
	    host = const_cast<char *>("<no host>");

	const bool has_message = strFeed(compo->params, 1) == 1;
	char *channel = strtok_r(compo->params, "\n", &state2);
	char *message = strtok_r(NULL, "\n", &state2);

	if (channel == NULL)
	    throw std::runtime_error("unable to get channel");
	else if (*channel == ':')
	    channel++;

	if (strings_match_ignore_case(nick, g_my_nickname)) {
	    if (destroy_chat_window(channel) != 0)
		throw std::runtime_error("failed to destroy chat window");
	    return;
	} else {
	    if (event_names_htbl_remove(nick, channel) != OK) {
		throw std::runtime_error("failed to remove user from "
		    "channel list");
	    }
	}

	const bool joins_parts_quits =
	    config_bool("joins_parts_quits", true);

	if (joins_parts_quits) {
	    printtext_context_init(&ctx, NULL, TYPE_SPEC1_SPEC2, true);

	    if ((ctx.window = window_by_label(channel)) == NULL)
		throw std::runtime_error("window lookup error");
	    if (!has_message)
		message = const_cast<char *>("");
	    if (has_message && *message == ':')
		message++;

	    printtext(&ctx, "%s%s%c %s%s@%s%s has left %s%s%c %s%s%s",
		COLOR2, nick, NORMAL, LEFT_BRKT, user, host, RIGHT_BRKT,
		COLOR2, channel, NORMAL,
		LEFT_BRKT, message, RIGHT_BRKT);
	}
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "event_part: fatal: %s", e.what());
#if SHUTDOWN_IRC_CONNECTION_BEHAVIOR
	printtext(&ctx, "Shutting down IRC connection...");
	g_on_air = false;
#endif
    }
}

/* event_quit

   Example:
     :<nick>!<user>@<host> QUIT :<message> */
void
event_quit(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *message = *(compo->params) == ':'
	    ? &compo->params[1]
	    : &compo->params[0];
	char *prefix = NULL;
	char *state = const_cast<char *>("");

	if (compo->prefix == NULL)
	    throw std::runtime_error("no prefix");
	prefix = & (compo->prefix[1]);

	char *nick = strtok_r(prefix, "!@", &state);
	char *user = strtok_r(NULL, "!@", &state);
	char *host = strtok_r(NULL, "!@", &state);

	if (nick == NULL)
	    throw std::runtime_error("unable to get nickname");
	if (user == NULL)
	    user = const_cast<char *>("<no user>");
	if (host == NULL)
	    host = const_cast<char *>("<no host>");

	printtext_context_init(&ctx, NULL, TYPE_SPEC1_SPEC2, true);

	for (int i = 1; i <= g_ntotal_windows; i++) {
	    PIRC_WINDOW window = window_by_refnum(i);

	    if (window && is_irc_channel(window->label) &&
		event_names_htbl_remove(nick, window->label) == OK) {
		const bool joins_parts_quits =
		    config_bool("joins_parts_quits", true);

		if (joins_parts_quits) {
		    ctx.window = window;

		    printtext(&ctx, "%s%s%c %s%s@%s%s has quit %s%s%s",
			COLOR2, nick, NORMAL, LEFT_BRKT, user, host, RIGHT_BRKT,
			LEFT_BRKT, message, RIGHT_BRKT);
		}
	    }
	}
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "event_quit: error: %s", e.what());
    }
}

/* event_topic: 332

   Example:
     :irc.server.com 332 <recipient> <channel> :This is the topic. */
void
event_topic(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *channel = NULL, *topic = NULL;
	char *state = const_cast<char *>("");

	if (strFeed(compo->params, 2) != 2)
	    throw std::runtime_error("strFeed");

	/* unused */
	(void) strtok_r(compo->params, "\n", &state);

	channel = strtok_r(NULL, "\n", &state);
	topic = strtok_r(NULL, "\n", &state);

	if (channel == NULL)
	    throw std::runtime_error("unable to get channel");
	else if (topic == NULL)
	    throw std::runtime_error("unable to get topic");
	else if (*topic == ':')
	    topic++;

	printtext_context_init(&ctx, NULL, TYPE_SPEC1, true);

	if ((ctx.window = window_by_label(channel)) == NULL)
	    throw std::runtime_error("window lookup error");

	printtext(&ctx, "Topic for %s%s%s%c%s: %s",
	    LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT, topic);

	new_window_title(channel, topic);
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "event_topic: error: %s", e.what());
    }
}

/* event_topic_chg

   Example:
     :<nick>!<user>@<host> TOPIC <channel> :New topic */
void
event_topic_chg(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *prefix = NULL;
	char *state1 = const_cast<char *>("");
	char *state2 = const_cast<char *>("");

	if (compo->prefix == NULL)
	    throw std::runtime_error("no prefix");
	prefix = & (compo->prefix[1]);

	char *nick = strtok_r(prefix, "!@", &state1);
	char *user = strtok_r(NULL, "!@", &state1);
	char *host = strtok_r(NULL, "!@", &state1);

	if (nick == NULL)
	    throw std::runtime_error("no nickname");

	/* unused */
	(void) user;
	(void) host;

	if (strFeed(compo->params, 1) != 1)
	    throw std::runtime_error("strFeed");

	char *channel = strtok_r(compo->params, "\n", &state2);
	char *new_topic = strtok_r(NULL, "\n", &state2);

	if (channel == NULL)
	    throw std::runtime_error("unable to get channel");
	else if (new_topic == NULL)
	    throw std::runtime_error("unable to get new topic");
	else if (*new_topic == ':')
	    new_topic++;

	printtext_context_init(&ctx, NULL, TYPE_SPEC1, true);

	if ((ctx.window = window_by_label(channel)) == NULL)
	    throw std::runtime_error("window lookup error");

	new_window_title(channel, new_topic);

	printtext(&ctx, "%c%s%c changed the topic of %c%s%c to: %s",
	    BOLD, nick, BOLD, BOLD, channel, BOLD, new_topic);
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "event_topic_chg: error: %s", e.what());
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
	char *state1 = const_cast<char *>("");
	char *state2 = const_cast<char *>("");

	if (strFeed(compo->params, 3) != 3)
	    throw std::runtime_error("strFeed");

	/* ignore */
	(void) strtok_r(compo->params, "\n", &state1);

	char	*channel  = strtok_r(NULL, "\n", &state1);
	char	*creator  = strtok_r(NULL, "\n", &state1);
	char	*time_str = strtok_r(NULL, "\n", &state1);

	if (channel == NULL)
	    throw std::runtime_error("no channel");
	else if (creator == NULL)
	    throw std::runtime_error("no creator");
	else if (time_str == NULL)
	    throw std::runtime_error("no time!");
	else if (*time_str == ':')
	    time_str++; /* Remove leading colon */

	printtext_context_init(&ctx, NULL, TYPE_SPEC1, true);

	if ((ctx.window = window_by_label(channel)) == NULL)
	    throw std::runtime_error("window lookup error");
	else if (!is_numeric(time_str))
	    throw std::runtime_error("expected numeric string");

	const time_t timestamp = (time_t) strtol(time_str, NULL, 10);
	struct tm result = { 0 };

#if defined(UNIX)
	if (localtime_r(&timestamp, &result) == NULL)
	    throw std::runtime_error("localtime_r: " TM_STRUCT_MSG);
#elif defined(WIN32)
	if (localtime_s(&result, &timestamp) != 0)
	    throw std::runtime_error("localtime_s: " TM_STRUCT_MSG);
#endif

	char tbuf[100] = { '\0' };

	if (strftime(tbuf, ARRAY_SIZE(tbuf), "%c", &result) == 0)
	    throw std::runtime_error("strftime: zero return");

	string_copy = sw_strdup(creator);
	char *nick = strtok_r(string_copy, "!@", &state2);
	char *user = strtok_r(NULL, "!@", &state2);
	char *host = strtok_r(NULL, "!@", &state2);

	if (nick == NULL)
	    throw std::runtime_error("no nickname");
	else if (user && host) {
	    printtext(&ctx, "Topic set by %c%s%c %s%s@%s%s %s%s%s",
		BOLD, nick, BOLD,
		LEFT_BRKT, user, host, RIGHT_BRKT,
		LEFT_BRKT, trim(tbuf), RIGHT_BRKT);
	} else {
	    printtext(&ctx, "Topic set by %c%s%c %s%s%s",
		BOLD, nick, BOLD,
		LEFT_BRKT, trim(tbuf), RIGHT_BRKT);
	}
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "event_topic_creator: error: %s", e.what());
    }

    free(string_copy);
}
