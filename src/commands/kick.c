/* command /kick
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

#include "../dataClassify.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "kick.h"

static void
PrintAndFree(const char *msg, char *cp)
{
    struct printtext_context ptext_ctx = {
	.window	    = g_active_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    printtext(&ptext_ctx, "%s", msg);
    if (cp) free(cp);
}

/* usage: /kick <nick1[,nick2][,nick3][...]> [reason] */
void
cmd_kick(const char *data)
{
    char *dcopy = sw_strdup(data);
    char *nicks, *reason;
    char *state = "";

    if (Strings_match(dcopy, "") || (nicks = strtok_r(dcopy, " ", &state)) == NULL) {
	PrintAndFree("/kick: missing arguments", dcopy);
	return;
    }

    const bool has_reason = (reason = strtok_r(NULL, " ", &state)) != NULL;

    if (!is_irc_channel(g_active_window->label)) {
	PrintAndFree("/kick: active window isn't an irc channel", dcopy);
	return;
    } else {
	if (net_send("KICK %s %s :%s", g_active_window->label, nicks, has_reason ? reason : "") < 0)
	    g_on_air = false;
	free(dcopy);
    }
}
