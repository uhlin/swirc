/* wallops.cpp
   Copyright (C) 2018-2022 Markus Uhlin. All rights reserved.

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

#include "notice.h" /* get_notice() */
#include "wallops.h"

/* event_wallops

   Examples:
     :irc.server.com WALLOPS <message>
     :nick!user@host WALLOPS <message> */
void
event_wallops(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	try {
		char	*prefix, *message;

		if (g_server_hostname == NULL)
			throw std::runtime_error("no server hostname");
		if ((prefix = compo->prefix) == NULL)
			throw std::runtime_error("no prefix");
		if (*prefix == ':')
			prefix++;
		if (*(message = compo->params) == ':')
			message++;

		printtext_context_init(&ctx, g_active_window, TYPE_SPEC_NONE,
		    true);

		if (strings_match_ignore_case(prefix, g_server_hostname)) {
			printtext(&ctx, "%s!%s%c %s", COLOR3, "WALLOPS", NORMAL,
			    message);
			return;
		} else {
			char	*last = const_cast<char *>("");
			char	*nick, *user, *host;
			char	*str;

			if ((nick = strtok_r(prefix, "!@", &last)) == NULL ||
			    (user = strtok_r(NULL, "!@", &last)) == NULL ||
			    (host = strtok_r(NULL, "!@", &last)) == NULL) {
				throw std::runtime_error("no nick or "
				    "user@host");
			}

			/*
			 * NOTE: Current look is identical to notice
			 */
			str = get_notice(nick, user, host);
			printtext(&ctx, "%s %s", str, message);
			free(str);
		}
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN,
		    true);
		printtext(&ctx, "event_wallops: %s", e.what());
	}
}
