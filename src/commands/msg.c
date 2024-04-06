/* Message command
   Copyright (C) 2016-2024 Markus Uhlin. All rights reserved.

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
#include "../io-loop.h"
#include "../printtext.h"
#include "../strHand.h"

#include "msg.h"

/*
 * usage: /msg <recipient> <message>
 */
void
cmd_msg(const char *data)
{
	char		*dcopy = sw_strdup(data);
	char		*state = "";
	const char	*recipient, *message;

	if (strings_match(dcopy, "") || strFeed(dcopy, 1) != 1 ||
	    (recipient = strtok_r(dcopy, "\n", &state)) == NULL ||
	    (message = strtok_r(NULL, "\n", &state)) == NULL) {
		print_and_free("/msg: missing arguments", dcopy);
		return;
	} else if (!is_valid_nickname(recipient) &&
	    !is_irc_channel(recipient)) {
		print_and_free("/msg: neither a nickname or irc channel",
		    dcopy);
		return;
	} else if (strings_match_ignore_case(recipient, "ChanServ")) {
		print_and_free("/msg: for safety reasons: "
		    "consider using command /chanserv", dcopy);
		return;
	} else if (strings_match_ignore_case(recipient, "NickServ")) {
		print_and_free("/msg: for safety reasons: "
		    "consider using command /nickserv", dcopy);
		return;
	} else if (window_by_label(recipient) == NULL &&
	    is_valid_nickname(recipient)) {
		if (spawn_chat_window(recipient, recipient) != 0) {
			print_and_free("/msg: fatal: cannot spawn chat window!",
			    dcopy);
			return;
		}
		transmit_user_input(recipient, message);
		free(dcopy);
	} else if (window_by_label(recipient) == NULL &&
	    is_irc_channel(recipient)) {
		print_and_free("/msg: not on that channel", dcopy);
		return;
	} else {
		transmit_user_input(recipient, message);
		free(dcopy);
	}
}
