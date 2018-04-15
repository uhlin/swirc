/* join and part commands
   Copyright (C) 2016, 2017 Markus Uhlin. All rights reserved.

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

#include "../config.h"
#include "../dataClassify.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "jp.h"

/* usage: /join <channel> [key] */
void
cmd_join(const char *data)
{
    char *channel, *key;
    char *dcopy = sw_strdup(data);
    char *state = "";

    if (strings_match(dcopy, "") ||
	(channel = strtok_r(dcopy, " ", &state)) == NULL) {
	print_and_free("/join: missing arguments", dcopy);
	return;
    }

    const bool has_channel_key = (key = strtok_r(NULL, " ", &state)) != NULL;

    if (strtok_r(NULL, " ", &state) != NULL) {
	print_and_free("/join: implicit trailing data", dcopy);
	return;
    } else if (!is_irc_channel(channel) || strpbrk(channel + 1, ",") != NULL) {
	print_and_free("/join: bogus irc channel", dcopy);
	return;
    } else if (has_channel_key && strchr(key, ',') != NULL) {
	print_and_free("/join: commas aren't allowed in a key", dcopy);
	return;
    } else {
	if (has_channel_key) {
	    if (net_send("JOIN %s %s", strToLower(channel), key) < 0)
		g_on_air = false;
	} else {
	    if (net_send("JOIN %s", strToLower(channel)) < 0)
		g_on_air = false;
	}
	free(dcopy);
    }
}

/* usage: /part [channel] [message] */
void
cmd_part(const char *data)
{
    char *channel, *message;
    char *dcopy = sw_strdup(data);
    char *state = "";

    if (strings_match(dcopy, "") ||
	(channel = strtok_r(dcopy, " ", &state)) == NULL) {
	if (is_irc_channel(g_active_window->label)) {
	    if (net_send("PART %s :%s", g_active_window->label,
			 Config("part_message")) < 0)
		g_on_air = false;
	} else {
	    print_and_free("/part: missing arguments", NULL);
	}
	free(dcopy);
	return;
    }

    const bool has_message = (message = strtok_r(NULL, " ", &state)) != NULL;

    if (strtok_r(NULL, " ", &state) != NULL) {
	print_and_free("/part: implicit trailing data", dcopy);
	return;
    } else if (!is_irc_channel(channel) || strpbrk(channel + 1, ",") != NULL) {
	print_and_free("/part: bogus irc channel", dcopy);
	return;
    } else {
	if (has_message) {
	    if (net_send("PART %s :%s", strToLower(channel), message) < 0)
		g_on_air = false;
	} else {
	    if (net_send("PART %s", strToLower(channel)) < 0)
		g_on_air = false;
	}
	free(dcopy);
    }
}
