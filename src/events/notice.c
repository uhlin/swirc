/* Handles event NOTICE
   Copyright (C) 2014, 2016-2017 Markus Uhlin. All rights reserved.

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

#include "notice.h"

#define INNER_B1	Theme("notice_inner_b1")
#define INNER_B2	Theme("notice_inner_b2")
#define NCOLOR1		Theme("notice_color1")
#define NCOLOR2		Theme("notice_color2")

struct notice_context {
    char *srv_name;
    char *dest;
    char *msg;
};

struct special_msg_context {
    char *nick;
    char *user;
    char *host;
    char *dest;
    char *msg;
};

static void
handle_notice_while_connecting(struct irc_message_compo *compo)
{
    struct printtext_context ctx = {
	.window     = g_status_window,
	.spec_type  = TYPE_SPEC_NONE,
	.include_ts = true,
    };
    const char	*msg      = strchr(compo->params, ':');
    const char	*srv_host = compo->prefix ? &compo->prefix[1] : "auth";

    if (msg == NULL || Strings_match(++msg, "")) {
	return;
    }

    printtext(&ctx, "%s!%s%c %s", COLOR3, srv_host, NORMAL, msg);
}

static void
handle_notice_from_my_server(const struct notice_context *ctx)
{
    struct printtext_context ptext_ctx = {
	.window     = g_status_window,
	.spec_type  = TYPE_SPEC_NONE,
	.include_ts = true,
    };

    if (g_my_nickname && Strings_match_ignore_case(ctx->dest, g_my_nickname))
	printtext(&ptext_ctx, "%s!%s%c %s",
	    COLOR3, ctx->srv_name, NORMAL, ctx->msg);
}

static void
output_ctcp_reply(const char *cmd, const struct special_msg_context *ctx,
		  const char *msg)
{
    struct printtext_context pt_ctx = {
	.window	    = g_active_window,
	.spec_type  = TYPE_SPEC2,
	.include_ts = true,
    };

    if (*msg == ':')
	msg++;

    printtext(&pt_ctx, "CTCP %c%s%c reply from %c%s%c %s%s@%s%s: %s",
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
    char *dest, *msg;
    char *nick, *user, *host;
    char *params = &compo->params[0];
    char *prefix = compo->prefix ? &compo->prefix[0] : NULL;
    char *state1, *state2;
    struct printtext_context ptext_ctx;

    state1 = state2 = "";

    if (g_connection_in_progress || g_server_hostname == NULL) {
	handle_notice_while_connecting(compo);
	return;
    } else if (!prefix) {
	/* if this happens it's either the server or a bug (or both) */
	goto bad;
    } else if (Strfeed(params, 1) != 1) {
	goto bad;
    } else if ((dest = strtok_r(params, "\n", &state1)) == NULL ||
	       (msg = strtok_r(NULL, "\n", &state1)) == NULL) {
	goto bad;
    } else {
	if (*prefix == ':')
	    prefix++;
	if (*msg == ':')
	    msg++;
    }

    if (Strings_match_ignore_case(prefix, g_server_hostname)) {
	struct notice_context ctx = {
	    .srv_name = prefix,
	    .dest     = dest,
	    .msg      = msg,
	};

	handle_notice_from_my_server(&ctx);
	return;
    } else if ((nick = strtok_r(prefix, "!@", &state2)) == NULL) {
	goto bad;
    }

    user = strtok_r(NULL, "!@", &state2);
    host = strtok_r(NULL, "!@", &state2);

    if (!user || !host) {
	user = "<no user>";
	host = "<no host>";
    }

    if (*msg == '\001') {
	struct special_msg_context msg_ctx = {
	    .nick = nick,
	    .user = user,
	    .host = host,
	    .dest = dest,
	    .msg  = msg,
	};

	handle_special_msg(&msg_ctx);
	return;
    } else if (window_by_label(dest) != NULL && is_irc_channel(dest)) {
	ptext_ctx.window     = window_by_label(dest);
	ptext_ctx.spec_type  = TYPE_SPEC_NONE;
	ptext_ctx.include_ts = true;
	printtext(&ptext_ctx, "%s%s%s%c%s%s%s%c%s %s",
	    Theme("notice_lb"),
	    NCOLOR1, nick, NORMAL, Theme("notice_sep"), NCOLOR2, dest, NORMAL,
	    Theme("notice_rb"),
	    msg);
    } else {
	if (Strings_match_ignore_case(dest, g_my_nickname))
	    ptext_ctx.window =
		window_by_label(nick) ? window_by_label(nick) : g_active_window;
	else
	    ptext_ctx.window =
		window_by_label(dest) ? window_by_label(dest) : g_status_window;

	ptext_ctx.spec_type  = TYPE_SPEC_NONE;
	ptext_ctx.include_ts = true;

	printtext(&ptext_ctx, "%s%s%s%c%s%s%s@%s%c%s%s %s",
		  Theme("notice_lb"),
		  NCOLOR1, nick, NORMAL,
		  INNER_B1, NCOLOR2, user, host, NORMAL, INNER_B2,
		  Theme("notice_rb"),
		  msg);
    }

    return;

  bad:
    ptext_ctx.window     = g_status_window;
    ptext_ctx.spec_type  = TYPE_SPEC1_FAILURE;
    ptext_ctx.include_ts = true;
    printtext(&ptext_ctx, "On issuing event %s: An error occurred",
	      compo->command);
    printtext(&ptext_ctx, "  params = %s", compo->params);
    printtext(&ptext_ctx, "  prefix = %s",
	      compo->prefix ? compo->prefix : "none");
}
