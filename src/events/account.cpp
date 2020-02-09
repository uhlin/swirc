/* events/account.cpp
   Copyright (C) 2018-2020 Markus Uhlin. All rights reserved.

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

#include "account.h"
#include "names.h"

/* event_account

   Examples:
     :nick!user@host ACCOUNT <accountname> (logged into a new account)
     :nick!user@host ACCOUNT *             (logged out of their account) */
void
event_account(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    try {
	char *accountname = & (compo->params[0]);
	char *last = const_cast<char *>("");
	char *prefix = NULL;

	if (compo->prefix == NULL)
	    throw std::runtime_error("no prefix");
	else
	    prefix = & (compo->prefix[1]);

	char *nick = strtok_r(prefix, "!@", &last);
	char *user = strtok_r(NULL, "!@", &last);
	char *host = strtok_r(NULL, "!@", &last);

	if (nick == NULL || user == NULL || host == NULL)
	    throw std::runtime_error("no nick or user@host");

	if (*accountname == ':')
	    accountname++;

	const bool logged_out = strings_match(accountname, "*");
	printtext_context_init(&ctx, NULL, TYPE_SPEC1_SPEC2, true);

	for (int i = 1; i <= g_ntotal_windows; i++) {
	    PIRC_WINDOW window = window_by_refnum(i);

	    if (window && is_irc_channel(window->label) &&
		event_names_htbl_lookup(nick, window->label) != NULL) {
		ctx.window = window;

		if (logged_out) {
		    printtext(&ctx, "%s%s%c %s%s@%s%s has logged out "
			      "of their account",
			      COLOR2, nick, NORMAL,
			      LEFT_BRKT, user, host, RIGHT_BRKT);
		} else {
		    /*
		     * Logged in...
		     */
		    printtext(&ctx, "%s%s%c %s%s@%s%s has logged into "
			      "a new account %s%s%c",
			      COLOR1, nick, NORMAL,
			      LEFT_BRKT, user, host, RIGHT_BRKT,
			      COLOR4, accountname, NORMAL);
		}
	    }
	} /* for */
    } catch (std::runtime_error &e) {
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "event_account: %s", e.what());
    }
}
