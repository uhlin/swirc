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
