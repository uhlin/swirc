/* command /msg
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
#include "../irc.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "msg.h"

/* usage: /msg <recipient> <message> */
void
cmd_msg(const char *data)
{
    char *dcopy = sw_strdup(data);
    char *recipient, *message;
    char *state = "";
    struct printtext_context ctx = {
	.window	    = g_active_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    if (Strings_match(dcopy, "")
	|| Strfeed(dcopy, 1) != 1
	|| (recipient = strtok_r(dcopy, "\n", &state)) == NULL
	|| (message = strtok_r(NULL, "\n", &state)) == NULL) {
	printtext(&ctx, "/msg: missing arguments");
	free(dcopy);
	return;
    } else if (!is_valid_nickname(recipient) && !is_irc_channel(recipient)) {
	printtext(&ctx, "/msg: neither a nickname or irc channel");
	free(dcopy);
	return;
    } else {
	if (window_by_label(recipient) == NULL && spawn_chat_window(recipient, "") != 0) {
	    printtext(&ctx, "/msg: fatal: cannot spawn chat window!");
	    free(dcopy);
	    return;
	} else {
	    ctx.window	  = window_by_label(recipient);
	    ctx.spec_type = TYPE_SPEC_NONE;
	    if (net_send("PRIVMSG %s :%s", recipient, message) < 0)
		g_on_air = false;
	    else
		printtext(&ctx, "%s%s%s%c%s %s",
			  Theme("nick_s1"),
			  COLOR1, g_my_nickname, NORMAL,
			  Theme("nick_s2"),
			  message);
	    free(dcopy);
	}
    }
}
