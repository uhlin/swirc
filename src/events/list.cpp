/* event 321 (RPL_LISTSTART) and 322 (RPL_LIST)
   Copyright (C) 2016-2025 Markus Uhlin. All rights reserved.

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

#include <string>

#include "../errHand.h"
#include "../irc.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "list.h"

/* event_liststart: 321 (according to the rfc: obsolete / not used)

   Example:
     :irc.server.com 321 <my nick> Channel :Users  Name */
void
event_liststart(struct irc_message_compo *compo)
{
	debug("Got event %s. (Not used)", compo->command);
}

/* event_list: 322

   Example:
     :irc.server.com 322 <my nick> <channel> <num visible> :<topic> */
void
event_list(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	 ctx;
	char			*channel, *num_visible, *topic;
	char			*state = const_cast<char *>("");
	std::string		 str("");

	if (strFeed(compo->params, 3) != 3)
		return;

	/*
	 * my nick (not used)
	 */
	(void) strtok_r(compo->params, "\n", &state);

	if ((channel = strtok_r(nullptr, "\n", &state)) == nullptr ||
	    (num_visible = strtok_r(nullptr, "\n", &state)) == nullptr ||
	    (topic = strtok_r(nullptr, "\n", &state)) == nullptr)
		return;

	if (*topic == ':')
		topic++;

#define B1	Theme("notice_inner_b1")
#define B2	Theme("notice_inner_b2")

	(void) str.append(COLOR1).append(channel).append(TXT_NORMAL);
	(void) str.append(B1).append(num_visible).append(B2);

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC3, true);
	printtext(&ctx, "%s: %s", str.c_str(), topic);
}
