/* Miscellaneous events
   Copyright (C) 2014-2020 Markus Uhlin. All rights reserved.

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
#include "../network.h"
#include "../printtext.h"
#include "../readline.h"
#include "../statusbar.h"
#include "../strHand.h"
#include "../theme.h"

#include "misc.h"
#include "welcome.h"

/* Not written for a specific event */
void
event_allaround_extract_find_colon(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;
    char *cp = NULL;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);

    if ((cp = strchr(compo->params, ':')) == NULL) {
	printtext(&ctx, "on issuing event %s: no colon found", compo->command);
	return;
    }

#if 0
    if (!strings_match(compo->command, "444") &&
	!strings_match(compo->command, "484"))
	ctx.spec_type = TYPE_SPEC1;
#endif
    printtext(&ctx, "%s", ++cp);
}

/* This function isn't written for a specific event. It extracts the
   message and removes the first colon in it.

   Events that uses this function:
     :irc.server.com 042 <nickname> XXXXXXXXX :your unique ID
     :irc.server.com 252 <nickname> <integer> :operator(s) online
     :irc.server.com 253 <nickname> <integer> :unknown connections
     :irc.server.com 254 <nickname> <integer> :channels formed
     :irc.server.com 396 <nickname> <hostname> :is now your displayed host */
void
event_allaround_extract_remove_colon(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;
    char *cp = NULL;
    char *msg = NULL, *msg_copy = NULL;
    char *state = const_cast<char *>("");

    try {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1, true);

	if (strFeed(compo->params, 1) != 1)
	    throw std::runtime_error("strFeed");

	/* unused */
	(void) strtok_r(compo->params, "\n", &state);

	if ((msg = strtok_r(NULL, "\n", &state)) == NULL)
	    throw std::runtime_error("unable to get message");

	msg_copy = sw_strdup(msg);

	if ((cp = strchr(msg_copy, ':')) == NULL)
	    throw std::runtime_error("no colon found!");

	cp++;
	(void) memmove(cp - 1, cp, strlen(cp) + 1);
	printtext(&ctx, "%s", msg_copy);
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "on processing event %s: error: %s",
		  compo->command, e.what());
    }

    free(msg_copy);
}

/* event_serverFeatures: 005 (RPL_ISUPPORT)

   Lists features supported by the server.
   (sent as connection registration is completed)

   Example:
     :irc.server.com 005 <anything> ... */
void
event_serverFeatures(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *cp = NULL;
	char *msg = NULL, *msg_copy = NULL;
	char *state = const_cast<char *>("");

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1, true);

	if (strFeed(compo->params, 1) != 1)
	    throw std::runtime_error("strFeed");

	/* unused */
	(void) strtok_r(compo->params, "\n", &state);

	if ((msg = strtok_r(NULL, "\n", &state)) == NULL)
	    throw std::runtime_error("no message!");
	if (*msg == ':')
	    msg++;
	if (*msg) {
	    const char *ar[] = {
		":are available on this server",
		":are supported by this server",
		":are supported on this server",
	    };

	    msg_copy = sw_strdup(msg);

	    for (const char **ar_p = &ar[0];
		 ar_p < &ar[ARRAY_SIZE(ar)]; ar_p++) {
		while ((cp = strstr(msg_copy, *ar_p)) != NULL) {
		    /*
		     * Delete the colon
		     */
		    cp++;
		    (void) memmove(cp - 1, cp, strlen(cp) + 1);
		}
	    }

	    printtext(&ctx, "%s", msg_copy);
	    free(msg_copy);
	}
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "event_serverFeatures: error: %s", e.what());
    }
}

/* event_channelCreatedWhen: 329 (undocumented in the RFC)

   Example:
     :irc.server.com 329 <my nickname> <channel> <seconds> */
