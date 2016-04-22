/* join and part commands
   Copyright (C) 2016 Markus Uhlin. All rights reserved.

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

   THIS SOFTWARE IS PROVIDED THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS
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

static struct printtext_context ptext_ctx = {
    .window	= NULL,
    .spec_type	= TYPE_SPEC1_FAILURE,
    .include_ts = true,
};

/* usage: /join <channel> [key] */
void
cmd_join(const char *data)
{
    char *channel, *key;
    char *dcopy = sw_strdup(data);
    char *state = "";

    ptext_ctx.window = g_active_window;

    if (Strings_match(dcopy, "") || (channel = strtok_r(dcopy, " ", &state)) == NULL) {
	printtext(&ptext_ctx, "/join: missing arguments");
	free(dcopy);
	return;
    }

    const bool has_key = (key = strtok_r(NULL, " ", &state)) != NULL;

    if (strtok_r(NULL, " ", &state) != NULL) {
	printtext(&ptext_ctx, "/join: implicit trailing data");
	free(dcopy);
	return;
    } else if (!is_irc_channel(channel) || strpbrk(channel + 1, ",&#+!") != NULL) {
	printtext(&ptext_ctx, "/join: bogus irc channel");
	free(dcopy);
	return;
    } else if (has_key && strchr(key, ',') != NULL) {
	printtext(&ptext_ctx, "/join: commas aren't allowed in a key");
	free(dcopy);
	return;
    } else {
	if (has_key)
	    net_send(g_socket, 0, "JOIN %s %s", str_tolower(channel), key);
	else
	    net_send(g_socket, 0, "JOIN %s", str_tolower(channel));
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

    ptext_ctx.window = g_active_window;

    if (Strings_match(dcopy, "") || (channel = strtok_r(dcopy, " ", &state)) == NULL) {
	if (is_irc_channel(g_active_window->label))
	    net_send(g_socket, 0, "PART %s :%s", g_active_window->label, Config("part_message"));
	else
	    printtext(&ptext_ctx, "/part: missing arguments");
	free(dcopy);
	return;
    }

    const bool has_message = (message = strtok_r(NULL, " ", &state)) != NULL;

    if (strtok_r(NULL, " ", &state) != NULL) {
	printtext(&ptext_ctx, "/part: implicit trailing data");
	free(dcopy);
	return;
    } else if (!is_irc_channel(channel) || strpbrk(channel + 1, ",&#+!") != NULL) {
	printtext(&ptext_ctx, "/part: bogus irc channel");
	free(dcopy);
	return;
    } else {
	if (has_message)
	    net_send(g_socket, 0, "PART %s :%s", str_tolower(channel), message);
	else
	    net_send(g_socket, 0, "PART %s", str_tolower(channel));
	free(dcopy);
    }
}
