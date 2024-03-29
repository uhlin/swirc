/* events/account.cpp
   Copyright (C) 2018-2023 Markus Uhlin. All rights reserved.

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
#include "i18n.h"
#include "names.h"

static void	print_logged_in(PPRINTTEXT_CONTEXT, const char *, const char *,
		    const char *, const char *) NONNULL;
static void	print_logged_out(PPRINTTEXT_CONTEXT, const char *, const char *,
		    const char *) NONNULL;

static void
print_logged_in(PPRINTTEXT_CONTEXT ctx,
    const char *nick,
    const char *user,
    const char *host,
    const char *accountname)
{
	printtext(ctx, _("%s%s%c %s%s@%s%s has "
	    "logged into a new account %s%s%c"),
	    COLOR1, nick, NORMAL,
	    LEFT_BRKT, user, host, RIGHT_BRKT,
	    COLOR4, accountname, NORMAL);
}

static void
print_logged_out(PPRINTTEXT_CONTEXT ctx,
    const char *nick,
    const char *user,
    const char *host)
{
	printtext(ctx, _("%s%s%c %s%s@%s%s has "
	    "logged out of their account"),
	    COLOR2, nick, NORMAL,
	    LEFT_BRKT, user, host, RIGHT_BRKT);
}

/* event_account

   Examples:
     :nick!user@host ACCOUNT <accountname> (logged into a new account)
     :nick!user@host ACCOUNT *             (logged out of their account) */
void
event_account(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	try {
		char		*last = const_cast<char *>("");
		char		*prefix;
		const char	*accountname;
		const char	*nick, *user, *host;

		if ((prefix = compo->prefix) == NULL)
			throw std::runtime_error("no prefix");
		else
			prefix++;

		if ((nick = strtok_r(prefix, "!@", &last)) == NULL ||
		    (user = strtok_r(NULL, "!@", &last)) == NULL ||
		    (host = strtok_r(NULL, "!@", &last)) == NULL)
			throw std::runtime_error("no nick or user@host");
		else if (*(accountname = compo->params) == ':')
			accountname++;

		const bool logged_out = strings_match(accountname, "*");

		printtext_context_init(&ctx, NULL, TYPE_SPEC1_SPEC2, true);

		for (int i = 1; i <= g_ntotal_windows; i++) {
			PIRC_WINDOW	window;

			if ((window = window_by_refnum(i)) != NULL &&
			    is_irc_channel(window->label) &&
			    event_names_htbl_lookup(nick, window->label) !=
			    NULL) {
				ctx.window = window;

				if (logged_out) {
					print_logged_out(&ctx, nick, user,
					    host);
				} else {
					/*
					 * Logged in...
					 */

					print_logged_in(&ctx, nick, user, host,
					    accountname);
				}
			}
		} /* for */
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_status_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ctx, "%s: %s", __func__, e.what());
	}
}
