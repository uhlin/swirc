/* commands/znc.cpp
   Copyright (C) 2020-2025 Markus Uhlin. All rights reserved.

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

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../errHand.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "znc.h"

volatile bool g_invoked_by_znc_jump_net = false;

static stringarray_t znc_commands = {
	"Version",
	"ListMods",
	"ListAvailMods",
	"ListNicks",
	"ListServers",
	"AddNetwork",
	"DelNetwork",
	"ListNetworks",
	"MoveNetwork",
	"JumpNetwork",
	"AddServer",
	"DelServer",
	"AddTrustedServerFingerprint",
	"DelTrustedServerFingerprint",
	"ListTrustedServerFingerprints",
	"EnableChan",
	"DisableChan",
	"Attach",
	"Detach",
	"Topics",
	"PlayBuffer",
	"ClearBuffer",
	"ClearAllBuffers",
	"ClearAllChannelBuffers",
	"ClearAllQueryBuffers",
	"SetBuffer",
	"SetBindHost",
	"SetUserBindHost",
	"ClearBindHost",
	"ClearUserBindHost",
	"ShowBindHost",
	"Jump",
	"Disconnect",
	"Connect",
	"Uptime",
	"LoadMod",
	"UnloadMod",
	"ReloadMod",
	"UpdateMod",
	"ShowMOTD",
	"SetMOTD",
	"AddMOTD",
	"ClearMOTD",
	"ListPorts",
	"AddPort",
	"DelPort",
	"Rehash",
	"SaveConfig",
	"ListUsers",
	"ListAllUserNetworks",
	"ListChans",
	"ListClients",
	"Traffic",
	"Broadcast",
	"Shutdown",
	"Restart",
};

static void
add_cmd(PTEXTBUF matches, CSTRING str)
{
	if (textBuf_size(matches) != 0) {
		if ((errno = textBuf_ins_next(matches, textBuf_tail(matches),
		    str, -1)) != 0)
			err_sys("%s: textBuf_ins_next", __func__);
	} else {
		if ((errno = textBuf_ins_next(matches, NULL, str, -1)) != 0)
			err_sys("%s: textBuf_ins_next", __func__);
	}
}

/*
 * usage: /znc [*module] <command>
 */
void
cmd_znc(CSTRING data)
{
	STRING			 dcopy;
	bool			 written_linefeed = false;
	static chararray_t	 cmd = "/znc";

	if (strings_match(data, "")) {
		printtext_print("err", "%s: missing arguments", cmd);
		return;
	}

	if (*(dcopy = sw_strdup(data)) == '*')
		written_linefeed = (strFeed(dcopy, 1) == 1);
	std::istringstream input(dcopy);
	free(dcopy);

	try {
		STRING				 module;
		std::string			 token;
		std::vector<std::string>	 tokens;

		while (std::getline(input, token))
			tokens.push_back(token);

		if (!written_linefeed) {
			if (tokens.size() != 1) {
				throw std::runtime_error("bogus number of "
				    "tokens (expected 1)");
			} else if (net_send("PRIVMSG *status :%s",
			    tokens.at(0).c_str()) < 0) {
				throw std::runtime_error("cannot send");
			}
		} else if (tokens.size() != 2) {
			throw std::runtime_error("bogus number of tokens "
			    "(expected 2)");
		} else if (tokens.at(0).at(0) != '*') {
			throw std::runtime_error("bogus module name");
		} else {
			module = sw_strdup(tokens.at(0).c_str());
			(void) net_send("PRIVMSG %s :%s", strToLower(module),
			    tokens.at(1).c_str());
			free(module);
		}
	} catch (const std::runtime_error &e) {
		printtext_print("err", "%s: %s", cmd, e.what());
	} catch (...) {
		printtext_print("err", "%s: %s", cmd, "unknown exception");
	}
}

PTEXTBUF
get_list_of_matching_znc_commands(CSTRING search_var)
{
	PTEXTBUF	matches = textBuf_new();
	const size_t	varlen = strlen(search_var);

	for (size_t i = 0; i < ARRAY_SIZE(znc_commands); i++) {
		CSTRING cmd = znc_commands[i];

		if (!strncmp(search_var, cmd, varlen))
			add_cmd(matches, cmd);
	}

	if (textBuf_size(matches) == 0) {
		textBuf_destroy(matches);
		return NULL;
	}

	return matches;
}
