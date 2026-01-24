/* commands/squery.cpp
   Copyright (C) 2020-2026 Markus Uhlin. All rights reserved.

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

#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "squery.h"

static stringarray_t squery_commands = {
	/* Alis v2.4 */
	"alis ",
	"alis admin",
	"alis die",
	"alis hash",
	"alis info",
	"alis list ",
	"alis help",
	"alis help ",
	"alis help admin",
	"alis help die",
	"alis help hash",
	"alis help info",
	"alis help list",
	"alis help status",
	"alis help version",

	/* Clis v1.0a2 */
	"clis ",
	"clis admin",
	"clis info",
	"clis list ",
	"clis list -min ",
	"clis list -max ",
	"clis list --topic ",
	"clis list --show ",
	"clis version",
	"clis help",
	"clis help ",
	"clis help admin",
	"clis help info",
	"clis help list",
	"clis help version",

	//lint -e786
	cs_cmds_macro(chanserv),
	ns_cmds_macro(nickserv),
	//lint +e786
};

/*
 * usage: /squery <servicename> <text>
 */
void
cmd_squery(CSTRING data)
{
	STRING			 dcopy = nullptr;
	static const char	 cmd[] = "/squery";

	if (strings_match(data, "")) {
		printtext_print("err", "%s: missing arguments", cmd);
		return;
	}

	try {
		std::string			token;
		std::vector<std::string>	tokens;

		dcopy = sw_strdup(data);
		(void) strFeed(dcopy, 1);
		std::istringstream input(dcopy);
		free(dcopy);
		dcopy = nullptr;

		while (std::getline(input, token)) {
#if defined(__cplusplus) && __cplusplus >= 201103L
			tokens.emplace_back(token);
#else
			tokens.push_back(token);
#endif
		}

		if (tokens.size() != 2)
			throw std::runtime_error("missing arguments");

		CSTRING servicename = tokens.at(0).c_str();
		CSTRING text = tokens.at(1).c_str();

		if (net_send("SQUERY %s :%s", servicename, text) < 0)
			throw std::runtime_error("cannot send");
	} catch (const std::runtime_error &e) {
		printtext_print("err", "%s: %s", cmd, e.what());
	} catch (...) {
		printtext_print("err", "%s: %s", cmd, "unknown exception");
	}

	free(dcopy);
}

PTEXTBUF
get_list_of_matching_squery_commands(CSTRING search_var)
{
	PTEXTBUF	matches = textBuf_new();
	const size_t	varlen = strlen(search_var);

	for (size_t i = 0; i < ARRAY_SIZE(squery_commands); i++) {
		CSTRING cmd = squery_commands[i];

		if (!strncmp(search_var, cmd, varlen))
			textBuf_emplace_back(__func__, matches, cmd, 0);
	}

	if (textBuf_size(matches) == 0) {
		textBuf_destroy(matches);
		return NULL;
	}

	return matches;
}
