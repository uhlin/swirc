/* command /invite
   Copyright (C) 2016 Markus Uhlin. All rights reserved.

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
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "invite.h"

/* usage: /invite <targ_nick> <channel> */
void
cmd_invite(const char *data)
{
    char *dcopy = sw_strdup(data);
    char *targ_nick, *channel;
    char *state = "";

    if (Strings_match(dcopy, "")
	|| Strfeed(dcopy, 1) != 1
	|| (targ_nick = strtok_r(dcopy, "\n", &state)) == NULL
	|| (channel = strtok_r(NULL, "\n", &state)) == NULL) {
	PrintAndFree("/invite: missing arguments", dcopy);
	return;
    } else if (!is_valid_nickname(targ_nick) || !is_irc_channel(channel)) {
	PrintAndFree("/invite: bogus nickname or channel", dcopy);
	return;
    } else if (window_by_label(channel) == NULL) {
	PrintAndFree("/invite: not on that channel", dcopy);
	return;
    } else {
	struct printtext_context ctx = {
	    .window	= g_active_window,
	    .spec_type  = TYPE_SPEC1,
	    .include_ts = true,
	};

	if (net_send("INVITE %s %s", targ_nick, channel) > 0)
	    printtext(&ctx, "Inviting %s%s%c to %s%s%s%c%s",
		      COLOR1, targ_nick, NORMAL,
		      LEFT_BRKT,
		      COLOR2, channel, NORMAL,
		      RIGHT_BRKT);

	free(dcopy);
    }
}
