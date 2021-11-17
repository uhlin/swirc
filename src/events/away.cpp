/* events/away.cpp
   Copyright (C) 2018-2021 Markus Uhlin. All rights reserved.

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

#include "../strHand.h"
#include "../dataClassify.h"
#include "../irc.h"
#include "../printtext.h"
#include "../theme.h"

#include "away.h"
#include "names.h"

/* event_away

   Example:
     :nick!user@host AWAY [message] */
void
event_away(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *last = const_cast<char *>("");

	if (compo == NULL)
	    throw std::runtime_error("no components");
	if (compo->prefix == NULL)
	    throw std::runtime_error("null prefix");

	char	*prefix	 = & (compo->prefix[1]);
	char	*message = compo->params;

	if (message) {
	    if (*message == ':')
		message++;
	    squeeze_text_deco(message);
	}

	char *nick = strtok_r(prefix, "!@", &last);
	char *user = strtok_r(NULL, "!@", &last);
	char *host = strtok_r(NULL, "!@", &last);

	if (nick == NULL || user == NULL || host == NULL)
	    throw std::runtime_error("no nick or user@host");

	printtext_context_init(&ctx, NULL, TYPE_SPEC1_SPEC2, true);

	for (int i = 1; i <= g_ntotal_windows; i++) {
	    PIRC_WINDOW window = window_by_refnum(i);

	    if (window && is_irc_channel(window->label) &&
		event_names_htbl_lookup(nick, window->label) != NULL) {
		ctx.window = window;

		if (message) {
		    printtext(&ctx, "%s%s%c %s%s@%s%s has been marked "
			"as being away (%s)",
			COLOR2, nick, NORMAL,
			LEFT_BRKT, user, host, RIGHT_BRKT,
			message);
		} else {
		    printtext(&ctx, "%s%s%c %s%s@%s%s is no longer marked "
			"as being away!",
			COLOR1, nick, NORMAL,
			LEFT_BRKT, user, host, RIGHT_BRKT);
		}
	    }
	} /* for */
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "event_away: %s", e.what());
    }
}

/*
 * 305 RPL_UNAWAY
 */
void
event_unaway(struct irc_message_compo *compo)
{
    irc_extract_msg(compo, g_active_window, 1, false);
    //g_is_away = false;
}

/*
 * 306 RPL_NOWAWAY
 */
void
event_nowAway(struct irc_message_compo *compo)
{
	irc_extract_msg(compo, g_active_window, 1, false);
}
