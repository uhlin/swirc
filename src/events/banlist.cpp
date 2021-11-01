/* Event 367 (RPL_BANLIST) and 368 (RPL_ENDOFBANLIST)
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
     :irc.server.com 367 <recipient> <channel> <mask> <nick>!<user>@<host>
                         <time>
     :irc.server.com 367 <recipient> <channel> <mask>

   Other events with the same semantics:
     - 346 (RPL_INVITELIST)
     - 348 (RPL_EXCEPTLIST) */
void
event_banlist(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;
    char *state1 = const_cast<char *>("");
    char *state2 = const_cast<char *>("");
    int feeds_written = 0;

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1, true);

    try {
	if ((feeds_written = strFeed(compo->params, 4)) == 4) {
	    char buf[500] = { 0 };

	    (void) strtok_r(compo->params, "\n", &state1); /* recipient */

	    char *channel = strtok_r(NULL, "\n", &state1);
	    char *mask    = strtok_r(NULL, "\n", &state1);
	    char *issuer  = strtok_r(NULL, "\n", &state1);
	    char *seconds = strtok_r(NULL, "\n", &state1);

	    if (channel == NULL)
		throw std::runtime_error("unable to get channel");
	    else if (mask == NULL)
		throw std::runtime_error("unable to get mask");
	    else if (issuer == NULL)
		throw std::runtime_error("unable to get issuer");
	    else if (seconds == NULL)
		throw std::runtime_error("unable to get seconds");
	    else if (sw_strcpy(buf, issuer, ARRAY_SIZE(buf)) != 0)
		throw std::runtime_error("cannot copy issuer");
	    else if (!is_numeric(seconds))
		throw std::runtime_error("seconds not a number");

	    if (window_by_label(channel))
		ctx.window = window_by_label(channel);

	    char *issuer_name     = strtok_r(buf, "!", &state2);
	    char *issuer_userhost = strtok_r(NULL, "!", &state2);

	    if (issuer_name == NULL)
		throw std::runtime_error("unable to get issuer name");

	    const time_t date_of_issue = (time_t) strtol(seconds, NULL, 10);
	    struct tm result = { 0 };

#define TM_STRUCT_MSG "unable to retrieve tm structure"
#if defined(UNIX)
	    if (localtime_r(&date_of_issue, &result) == NULL)
		throw std::runtime_error("localtime_r: " TM_STRUCT_MSG);
#elif defined(WIN32)
	    if (localtime_s(&result, &date_of_issue) != 0)
		throw std::runtime_error("localtime_s: " TM_STRUCT_MSG);
#endif

	    char tbuf[100] = { '\0' };

	    if (strftime(tbuf, ARRAY_SIZE(tbuf), "%c", &result) == 0)
		throw std::runtime_error("strftime: zero return");

	    printtext(&ctx, "%s%s%s%c%s: %s%s%c issued by %s%s%c %s%s%s %s%s%s",
		LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT,
		COLOR4, mask, NORMAL,
		COLOR2, issuer_name, NORMAL,
		LEFT_BRKT, issuer_userhost ? issuer_userhost : "", RIGHT_BRKT,
		LEFT_BRKT, trim(tbuf), RIGHT_BRKT);
	} else if (feeds_written == 2) {
	    (void) strtok_r(compo->params, "\n", &state1); /* recipient */
	    char *channel = strtok_r(NULL, "\n", &state1);
	    char *mask    = strtok_r(NULL, "\n", &state1);

	    if (channel == NULL)
		throw std::runtime_error("unable to get channel");
	    else if (mask == NULL)
		throw std::runtime_error("unable to get mask");

	    if (window_by_label(channel))
		ctx.window = window_by_label(channel);

	    printtext(&ctx, "%s%s%s%c%s: %s%s%c",
		LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT,
		COLOR4, mask, NORMAL);
	} else {
	    throw std::runtime_error("unexpected number of feeds written");
	}
    } catch (const std::runtime_error &e) {
	ctx.window    = g_status_window;
	ctx.spec_type = TYPE_SPEC1_FAILURE;
	printtext(&ctx, "event_banlist(%s): error: %s",
	    compo->command, e.what());
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
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1, true);

	try {
		char	*channel, *msg;
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 2) != 2)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state); /* recipient */

		if ((channel = strtok_r(NULL, "\n", &state)) == NULL ||
		    (msg = strtok_r(NULL, "\n", &state)) == NULL) {
			throw std::runtime_error("unable to retrieve event "
			    "components");
		}

		if ((ctx.window = window_by_label(channel)) == NULL)
			ctx.window = g_status_window;

		if (*msg == ':')
			msg++;
		if (*msg)
			printtext(&ctx, "%s", msg);
	} catch (const std::runtime_error& e) {
		ctx.spec_type = TYPE_SPEC1_FAILURE;

		printtext(&ctx, "event_eof_banlist(%s): error: %s",
		    compo->command, e.what());
	}
}
