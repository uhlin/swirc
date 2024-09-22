/* The wholeft command
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

#include "../dataClassify.h"
#include "../main.h"
#include "../netsplit.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"
#include "../window.h"

#include "wholeft.h"

static bool
netsplit_chk(CSTRING channel)
{
	const std::vector<netsplit *> &netsplit_db = netsplit_get_db();
	std::vector<netsplit *>::const_iterator it;

	for (it = netsplit_db.begin(); it != netsplit_db.end(); ++it) {
		immutable_cp_t db_chan = (*it)->channel.c_str();

		if (strings_match_ignore_case(channel, db_chan))
			return true;
	}

	return false;
}

static void
pr_wholeft(const netsplit *ns)
{
	PRINTTEXT_CONTEXT	ctx;
	STRING			list;

	if (ns == nullptr || ns->nicks.empty())
		return;

	list = sw_strdup("");

	for (const std::string &nick : ns->nicks) {
		std::string str(nick);

		str.insert(0, " ");
		realloc_strcat(&list, str.c_str());
	}

	const struct netsplit_context ns_ctx(ns->channel.c_str(),
	    ns->server[0].c_str(),
	    ns->server[1].c_str());

	printtext_context_init(&ctx, nullptr, TYPE_SPEC1, true);

	if ((ctx.window = window_by_label(ns_ctx.chan)) == nullptr)
		ctx.window = g_status_window;

	printtext(&ctx, "%s%s%s %s %s %s - the following users left: %s",
	    LEFT_BRKT, ns_ctx.chan, RIGHT_BRKT,
	    ns_ctx.serv1, THE_SPEC2, ns_ctx.serv2,
	    list);
	free(list);
}

static void
wholeft(CSTRING channel)
{
	const std::vector<netsplit *> &netsplit_db = netsplit_get_db();
	std::vector<netsplit *>::const_iterator it;

	for (it = netsplit_db.begin(); it != netsplit_db.end(); ++it) {
		immutable_cp_t db_chan = (*it)->channel.c_str();

		if (strings_match_ignore_case(channel, db_chan) &&
		    (*it)->has_announced_split() &&
		    !(*it)->join_begun())
			pr_wholeft(*it);
	}
}

/*
 * usage: /wholeft
 */
void
cmd_wholeft(CSTRING data)
{
	static chararray_t cmd = "/wholeft";

	if (!strings_match(data, "")) {
		printtext_print("err", "%s: implicit trailing data", cmd);
		return;
	} else if (!is_irc_channel(ACTWINLABEL)) {
		printtext_print("err", "%s: the active window isn't an irc "
		    "channel", cmd);
		return;
	} else if (netsplit_db_empty()) {
		printtext_print("err", "%s: there are no netsplits at the "
		    "moment", cmd);
		return;
	}

	if (!netsplit_chk(ACTWINLABEL)) {
		printtext_print("err", "%s: there are no netsplits for %s", cmd,
		    ACTWINLABEL);
		return;
	}

	wholeft(ACTWINLABEL);
}
