/* ctcp.c
   Copyright (C) 2025 Markus Uhlin. All rights reserved.

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
#include "../irc.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "ctcp.h"
#include "i18n.h"
#include "misc.h"

static void
query_time(CSTRING p_target)
{
	if (net_send("PRIVMSG %s :\001TIME\001", p_target) > 0)
		confirm_ctcp_sent("TIME", p_target);
}

static void
query_userinfo(CSTRING p_target)
{
	if (net_send("PRIVMSG %s :\001USERINFO\001", p_target) > 0)
		confirm_ctcp_sent("USERINFO", p_target);
}

static void
query_version(CSTRING p_target)
{
	if (net_send("PRIVMSG %s :\001VERSION\001", p_target) > 0)
		confirm_ctcp_sent("VERSION", p_target);
}

/*
 * usage: /ctcp <query> <target>
 */
void
cmd_ctcp(CSTRING data)
{
	CSTRING			query, target;
	STRING			dcopy;
	STRING			last = "";
	static chararray_t	cmd = "/ctcp";
	static chararray_t	sep = "\n";

	if (strings_match(data, "")) {
		printtext_print("err", "%s", _("Insufficient arguments"));
		return;
	}

	dcopy = sw_strdup(data);
	(void) strFeed(dcopy, 1);

	if ((query = strtok_r(dcopy, sep, &last)) == NULL ||
	    (target = strtok_r(NULL, sep, &last)) == NULL) {
		printf_and_free(dcopy, _("%s: insufficient arguments"), cmd);
		return;
	} else if (!is_valid_nickname(target) && !is_irc_channel(target)) {
		printf_and_free(dcopy, _("%s: invalid target"), cmd);
		return;
	} else if (is_irc_channel(target) &&
		   strpbrk(target, g_forbidden_chan_name_chars) != NULL) {
		printf_and_free(dcopy,
		    _("%s: forbidden channel name characters"), cmd);
		return;
	}

	if (strings_match(query, "time"))
		query_time(target);
	else if (strings_match(query, "userinfo"))
		query_userinfo(target);
	else if (strings_match(query, "version"))
		query_version(target);
	else
		printtext_print("err", "%s", _("Invalid CTCP query"));
	free(dcopy);
}
