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

_Atomic(bool) g_invoked_by_znc_jump_net = false;

static stringarray_t znc_commands = {
	"AddMOTD ",
	"AddNetwork ",
	"AddPort ",
	"AddServer ",
	"AddTrustedServerFingerprint ",
	"Attach",
	"Broadcast",
	"ClearAllBuffers",
	"ClearAllChannelBuffers",
	"ClearAllQueryBuffers",
	"ClearBindHost",
	"ClearBuffer ",
	"ClearMOTD",
	"ClearUserBindHost",
	"Connect",
	"DelNetwork ",
	"DelPort",
	"DelServer ",
	"DelTrustedServerFingerprint ",
	"Detach",
	"DisableChan",
	"Disconnect",
	"EnableChan",
	"Jump",
	"JumpNetwork ",
	"ListAllUserNetworks",
	"ListAvailMods",
	"ListChans",
	"ListClients",
	"ListMods",
	"ListNetworks",
	"ListNicks",
	"ListPorts",
	"ListServers",
	"ListTrustedServerFingerprints",
	"ListUsers",
	"LoadMod ",
	"MoveNetwork",
	"PlayBuffer ",
	"Rehash",
	"ReloadMod",
	"Restart",
	"SaveConfig",
	"SetBindHost",
	"SetBuffer ",
	"SetMOTD ",
	"SetUserBindHost",
	"ShowBindHost",
	"ShowMOTD",
	"Shutdown",
	"Topics",
	"Traffic",
	"UnloadMod",
	"UpdateMod ",
	"Uptime",
	"Version",

	// In lowercase as well...
	"addmotd ",
	"addnetwork ",
	"addport ",
	"addserver ",
	"addtrustedserverfingerprint ",
	"attach",
	"broadcast",
	"clearallbuffers",
	"clearallchannelbuffers",
	"clearallquerybuffers",
	"clearbindhost",
	"clearbuffer ",
	"clearmotd",
	"clearuserbindhost",
	"connect",
	"delnetwork ",
	"delport",
	"delserver ",
	"deltrustedserverfingerprint ",
	"detach",
	"disablechan",
	"disconnect",
	"enablechan",
	"jump",
	"jumpnetwork ",
	"listallusernetworks",
	"listavailmods",
	"listchans",
	"listclients",
	"listmods",
	"listnetworks",
	"listnicks",
	"listports",
	"listservers",
	"listtrustedserverfingerprints",
	"listusers",
	"loadmod ",
	"movenetwork",
	"playbuffer ",
	"rehash",
	"reloadmod",
	"restart",
	"saveconfig",
	"setbindhost",
	"setbuffer ",
	"setmotd ",
	"setuserbindhost",
	"showbindhost",
	"showmotd",
	"shutdown",
	"topics",
	"traffic",
	"unloadmod",
	"updatemod ",
	"uptime",
	"version",
};

static void
send_cmd(CSTRING p_module, CSTRING p_command)
{
	char			v_module[80] = { '\0' };
	static chararray_t	array = "JumpNetwork ";
	static const size_t	size = sizeof array - 1;

	if (sw_strcpy(v_module, p_module, sizeof v_module) != 0)
		throw std::runtime_error("string copying error");
	else if (net_send("PRIVMSG %s :%s",
	    strToLower(v_module), p_command) < 0)
		throw std::runtime_error("cannot send");
	else if (!strncasecmp(p_command, array, size) &&
	    !strings_match(&p_command[size], "") &&
	    strings_match(v_module, "*status"))
		g_invoked_by_znc_jump_net = true;
	else
		return;
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
		std::string			 token;
		std::vector<std::string>	 tokens;

		while (std::getline(input, token))
			tokens.push_back(token);

		if (!written_linefeed) {
			if (tokens.size() != 1) {
				throw std::runtime_error("bogus number of "
				    "tokens (expected 1)");
			} else
				send_cmd("*status", tokens.at(0).c_str());
		} else if (tokens.size() != 2) {
			throw std::runtime_error("bogus number of tokens "
			    "(expected 2)");
		} else if (tokens.at(0).at(0) != '*') {
			throw std::runtime_error("bogus module name");
		} else {
			send_cmd(tokens.at(0).c_str(), tokens.at(1).c_str());
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
			textBuf_emplace_back(__func__, matches, cmd, 0);
	}

	if (textBuf_size(matches) == 0) {
		textBuf_destroy(matches);
		return NULL;
	}

	return matches;
}
