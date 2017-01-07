/* commands/services.c
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

#include "services.h"

/* usage: /chanserv <service hostname | --> <command> [...] */
void
cmd_chanserv(const char *data)
{
    char *dcopy = sw_strdup(data);
    char *srv_host = NULL, *cmd = NULL;
    char *state = "";

    if (Strings_match(dcopy, "")
	|| Strfeed(dcopy, 1) != 1
	|| (srv_host = strtok_r(dcopy, "\n", &state)) == NULL
	|| (cmd = strtok_r(NULL, "\n", &state)) == NULL) {
	print_and_free("/chanserv: missing arguments", dcopy);
	return;
    }

    if (Strings_match(srv_host, "--")) {
	if (!is_valid_hostname( Config("chanserv_host") )) {
	    print_and_free("/chanserv: in the config file: "
		"bogus chanserv_host", dcopy);
	    return;
	}

	if (net_send("PRIVMSG ChanServ@%s :%s",
		     Config("chanserv_host"), cmd) < 0)
	    g_on_air = false;
    } else {
	if (!is_valid_hostname(srv_host)) {
	    print_and_free("/chanserv: bogus service hostname!", dcopy);
	    return;
	}

	if (net_send("PRIVMSG ChanServ@%s :%s", srv_host, cmd) < 0)
	    g_on_air = false;
    }

    free(dcopy);
}

/* usage: /nickserv <service hostname | --> <command> [...] */
void
cmd_nickserv(const char *data)
{
    char *dcopy = sw_strdup(data);
    char *srv_host = NULL, *cmd = NULL;
    char *state = "";

    if (Strings_match(dcopy, "")
	|| Strfeed(dcopy, 1) != 1
	|| (srv_host = strtok_r(dcopy, "\n", &state)) == NULL
	|| (cmd = strtok_r(NULL, "\n", &state)) == NULL) {
	print_and_free("/nickserv: missing arguments", dcopy);
	return;
    }

    if (Strings_match(srv_host, "--")) {
	if (!is_valid_hostname( Config("nickserv_host") )) {
	    print_and_free("/nickserv: in the config file: "
		"bogus nickserv_host", dcopy);
	    return;
	}

	if (net_send("PRIVMSG NickServ@%s :%s",
		     Config("nickserv_host"), cmd) < 0)
	    g_on_air = false;
    } else {
	if (!is_valid_hostname(srv_host)) {
	    print_and_free("/nickserv: bogus service hostname!", dcopy);
	    return;
	}

	if (net_send("PRIVMSG NickServ@%s :%s", srv_host, cmd) < 0)
	    g_on_air = false;
    }

    free(dcopy);
}
