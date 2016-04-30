/* Miscellaneous events
   Copyright (C) 2014-2016 Markus Uhlin. All rights reserved.

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

#include "../config.h"
#include "../irc.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "misc.h"
#include "welcome.h"

void
event_bounce(struct irc_message_compo *compo)
{
    char *cp;
    char *msg, *msg_copy;
    char *state = "";
    struct printtext_context ctx = {
	.window     = g_status_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    if (Strfeed(compo->params, 1) != 1) {
	printtext(&ctx, "In event_bounce: Strfeed(..., 1) != 1");
	goto failure;
    }

    (void) strtok_r(compo->params, "\n", &state);

    if ((msg = strtok_r(NULL, "\n", &state)) == NULL) {
	printtext(&ctx, "In event_bounce: Unable to extract message");
	goto failure;
    }

    if (*msg == ':') {
	msg++;
    }

    if (*msg) {
	msg_copy = sw_strdup(msg);

	while (cp = strstr(msg_copy, ":are supported by this server"), cp != NULL) {
	    cp++;
	    (void) memmove(cp - 1, cp, strlen(cp) + 1); /* Delete the colon */
	}

	while (cp = strstr(msg_copy, ":are available on this server"), cp != NULL) {
	    cp++;
	    (void) memmove(cp - 1, cp, strlen(cp) + 1); /* Delete the colon */
	}

	ctx.spec_type = TYPE_SPEC1;
	printtext(&ctx, "%s", msg_copy);
	free(msg_copy);
    }

    return;

  failure:
    irc_unsuccessful_event_cleanup();
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
    char *cp;
    char *msg, *msg_copy;
    char *state = "";
    struct printtext_context ctx = {
	.window     = g_status_window,
	.spec_type  = TYPE_SPEC1_WARN,
	.include_ts = true,
    };

    if (Strfeed(compo->params, 1) != 1) {
	printtext(&ctx, "On issuing event %s: Strfeed(..., 1) != 1", compo->command);
	return;
    }

    (void) strtok_r(compo->params, "\n", &state);

    if ((msg = strtok_r(NULL, "\n", &state)) == NULL) {
	printtext(&ctx, "On issuing event %s: Unable to extract message", compo->command);
	return;
    }

    msg_copy = sw_strdup(msg);

    if ((cp = strchr(msg_copy, ':')) == NULL) {
	printtext(&ctx, "On issuing event %s: No colon found", compo->command);
	free(msg_copy);
	return;
    }

    cp++;
    (void) memmove(cp - 1, cp, strlen(cp) + 1);
    ctx.spec_type = TYPE_SPEC1;
    printtext(&ctx, "%s", msg_copy);
    free(msg_copy);
}

/* event_local_and_global_users: 265, 266

   These aren't documented at the time of writing this. They're sent
   upon successful registration.

   Examples:
     :irc.server.com 265 <nickname> <#> <#> :Current local users <#>, max <#>
     :irc.server.com 266 <nickname> <#> <#> :Current global users <#>, max <#> */
void
event_local_and_global_users(struct irc_message_compo *compo)
{
    const char *ccp = strchr(compo->params, ':');
    struct printtext_context ctx;

    if (ccp == NULL || Strings_match(++ccp, "")) {
	return;
    }

    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1;
    ctx.include_ts = true;
    printtext(&ctx, "%s", ccp);
}

/* event_nicknameInUse: 433

   Example:
     :irc.server.com 433 * <nick> :Nickname is already in use. */
void
event_nicknameInUse(struct irc_message_compo *compo)
{
    char *nick;
    char *params = &compo->params[0];
    char *state = "";
    struct printtext_context ctx = {
	.window     = g_status_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    if (Strfeed(params, 2) != 2) {
	printtext(&ctx, "On issuing event %s: Strfeed(..., 2) != 2", compo->command);
	return;
    }

    (void) strtok_r(params, "\n", &state);

    if ((nick = strtok_r(NULL, "\n", &state)) == NULL) {
	printtext(&ctx, "On issuing event %s: An error occured", compo->command);
	return;
    }

    printtext(&ctx, "Nickname %c%s%c is already in use", BOLD, nick, BOLD);
    ctx.spec_type = TYPE_SPEC1;

    if (g_connection_in_progress) {
	if (g_alt_nick_tested) {
	    printtext(&ctx, "The alt_nick has already been tested. Disconnecting...");
	    event_welcome_signalit();
	    irc_unsuccessful_event_cleanup();
	} else if (!g_alt_nick_tested && *Config("alt_nick")) {
	    printtext(&ctx, "Testing alt_nick (%s) instead...", Config("alt_nick"));
	    net_send("NICK %s", Config("alt_nick"));
	    g_alt_nick_tested = true;
	} else {
	    printtext(&ctx, "Disconnecting...");
	    event_welcome_signalit();
	    irc_unsuccessful_event_cleanup();
	}
    }
}

/* event_channel_forward: 470 (undocumented)

   Example:
     :irc.server.com 470 <my nickname> <from channel> <to channel> :Forwarding to another channel

   Notes:
     The JOIN event is sent AFTER event 470 */
void
event_channel_forward(struct irc_message_compo *compo)
{
    char	*from_channel;
    char	*msg;
    char	*my_nick;
    char	*params = &compo->params[0];
    char	*state	= "";
    char	*to_channel;
    struct printtext_context ctx;

    if (Strfeed(params, 3) != 3) {
	goto bad;
    }

    my_nick	 = strtok_r(params, "\n", &state);
    from_channel = strtok_r(NULL, "\n", &state);
    to_channel	 = strtok_r(NULL, "\n", &state);
    msg		 = strtok_r(NULL, "\n", &state);

    if (my_nick == NULL || from_channel == NULL || to_channel == NULL || msg == NULL ||
	!Strings_match_ignore_case(my_nick, g_my_nickname)) {
	goto bad;
    }

    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1;
    ctx.include_ts = true;
    printtext(&ctx, "Channel forwarding from %c%s%c to %c%s%c",
	      BOLD, from_channel, BOLD, BOLD, to_channel, BOLD);
    return;

  bad:
    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_FAILURE;
    ctx.include_ts = true;
    printtext(&ctx, "On issuing event %s: An error occured", compo->command);
}
