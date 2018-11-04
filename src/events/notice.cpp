/* Handles event NOTICE
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

#include <stdexcept>

#include "../dataClassify.h"
#include "../irc.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "notice.h"

#define INNER_B1	Theme("notice_inner_b1")
#define INNER_B2	Theme("notice_inner_b2")
#define NCOLOR1		Theme("notice_color1")
#define NCOLOR2		Theme("notice_color2")

struct notice_context {
    char *srv_name;
    char *dest;
    char *msg;

    notice_context(char *srv_name, char *dest, char *msg) {
	this->srv_name = srv_name;
	this->dest     = dest;
	this->msg      = msg;
    }
};

struct special_msg_context {
    char *nick;
    char *user;
    char *host;
    char *dest;
    char *msg;

    special_msg_context(
	char *nick, char *user, char *host, char *dest, char *msg) {
	this->nick = nick;
	this->user = user;
	this->host = host;
	this->dest = dest;
	this->msg  = msg;
    }
};

static void
handle_notice_while_connecting(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;
    const char	*msg      = strchr(compo->params, ':');
    const char	*srv_host = compo->prefix ? &compo->prefix[1] : "auth";

    if (msg == NULL || strings_match(++msg, "")) {
	return;
    }

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC_NONE, true);
    printtext(&ctx, "%s!%s%c %s", COLOR3, srv_host, NORMAL, msg);
}

static void
handle_notice_from_my_server(const struct notice_context *ctx)
{
    if (g_my_nickname && strings_match_ignore_case(ctx->dest, g_my_nickname)) {
	PRINTTEXT_CONTEXT ptext_ctx;

	printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC_NONE,
	    true);
	printtext(&ptext_ctx, "%s!%s%c %s",
	    COLOR3, ctx->srv_name, NORMAL, ctx->msg);
    }
}

static void
output_ctcp_reply(const char *cmd, const struct special_msg_context *ctx,
		  const char *msg)
{
    PRINTTEXT_CONTEXT ptext_ctx;

    if (*msg == ':')
	msg++;

    printtext_context_init(&ptext_ctx, g_active_window, TYPE_SPEC2, true);
    printtext(&ptext_ctx, "CTCP %c%s%c reply from %c%s%c %s%s@%s%s: %s",
	BOLD, cmd, BOLD, BOLD, ctx->nick, BOLD,
	LEFT_BRKT, ctx->user, ctx->host, RIGHT_BRKT,
	msg);
}

static void
handle_special_msg(const struct special_msg_context *ctx)
{
    char *msg = sw_strdup(ctx->msg);

    squeeze(msg, "\001");
    msg = trim(msg);

    if (!strncmp(msg, "VERSION ", 8)) {
	output_ctcp_reply("VERSION", ctx, &msg[8]);
    } else if (!strncmp(msg, "TIME ", 5)) {
	output_ctcp_reply("TIME", ctx, &msg[5]);
    } else {
	/* do nothing */;
    }

    free(msg);
}

/* event_notice

   Examples:
     :irc.server.com NOTICE <dest> :<msg>
     :<nick>!<user>@<host> NOTICE <dest> :<msg> */
void
event_notice(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ptext_ctx;

    try {
	char *dest, *msg;
	char *params = &compo->params[0];
	char *prefix = compo->prefix ? &compo->prefix[0] : NULL;
	char *state1, *state2;

	dest = msg = NULL;
	state1 = state2 = (char *) "";
	printtext_context_init(&ptext_ctx, NULL, TYPE_SPEC_NONE, true);

	if (has_server_time(compo)) {
	    set_timestamp(ptext_ctx.server_time, sizeof ptext_ctx.server_time,
			  compo);
	    ptext_ctx.has_server_time = true;
	}

	if (g_connection_in_progress || g_server_hostname == NULL) {
	    handle_notice_while_connecting(compo);
	    return;
	} else if (prefix == NULL) {
	    throw std::runtime_error("no prefix!");
	} else if (strFeed(params, 1) != 1) {
	    throw std::runtime_error("strFeed");
	} else if ((dest = strtok_r(params, "\n", &state1)) == NULL) {
	    throw std::runtime_error("no destination");
	} else if ((msg = strtok_r(NULL, "\n", &state1)) == NULL) {
	    throw std::runtime_error("no message");
	}

	if (*prefix == ':')
	    prefix++;
	if (*msg == ':')
	    msg++;

	if (strings_match_ignore_case(prefix, g_server_hostname)) {
	    struct notice_context ctx(prefix, dest, msg);

	    handle_notice_from_my_server(&ctx);
	    return;
	}

	char	*nick = strtok_r(prefix, "!@", &state2);
	char	*user = strtok_r(NULL, "!@", &state2);
	char	*host = strtok_r(NULL, "!@", &state2);

	if (nick == NULL)
	    throw std::runtime_error("no nickname");
	if (user == NULL)
	    user = (char *) "<no user>";
	if (host == NULL)
	    host = (char *) "<no host>";

	if (*msg == '\001') {
	    /*
	     * Special message
	     */
	    struct special_msg_context msg_ctx(nick, user, host, dest, msg);

	    handle_special_msg(&msg_ctx);
	    return;
	} else if ((ptext_ctx.window = window_by_label(dest)) != NULL &&
		   is_irc_channel(dest)) {
	    /*
	     * Output notice in IRC channel
	     */
	    printtext(&ptext_ctx, "%s%s%s%c%s%s%s%c%s %s",
		Theme("notice_lb"), NCOLOR1, nick, NORMAL, Theme("notice_sep"),
		NCOLOR2, dest, NORMAL, Theme("notice_rb"), msg);
	} else {
	    if (strings_match_ignore_case(dest, g_my_nickname)) {
		ptext_ctx.window = window_by_label(nick)
		    ? window_by_label(nick)
		    : g_active_window;
	    } else {
		ptext_ctx.window = window_by_label(dest)
		    ? window_by_label(dest)
		    : g_status_window;
	    }

	    printtext(&ptext_ctx, "%s%s%s%c%s%s%s@%s%c%s%s %s",
		Theme("notice_lb"),
		NCOLOR1, nick, NORMAL,
		INNER_B1, NCOLOR2, user, host, NORMAL, INNER_B2,
		Theme("notice_rb"),
		msg);
	}
    } catch (std::runtime_error &e) {
	printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_WARN,
	    true);
	printtext(&ptext_ctx, "event_notice: error: %s", e.what());
    }
}
