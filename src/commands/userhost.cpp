/* commands/userhost.cpp  --  the userhost command
   Copyright (C) 2026 Markus Uhlin. All rights reserved.

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

#include <cinttypes>
#include <cstdint>
#include <string>
#include <vector>

#include "../dataClassify.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "i18n.h"
#include "userhost.h"

static STRING
get_nicklist(const std::vector<std::string> &p_nicks)
{
	std::string list("");

	for (const std::string &nick : p_nicks)
		list.append(nick).append(" ");

	return trim(sw_strdup(list.c_str()));
}

static bool
validate_nicks(const std::vector<std::string> &p_nicks,
	       std::string &p_invalid_nick)
{
	for (const std::string &nick : p_nicks) {
		if (!is_valid_nickname(nick.c_str())) {
			p_invalid_nick.assign(nick);
			return false;
		}
	}

	return true;
}

/*
 * usage: /userhost <nick1[,nick2][,nick3][...]>
 */
void
cmd_userhost(CSTRING p_data) noexcept
{
	CSTRING				token;
	STRING				dcopy;
	STRING				nicklist;
	auto				last = const_cast<STRING>("");
	static chararray_t		cmd = "/userhost";
	static chararray_t		sep = " ,";
	static const uint32_t		NICKS_MIN = 1;
	static const uint32_t		NICKS_MAX = 5;
	std::string			invalid_nick("");
	std::vector<std::string>	nicks;

	if (strings_match(p_data, "")) {
		printtext_print("err", _("%s: too few arguments"), cmd);
		return;
	}

	dcopy = sw_strdup(p_data);

	for (STRING str = dcopy;
	    (token = strtok_r(str, sep, &last)) != nullptr;
	    str = nullptr) {
#if defined(__cplusplus) && __cplusplus >= 201103L
		nicks.emplace_back(token);
#else
		nicks.push_back(token);
#endif
	}

	if (nicks.size() < NICKS_MIN) {
		printf_and_free(dcopy, "%s: too few nicknames: min=%" PRIu32,
				cmd, NICKS_MIN);
		return;
	} else if (nicks.size() > NICKS_MAX) {
		printf_and_free(dcopy, "%s: too many nicknames: max=%" PRIu32,
				cmd, NICKS_MAX);
		return;
	} else if (!validate_nicks(nicks, invalid_nick)) {
		printf_and_free(dcopy, _("%s: invalid nickname: %s"), cmd,
		    invalid_nick.c_str());
		return;
	}

	nicklist = get_nicklist(nicks);

	if (net_send("USERHOST %s", nicklist) < 0)
		printtext_print("warn", _("%s: cannot send"), cmd);

	free(dcopy);
	free(nicklist);
}
