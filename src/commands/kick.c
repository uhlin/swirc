/* Command /kick + /kickban
   Copyright (C) 2016-2019 Markus Uhlin. All rights reserved.

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
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "kick.h"

#ifdef UNIT_TESTING
#undef  ACTWINLABEL
#define ACTWINLABEL "#channel"
#endif

/*
 * usage: /kick <nick1[,nick2][,nick3][...]> [reason]
 */
void
cmd_kick(const char *data)
{
    char *dcopy = sw_strdup(data);
    char *nicks, *reason;
    char *state = "";

    (void) strFeed(dcopy, 1);

    if (strings_match(dcopy, "") ||
	(nicks = strtok_r(dcopy, "\n", &state)) == NULL) {
	print_and_free("/kick: missing arguments", dcopy);
	return;
    }

    const bool has_reason = (reason = strtok_r(NULL, "\n", &state)) != NULL;

    if (!is_irc_channel(ACTWINLABEL)) {
	print_and_free("/kick: active window isn't an irc channel", dcopy);
	return;
    } else {
	if (net_send("KICK %s %s :%s",
	    ACTWINLABEL, nicks, has_reason ? reason : "") < 0)
	    g_on_air = false;
	free(dcopy);
    }
}

/*
 * usage: /kickban <nick> <mask> [reason]
 */
void
cmd_kickban(const char *data)
{
    char	*dcopy	= sw_strdup(data);
    char	*reason = NULL;
    char	*state	= "";

    if (strings_match(dcopy, "")) {
	print_and_free("/kickban: missing arguments", dcopy);
	return;
    }

    (void) strFeed(dcopy, 2);

    char *nick = strtok_r(dcopy, "\n", &state);
    char *mask = strtok_r(NULL, "\n", &state);

    const bool has_reason = (reason = strtok_r(NULL, "\n", &state)) != NULL;

    if (nick == NULL) {
	print_and_free("/kickban: no nickname", dcopy);
	return;
    } else if (!is_valid_nickname(nick)) {
	print_and_free("/kickban: invalid nickname", dcopy);
	return;
    } else if (mask == NULL) {
	print_and_free("/kickban: no mask", dcopy);
	return;
    } else if (!is_irc_channel(ACTWINLABEL)) {
	print_and_free("/kickban: active window isn't an irc channel", dcopy);
	return;
    }

    if (net_send("MODE %s +b %s", ACTWINLABEL, mask) < 0 ||
	net_send("KICK %s %s :%s", ACTWINLABEL, nick, has_reason?reason:"") < 0)
	g_on_air = false;

    free(dcopy);
}
