/* Handle event welcome (001)
   Copyright (C) 2014-2018 Markus Uhlin. All rights reserved.

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

#include "../irc.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "welcome.h"

void
event_welcome(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;
    char *msg;
    char *nick;
    char *srv_host = &compo->prefix[0];
    char *state = "";

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_FAILURE, true);

    if (*srv_host == ':') {
	srv_host++;
    }

    if (strings_match(srv_host, "")) {
	printtext(&ctx, "Couldn't update the server hostname (empty prefix)");
	goto bad;
    } else {
	irc_set_server_hostname(srv_host);
    }

    if (strFeed(compo->params, 1) != 1) {
	printtext(&ctx, "In event_welcome: strFeed(..., 1) != 1");
	goto bad;
    }

    if ((nick = strtok_r(compo->params, "\n", &state)) == NULL ||
	(msg = strtok_r(NULL, "\n", &state)) == NULL) {
	printtext(&ctx, "In event_welcome: fatal error");
	goto bad;
    }

    irc_set_my_nickname(nick);

    if (*msg == ':') {
	msg++;
    }

    if (*msg) {
	ctx.spec_type = TYPE_SPEC1;
	printtext(&ctx, "%s", msg);
    }

    event_welcome_signalit();
    return;

  bad:
    event_welcome_signalit();
    net_send("QUIT");
    g_on_air = false;
}
