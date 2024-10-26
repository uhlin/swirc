/* The change hostname event (IRCv3)
   Copyright (C) 2024 Markus Uhlin. All rights reserved.

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
#include "../window.h"

#include "../events/names.h"

#include "chghost.h"
#include "i18n.h"

static void print_change(CSTRING, CSTRING, CSTRING) NONNULL;

static void
print_change(CSTRING nick, CSTRING user, CSTRING host)
{
	PIRC_WINDOW		window;
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, nullptr, TYPE_SPEC1_SPEC2, true);

	for (int i = 1; i <= g_ntotal_windows; i++) {
		if ((window = window_by_refnum(i)) != nullptr &&
		    is_irc_channel(window->label) &&
		    event_names_htbl_lookup(nick, window->label) != nullptr) {
			ctx.window = window;
			printtext(&ctx, _("%s%s%s has changed their hostname "
			    "to %s%s@%s%s"),
			    COLOR2, nick, TXT_NORMAL,
			    LEFT_BRKT, user, host, RIGHT_BRKT);
		}
	} // for
}

/* event_chghost: the change hostname event

   Example:
     :nick!~old_user@old_host.local CHGHOST ~new_user new_host.local */
void
event_chghost(struct irc_message_compo *compo)
{
	CSTRING		nick;
	CSTRING		old_host, new_host;
	CSTRING		old_user, new_user;
	STRING		last[2];

	last[0] = last[1] = const_cast<STRING>("");

	try {
		if (compo->prefix == nullptr)
			throw std::runtime_error("no prefix");
		else if ((nick = strtok_r(compo->prefix, "!@", &last[0])) ==
		    nullptr)
			throw std::runtime_error("no nick");
		old_user = strtok_r(nullptr, "!@", &last[0]);
		old_host = strtok_r(nullptr, "!@", &last[0]);
		if (old_user == nullptr ||
		    old_host == nullptr)
			throw std::runtime_error("no old user@host");
		(void) strFeed(compo->params, 1);
		new_user = strtok_r(compo->params, "\n", &last[1]);
		new_host = strtok_r(nullptr, "\n", &last[1]);
		if (new_user == nullptr ||
		    new_host == nullptr)
			throw std::runtime_error("no new user@host");
		print_change(nick, (*new_user == '~' ? &new_user[1] :
		    &new_user[0]), new_host);
		UNUSED_VAR(old_user);
		UNUSED_VAR(old_host);
	} catch (const std::runtime_error &e) {
		printtext_print("warn", "%s: %s", __func__, e.what());
	}
}