void
event_channelCreatedWhen(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *state = const_cast<char *>("");

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	if (strFeed(compo->params, 2) != 2)
	    throw std::runtime_error("strFeed");

	/* my nickname */
	(void) strtok_r(compo->params, "\n", &state);

	char	*channel = strtok_r(NULL, "\n", &state);
	char	*seconds = strtok_r(NULL, "\n", &state);

	if (channel == NULL)
	    throw std::runtime_error("unable to get channel");
	else if (seconds == NULL)
	    throw std::runtime_error("unable to get seconds");
	else if (*seconds == ':')
	    seconds++; /* Remove leading colon */

	if (!is_irc_channel(channel))
	    throw std::runtime_error("invalid irc channel");
	else if (!is_numeric(seconds))
	    throw std::runtime_error("expected numeric string");
	else if ((ctx.window = window_by_label(channel)) == NULL)
	    throw std::runtime_error("couldn't find channel window");
	else if (ctx.window->received_chancreated)
	    return;

	const time_t date_of_creation = (time_t) strtol(seconds, NULL, 10);
	struct tm result = { 0 };

#define TM_STRUCT_MSG "unable to retrieve tm structure"
#if defined(UNIX)
	if (localtime_r(&date_of_creation, &result) == NULL)
	    throw std::runtime_error("localtime_r: " TM_STRUCT_MSG);
#elif defined(WIN32)
	if (localtime_s(&result, &date_of_creation) != 0)
	    throw std::runtime_error("localtime_s: " TM_STRUCT_MSG);
#endif

	char tbuf[100] = { '\0' };

	if (strftime(tbuf, ARRAY_SIZE(tbuf), "%c", &result) == 0)
	    throw std::runtime_error("strftime: zero return");

	printtext(&ctx, "Channel %s%s%s%c%s created %s",
	    LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT,
	    trim(tbuf));
	ctx.window->received_chancreated = true;
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "event_channelCreatedWhen: error: %s", e.what());
    }
}

/* event_channelModeIs: 324 (RPL_CHANNELMODEIS)

   Examples:
     :irc.server.com 324 <my nickname> <channel> <mode> <mode params>
     :irc.server.com 324 <my nickname> <channel> <mode> */
void
event_channelModeIs(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *cp = NULL;
	char *state = const_cast<char *>("");

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	if (strFeed(compo->params, 2) != 2)
	    throw std::runtime_error("strFeed");

	/* my nickname */
	(void) strtok_r(compo->params, "\n", &state);

	char	*channel = strtok_r(NULL, "\n", &state);
	char	*data	 = strtok_r(NULL, "\n", &state);

	if (channel == NULL)
	    throw std::runtime_error("no channel");
	else if (data == NULL)
	    throw std::runtime_error("no data");
	else if ((ctx.window = window_by_label(channel)) == NULL)
	    throw std::runtime_error("couldn't find channel window");

	if (*data == ':')
	    data++;
	else if ((cp = strstr(data, " :")) != NULL) {
	    *++cp = ' ';
	    (void) memmove(cp - 1, cp, strlen(cp) + 1);
	}

	/* -------------------------------------------------- */

	errno = sw_strcpy(ctx.window->chanmodes, trim(data),
	    sizeof(ctx.window->chanmodes));
	if (errno)
	    throw std::runtime_error("unable to store channel modes");
	else if (! (ctx.window->received_chanmodes)) {
	    printtext(&ctx, "mode/%s%s%s%c%s %s%s%s",
		LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT,
		LEFT_BRKT, data, RIGHT_BRKT);
	    ctx.window->received_chanmodes = true;
	}

	statusbar_update_display_beta();
	readline_top_panel();
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "event_channelModeIs: error: %s", e.what());
    }
}

/* event_channel_forward: 470 (undocumented)

   Example:
     :irc.server.com 470 <my nickname> <from channel> <to channel>
                         :Forwarding to another channel

   Notes:
     The JOIN event is sent AFTER event 470 */
