/* Handle event welcome (001)
   Copyright (C) 2014-2025 Markus Uhlin. All rights reserved.

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

#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../identd.hpp"
#include "../irc.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "../commands/znc.h"

#include "welcome.h"

static void
autojoin()
{
	std::vector<std::string>::iterator	it;

	for (it = g_join_list.begin(); it != g_join_list.end(); ++it) {
		char	*str;

		if (!is_irc_channel(it->c_str()))
			(void) it->insert(0, "#");

		str = sw_strdup(it->c_str());

		if (window_by_label(str) == nullptr)
			(void) net_send("JOIN %s", strToLower(str));

		free(str);
	}
}

void
event_welcome(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	if (g_received_welcome) {
		if (g_invoked_by_znc_jump_net) {
			irc_deinit();
			irc_init();
			g_invoked_by_znc_jump_net = false;
		} else {
			err_log(EPROTO, "event_welcome(%s): warning: "
			    "already received welcome", compo->command);
			return;
		}
	}

	try {
		CSTRING		msg = nullptr;
		CSTRING		nick = nullptr;
		CSTRING		srv_host = nullptr;
		STRING		state = const_cast<STRING>("");

		if (config_bool("identd", false))
			identd::stop();

		if (compo->prefix == nullptr)
			throw std::runtime_error("no prefix!");

		srv_host = addrof(compo->prefix[0]);

		if (*srv_host == ':')
			srv_host++;
		if (strings_match(srv_host, "")) {
			throw std::runtime_error("unable to set "
			    "server hostname");
		}

		irc_set_server_hostname(srv_host);

		if (strFeed(compo->params, 1) != 1)
			throw std::runtime_error("strFeed");

		if ((nick = strtok_r(compo->params, "\n", &state)) == nullptr)
			throw std::runtime_error("no nickname");
		else if ((msg = strtok_r(nullptr, "\n", &state)) == nullptr)
			throw std::runtime_error("no message");

		irc_set_my_nickname(nick);

		if (*msg == ':')
			msg++;

		printtext_context_init(&ctx, g_status_window, TYPE_SPEC1, true);
		printtext(&ctx, "%s", msg);

		event_welcome_signalit();
		g_received_welcome = true;

		if (!g_icb_mode)
			autojoin();
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_status_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ctx, "event_welcome(%s): fatal: %s", compo->command,
		    e.what());
		net_request_disconnect();
		event_welcome_signalit();
	}
}
