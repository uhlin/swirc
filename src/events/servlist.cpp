/* events/servlist.cpp
   Copyright (C) 2020-2021 Markus Uhlin. All rights reserved.

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
#include <string>

#include "../irc.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "servlist.h"

/* event_servlist: 234 (RPL_SERVLIST)

   Examples:
     :irc.server.com 234 <me> <name> <server> <mask> <type> <hopcount> <info>
     :irc.server.com 234 <me> Alis@hub.uk hub.uk * 0xD000 1 :[...]
     :irc.server.com 234 <me> Clis@hub.uk hub.uk * 0xD000 1 :[...] */
void
event_servlist(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		char		*name, *server, *mask, *type, *hopcount, *info;
		char		*state = const_cast<char *>("");
		std::string	 str("");

		if (strFeed(compo->params, 6) != 6)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state); /* me */

		if ((name = strtok_r(NULL, "\n", &state)) == NULL ||
		    (server = strtok_r(NULL, "\n", &state)) == NULL ||
		    (mask = strtok_r(NULL, "\n", &state)) == NULL ||
		    (type = strtok_r(NULL, "\n", &state)) == NULL ||
		    (hopcount = strtok_r(NULL, "\n", &state)) == NULL ||
		    (info = strtok_r(NULL, "\n", &state)) == NULL) {
			throw std::runtime_error("unable to retrieve event "
			    "components");
		}

		/* for future use */
		(void) server;
		(void) type;
		(void) hopcount;

		if (*info == ':')
			info++;

#define B1	Theme("notice_inner_b1")
#define B2	Theme("notice_inner_b2")

		(void) str.append(COLOR1).append(name).append(TXT_NORMAL);
		(void) str.append(B1).append(mask).append(B2);

		printtext(&ctx, "%s: %s", str.c_str(), info);
	} catch (const std::runtime_error& e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_FAILURE;

		printtext(&ctx, "event_servlist(%s): error: %s", compo->command,
		    e.what());
	}
}

/* event_servlistEnd: 235 (RPL_SERVLISTEND)

   Example:
     :irc.server.com 235 <me> <mask> <type> :End of service listing */
void
event_servlistEnd(struct irc_message_compo *compo)
{
	irc_extract_msg(compo, g_active_window, 3, false);
}
