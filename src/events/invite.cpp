/* invite.cpp
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
    char	*nick, *user, *host;
    char	*prefix = &compo->prefix[1];
    char	*state1, *state2;
    char	*target, *channel;

    state1 = state2 = "";

    if ((nick = strtok_r(prefix, "!@", &state1)) == NULL ||
	(user = strtok_r(NULL, "!@", &state1)) == NULL ||
	(host = strtok_r(NULL, "!@", &state1)) == NULL) {
	return;
    }

    if (strFeed(compo->params, 1) != 1)
	return;

    if ((target = strtok_r(compo->params, "\n", &state2)) == NULL ||
	(channel = strtok_r(NULL, "\n", &state2)) == NULL)
	return;

    if (*channel == ':')
	channel++;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

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
}
