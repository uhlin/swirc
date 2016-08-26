/* Event 367 (RPL_BANLIST) and 368 (RPL_ENDOFBANLIST)
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

#include <time.h>

#include "../dataClassify.h"
#include "../errHand.h"
#include "../irc.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "banlist.h"

/* event_banlist: 367

   Examples:
     :irc.server.com 367 <recipient> <channel> <mask> irc.server.com <time>
     :irc.server.com 367 <recipient> <channel> <mask> <nick>!<user>@<host> <time>
     :irc.server.com 367 <recipient> <channel> <mask>

   Other events with the same semantics:
     - 346 (RPL_INVITELIST)
     - 348 (RPL_EXCEPTLIST) */
void
event_banlist(struct irc_message_compo *compo)
{
    int		 feeds_written	 = 0;
    char	*state1		 = "";
    char	*state2		 = "";
    char	*channel	 = NULL;
    char	*mask		 = NULL;
    char	*issuer		 = NULL;
    char	*seconds	 = NULL;
    char	*issuer_name	 = "";
    char	*issuer_userhost = "";
    struct printtext_context ctx = {
	.window	    = g_status_window,
	.spec_type  = TYPE_SPEC1,
	.include_ts = true,
    };

    if ((feeds_written = Strfeed(compo->params, 4)) == 4) {
	char buf[500] = { 0 };

	(void) strtok_r(compo->params, "\n", &state1); /* recipient */

	if ((channel = strtok_r(NULL, "\n", &state1)) == NULL
	    || (mask = strtok_r(NULL, "\n", &state1)) == NULL
	    || (issuer = strtok_r(NULL, "\n", &state1)) == NULL
	    || (seconds = strtok_r(NULL, "\n", &state1)) == NULL
	    || sw_strcpy(buf, issuer, sizeof buf) != 0
	    || !is_numeric(seconds))
	    return;

	if (window_by_label(channel))
	    ctx.window = window_by_label(channel);

	if ((issuer_name = strtok_r(buf, "!", &state2)) == NULL)
	    return;
	issuer_userhost = strtok_r(NULL, "!", &state2);

	const time_t date_of_issue = (time_t) strtol(seconds, NULL, 10);

	printtext(&ctx, "%s%s%s%c%s: %s%s%c issued by %s%s%c %s%s%s %s%s%s",
		  LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT,
		  Theme("color4"), mask, NORMAL,
		  COLOR2, issuer_name, NORMAL,
		  LEFT_BRKT, issuer_userhost ? issuer_userhost : "", RIGHT_BRKT,
		  LEFT_BRKT, trim(ctime(&date_of_issue)), RIGHT_BRKT);
    } else if (feeds_written == 2) {
	(void) strtok_r(compo->params, "\n", &state1); /* recipient */

	if ((channel = strtok_r(NULL, "\n", &state1)) == NULL
	    || (mask = strtok_r(NULL, "\n", &state1)) == NULL)
	    return;

	if (window_by_label(channel))
	    ctx.window = window_by_label(channel);

	printtext(&ctx, "%s%s%s%c%s: %s%s%c",
		  LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT,
		  Theme("color4"), mask, NORMAL);
    } else {
	err_log(0, "In event_banlist: an error occurred");
    }
}

/* event_eof_banlist: 368

   Examples:
     :irc.server.com 368 <recipient> <channel> :End of Channel Ban List

   Other events that uses this function:
     :irc.server.com 347 <recipient> <channel> :End of Channel Invite List
     :irc.server.com 349 <recipient> <channel> :End of Channel Exception List */
void
event_eof_banlist(struct irc_message_compo *compo)
{
    char	*state	 = "";
    char	*channel = NULL;
    char	*msg	 = NULL;
    struct printtext_context ctx = {
	.window	    = g_status_window,
	.spec_type  = TYPE_SPEC1,
	.include_ts = true,
    };

    if (Strfeed(compo->params, 2) != 2)
	return;

    (void) strtok_r(compo->params, "\n", &state); /* recipient */

    if ((channel = strtok_r(NULL, "\n", &state)) == NULL ||
	(msg = strtok_r(NULL, "\n", &state)) == NULL)
	return;

    if (window_by_label(channel))
	ctx.window = window_by_label(channel);

    if (*msg == ':')
	msg++;

    printtext(&ctx, "%s", msg);
}
