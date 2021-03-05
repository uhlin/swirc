/* events/invite.cpp
   Copyright (C) 2016-2021 Markus Uhlin. All rights reserved.

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
#include "../irc.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "invite.h"

/* event_inviting: 341 (RPL_INVITING)

   Example:
     :irc.server.com 341 <my nick> <targ_nick> <channel> */
void
event_inviting(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    (void) compo;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_SUCCESS, true);
    printtext(&ctx, "Invitation passed onto the end client");
}

/* event_invite

   Example:
     :<nick>!<user>@<host> INVITE <target> :<channel> */
void
event_invite(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

    try {
	char *prefix = compo->prefix;

	if (prefix == NULL)
	    throw std::runtime_error("no prefix");
	else if (*prefix == ':')
	    prefix++;

	char *state1 = const_cast<char *>("");
	char *nick = strtok_r(prefix, "!@", &state1);
	char *user = strtok_r(NULL, "!@", &state1);
	char *host = strtok_r(NULL, "!@", &state1);

	if (nick == NULL)
	    throw std::runtime_error("unable to get nick");
	else if (user == NULL || host == NULL) {
	    user = const_cast<char *>("<no user>");
	    host = const_cast<char *>("<no host>");
	}

	if (strFeed(compo->params, 1) != 1)
	    throw std::runtime_error("strFeed");

	char *state2 = const_cast<char *>("");
	char *target = strtok_r(compo->params, "\n", &state2);
	char *channel = strtok_r(NULL, "\n", &state2);

	if (target == NULL)
	    throw std::runtime_error("null target");
	else if (channel == NULL)
	    throw std::runtime_error("null channel");
	else if (*channel == ':')
	    channel++;

	if (!is_irc_channel(channel))
	    throw std::runtime_error("bogus irc channel");

	if (strings_match_ignore_case(target, g_my_nickname)) {
	    printtext(&ctx, "%c%s%c %s%s@%s%s invites you to %c%s%c",
		      BOLD, nick, BOLD, LEFT_BRKT, user, host, RIGHT_BRKT,
		      BOLD, channel, BOLD);
	} else {
	    /*
	     * TODO: Write to the channels where the user that's doing
	     * the invite is in
	     */
	    printtext(&ctx, "%c%s%c %s%s@%s%s invites %c%s%c to %c%s%c",
		      BOLD, nick, BOLD, LEFT_BRKT, user, host, RIGHT_BRKT,
		      BOLD, target, BOLD, BOLD, channel, BOLD);

	}
    } catch (const std::runtime_error &e) {
	ctx.spec_type = TYPE_SPEC1_WARN;
	printtext(&ctx, "event_invite: error: %s", e.what());
    }
}
