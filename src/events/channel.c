/* Channel related events
   Copyright (C) 2015, 2016 Markus Uhlin. All rights reserved.

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

#include "../dataClassify.h"
#include "../errHand.h"
#include "../irc.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "channel.h"
#include "names.h"

/* event_topic: 332

   Example:
     :irc.server.com 332 <recipient> <channel> :This is the topic. */
void
event_topic(struct irc_message_compo *compo)
{
    (void) compo;
}

/* event_topic_creator: 333

   Examples:
     :irc.server.com 333 <recipient> <channel> <nick>!<user>@<host> <time>
     :irc.server.com 333 <recipient> <channel> <nick> <time> */
void
event_topic_creator(struct irc_message_compo *compo)
{
    (void) compo;
}

/* event_mode

   Examples:
     :<nick>!<user>@<host> MODE <channel> +v <nick>
     :<my nick> MODE <my nick> :+i */
void
event_mode(struct irc_message_compo *compo)
{
    (void) compo;
}

/* event_join

   Example:
     :<nick>!<user>@<host> JOIN <channel> */
void
event_join(struct irc_message_compo *compo)
{
    char	*prefix	 = &compo->prefix[1];
    char	*channel =
	*(compo->params) == ':' ? &compo->params[1] : &compo->params[0];
    char	*state	 = "";
    char	*nick;
    char	*user;
    char	*host;
    struct printtext_context ctx;

    nick = strtok_r(prefix, "!@", &state);
    user = strtok_r(NULL, "!@", &state);
    host = strtok_r(NULL, "!@", &state);

    if (nick == NULL || user == NULL || host == NULL) {
	goto bad;
    }

    if (Strings_match_ignore_case(nick, g_my_nickname)) {
	if (spawn_chat_window(channel, "No title.") != 0) {
	    goto bad;
	}
    } else {
	if (event_names_htbl_insert(nick, channel) != OK) {
	    goto bad;
	}
    }

    if ((ctx.window = window_by_label(channel)) == NULL) {
	goto bad;
    }

    ctx.spec_type  = TYPE_SPEC1_SPEC2;
    ctx.include_ts = true;

    printtext(&ctx, "%s%s%c %s%s@%s%s has joined %s%s%c",
	      COLOR1, nick, NORMAL, LEFT_BRKT, user, host, RIGHT_BRKT,
	      COLOR2, channel, NORMAL);
    return;

  bad:
    err_msg("On issuing event %s: A fatal error occured", compo->command);
    abort();
}

/* event_part

   Examples:
     :<nick>!<user>@<host> PART <channel>
     :<nick>!<user>@<host> PART <channel> :<message> */
void
event_part(struct irc_message_compo *compo)
{
    char *channel;
    char *host;
    char *message;
    char *nick;
    char *prefix = &compo->prefix[1];
    char *state1, *state2;
    char *user;
    const bool has_message = Strfeed(compo->params, 1) == 1;
    struct printtext_context ctx;

    state1 = state2 = "";

    if ((nick = strtok_r(prefix, "!@", &state1)) == NULL
	|| (user = strtok_r(NULL, "!@", &state1)) == NULL
	|| (host = strtok_r(NULL, "!@", &state1)) == NULL) {
	return;
    }

    if ((channel = strtok_r(compo->params, "\n", &state2)) == NULL)
	goto bad;
    message = strtok_r(NULL, "\n", &state2);

    if (Strings_match_ignore_case(nick, g_my_nickname)) {
	if (destroy_chat_window(channel) != 0)
	    goto bad;
	else
	    return;
    } else {
	if (event_names_htbl_remove(nick, channel) != OK)
	    goto bad;
    }

    if ((ctx.window = window_by_label(channel)) == NULL) {
	goto bad;
    }

    ctx.spec_type  = TYPE_SPEC1_SPEC2;
    ctx.include_ts = true;

    if (!has_message)
	message = "";
    if (has_message && *message == ':')
	message++;
    printtext(&ctx, "%s%s%c %s%s@%s%s has left %s%s%c %s%s%s",
	      COLOR2, nick, NORMAL, LEFT_BRKT, user, host, RIGHT_BRKT,
	      COLOR2, channel, NORMAL,
	      LEFT_BRKT, message, RIGHT_BRKT);
    return;

  bad:
    err_msg("On issuing event %s: A fatal error occured", compo->command);
    abort();
}

/* event_quit

   Example:
     :<nick>!<user>@<host> QUIT :<message> */
void
event_quit(struct irc_message_compo *compo)
{
    char	*host;
    char	*message = *(compo->params) == ':' ? &compo->params[1] : &compo->params[0];
    char	*nick;
    char	*prefix	 = &compo->prefix[1];
    char	*state	 = "";
    char	*user;
    struct printtext_context ctx = {
	.window	    = NULL,
	.spec_type  = TYPE_SPEC1_SPEC2,
	.include_ts = true,
    };

    if ((nick = strtok_r(prefix, "!@", &state)) == NULL
	|| (user = strtok_r(NULL, "!@", &state)) == NULL
	|| (host = strtok_r(NULL, "!@", &state)) == NULL) {
	return;
    }

    for (int i = 1; i <= g_ntotal_windows; i++) {
	PIRC_WINDOW window = window_by_refnum(i);

	if (window && is_irc_channel(window->label) &&
	    event_names_htbl_remove(nick, window->label) == OK) {
	    ctx.window = window;
	    printtext(&ctx, "%s %s%s@%s%s has quit %s%s%s",
		      nick, LEFT_BRKT, user, host, RIGHT_BRKT,
		      LEFT_BRKT, message, RIGHT_BRKT);
	}
    }
}
