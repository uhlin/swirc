/* command /notice
   Copyright (C) 2016-2021 Markus Uhlin. All rights reserved.

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

#include <string>

#include "../dataClassify.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "notice.h"

#define B1 Theme("notice_inner_b1")
#define B2 Theme("notice_inner_b2")

/*
 * usage: /notice <recipient> <message>
 */
void
cmd_notice(const char *data)
{
	char	*dcopy = sw_strdup(data);
	char	*recipient, *message;
	char	*state = const_cast<char *>("");

	if (strings_match(dcopy, "") || strFeed(dcopy, 1) != 1 ||
	    (recipient = strtok_r(dcopy, "\n", &state)) == NULL ||
	    (message = strtok_r(NULL, "\n", &state)) == NULL) {
		print_and_free("/notice: missing arguments", dcopy);
		return;
	} else if (!is_valid_nickname(recipient) &&
	    !is_irc_channel(recipient)) {
		print_and_free("/notice: neither a nickname or irc channel",
		    dcopy);
		return;
	} else if (window_by_label(recipient) == NULL &&
	    is_irc_channel(recipient)) {
		print_and_free("/notice: not on that channel", dcopy);
		return;
	} else if (net_send("NOTICE %s :%s", recipient, message) > 0) {
		PRINTTEXT_CONTEXT	ctx;
		std::string		str(LEFT_BRKT);

		(void) str.append(COLOR1).append("notice").append(TXT_NORMAL);
		(void) str.append(B1);
		(void) str.append(COLOR2).append(recipient).append(TXT_NORMAL);
		(void) str.append(B2);
		(void) str.append(RIGHT_BRKT);

		printtext_context_init(&ctx, g_active_window, TYPE_SPEC_NONE,
		    true);
		printtext(&ctx, "%s %s", str.c_str(), message);
	}

	free(dcopy);
}
