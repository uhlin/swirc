/* Command join and part
   Copyright (C) 2016-2023 Markus Uhlin. All rights reserved.

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

#include "../config.h"
#include "../dataClassify.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "jp.h"

/*
 * usage: /join <channel> [key]
 */
void
cmd_join(const char *data)
{
	char	*dcopy = sw_strdup(data);

	try {
		char	*channel;
		char	*key;
		char	*state = const_cast<char *>("");

		if (strings_match(dcopy, "") ||
		    (channel = strtok_r(dcopy, " ", &state)) == NULL)
			throw std::runtime_error("missing arguments");

		const bool has_channel_key =
		    (key = strtok_r(NULL, " ", &state)) != NULL;

		if (strtok_r(NULL, " ", &state) != NULL)
			throw std::runtime_error("implicit trailing data");
		else if (strpbrk(channel, g_forbidden_chan_name_chars) != NULL)
			throw std::runtime_error("bogus irc channel");
		else if (has_channel_key && strchr(key, ',') != NULL)
			throw std::runtime_error("commas aren't allowed in a key");

		(void) strToLower(channel);
		std::string str(channel);

		if (!is_irc_channel(str.c_str()))
			(void) str.insert(0, "#");

		if (has_channel_key) {
			if (net_send("JOIN %s %s", str.c_str(), key) < 0)
				throw std::runtime_error("cannot send");
		} else {
			if (net_send("JOIN %s", str.c_str()) < 0)
				throw std::runtime_error("cannot send");
		}
	} catch (const std::runtime_error &e) {
		PRINTTEXT_CONTEXT	ctx;

		printtext_context_init(&ctx, g_active_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ctx, "/join: %s", e.what());
	}

	free(dcopy);
}

/*
 * usage: /part [channel] [message]
 */
void
cmd_part(const char *data)
{
	char	*dcopy = sw_strdup(data);

	try {
		char	*channel;
		char	*message;
		char	*state = const_cast<char *>("");

		(void) strFeed(dcopy, 1);

		if (strings_match(dcopy, "") ||
		    (channel = strtok_r(dcopy, "\n", &state)) == NULL) {
			if (is_irc_channel(ACTWINLABEL)) {
				if (net_send("PART %s :%s", ACTWINLABEL,
				    Config("part_message")) < 0)
					throw std::runtime_error("cannot send");
			} else {
				throw std::runtime_error("missing arguments");
			}

			free(dcopy);
			return;
		}

		const bool has_message =
		    (message = strtok_r(NULL, "\n", &state)) != NULL;

		if (strtok_r(NULL, "\n", &state) != NULL) {
			throw std::runtime_error("implicit trailing data");
		} else if (!is_irc_channel(channel) ||
		    strpbrk(channel + 1, g_forbidden_chan_name_chars) != NULL) {
			throw std::runtime_error("bogus irc channel");
		} else if (has_message) {
			if (net_send("PART %s :%s", strToLower(channel),
			    message) < 0)
				throw std::runtime_error("cannot send");
		} else {
			if (net_send("PART %s", strToLower(channel)) < 0)
				throw std::runtime_error("cannot send");
		}
	} catch (const std::runtime_error &e) {
		PRINTTEXT_CONTEXT	ctx;

		printtext_context_init(&ctx, g_active_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ctx, "/part: %s", e.what());
	}

	free(dcopy);
}