void
event_channel_forward(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *params = & (compo->params[0]);
	char *state = const_cast<char *>("");

	if (strFeed(params, 3) != 3)
	    throw std::runtime_error("strFeed");

	/* my nickname */
	(void) strtok_r(params, "\n", &state);

	char	*from_channel = strtok_r(NULL, "\n", &state);
	char	*to_channel   = strtok_r(NULL, "\n", &state);
	char	*msg	      = strtok_r(NULL, "\n", &state);

	if (from_channel == NULL)
	    throw std::runtime_error("no origin channel");
	else if (to_channel == NULL)
	    throw std::runtime_error("no destination channel");
	else if (msg == NULL)
	    throw std::runtime_error("no message");

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1, true);
	printtext(&ctx, "Channel forwarding from %c%s%c to %c%s%c",
	    BOLD, from_channel, BOLD, BOLD, to_channel, BOLD);
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "event_channel_forward: error: %s", e.what());
    }
}

/* event_local_and_global_users: 265, 266

   These aren't documented at the time of writing this. They're sent
   upon successful registration.

   Examples:
     :irc.server.com 265 <nickname> <#> <#>
                         :Current local users <#>, max <#>
     :irc.server.com 266 <nickname> <#> <#>
                         :Current global users <#>, max <#> */
void
event_local_and_global_users(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;
    const char *ccp = strchr(compo->params, ':');

    if (ccp == NULL || strings_match(++ccp, "")) {
	return;
    }

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1, true);
    printtext(&ctx, "%s", ccp);
}

/* event_nicknameInUse: 433
   TODO: Investigate thread termination!

   Example:
     :irc.server.com 433 * <nick> :Nickname is already in use. */
void
event_nicknameInUse(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *nick = NULL;
	char *params = & (compo->params[0]);
	char *state = const_cast<char *>("");

	if (strFeed(params, 2) != 2)
	    throw std::runtime_error("strFeed");

	/* unused */
	(void) strtok_r(params, "\n", &state);

	if ((nick = strtok_r(NULL, "\n", &state)) == NULL)
	    throw std::runtime_error("no nickname");

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "Nickname %c%s%c is already in use", BOLD, nick, BOLD);

	if (!atomic_load_bool(&g_connection_in_progress))
	    return;

	/* -------------------------------------------------- */

	ctx.spec_type = TYPE_SPEC1;
	if (g_alt_nick_tested) {
	    printtext(&ctx, "Alternative nickname already tested. "
		"Disconnecting...");
	    g_on_air = false;
	    event_welcome_signalit();
	} else if (!g_alt_nick_tested && !isEmpty(Config("alt_nick"))) {
	    printtext(&ctx, "Attempting to use alt_nick (%s) instead...",
		      Config("alt_nick"));
	    net_send("NICK %s", Config("alt_nick"));
	    g_alt_nick_tested = true;
	} else {
	    printtext(&ctx, "Disconnecting...");
	    g_on_air = false;
	    event_welcome_signalit();
	}
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "event_nicknameInUse: error: %s", e.what());
    }
}

/* event_userModeIs: 221 (RPL_UMODEIS)

   Example:
     :irc.server.com 221 <my nickname> <modes> */
void
event_userModeIs(struct irc_message_compo *compo)
{
    try {
	char *modes = NULL;
	char *state = const_cast<char *>("");

	if (strFeed(compo->params, 1) != 1)
	    throw std::runtime_error("strFeed");

	/* my nickname */
	(void) strtok_r(compo->params, "\n", &state);

	if ((modes = strtok_r(NULL, "\n", &state)) == NULL)
	    throw std::runtime_error("no modes");
	else if (strings_match(modes, g_user_modes))
	    return; /* no change */
	else if (sw_strcpy(g_user_modes, modes, sizeof g_user_modes) != 0)
	    throw std::runtime_error("unable to store user modes!");

	statusbar_update_display_beta();
	readline_top_panel();
    } catch (std::runtime_error &e) {
	PRINTTEXT_CONTEXT ctx;

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "event_userModeIs: error: %s", e.what());
    }
}

/* event_youAreOper: 381 (RPL_YOUREOPER)

   Example:
     :irc.server.com 381 ... */
void
event_youAreOper(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    (void) compo;

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_SUCCESS, true);
    printtext(&ctx, "You're now an IRC operator!");
    printtext(&ctx, "    auto_op_yourself = %s",
	config_bool_unparse("auto_op_yourself", true) ? "ON" : "OFF");
    g_am_irc_op = true;
}
