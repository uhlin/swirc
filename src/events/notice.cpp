/* Handles event NOTICE
   Copyright (C) 2014-2025 Markus Uhlin. All rights reserved.

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
#include <string>

#include "../commands/ignore.h"

#include "../dataClassify.h"
#include "../errHand.h"
#include "../irc.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "notice.h"
#include "special-msg-context.hpp"

#define INNER_B1	Theme("notice_inner_b1")
#define INNER_B2	Theme("notice_inner_b2")
#define NCOLOR1		Theme("notice_color1")
#define NCOLOR2		Theme("notice_color2")

struct notice_context {
	CSTRING srv_name;
	CSTRING dest;
	CSTRING msg;

	notice_context()
	    : srv_name(nullptr)
	    , dest(nullptr)
	    , msg(nullptr)
	{}

	notice_context(CSTRING p_srv_name, CSTRING p_dest, CSTRING p_msg)
	    : srv_name(p_srv_name)
	    , dest(p_dest)
	    , msg(p_msg)
	{}
};

static void
handle_notice_while_connecting(struct irc_message_compo *compo)
{
	CSTRING			msg = strchr(compo->params, ':');
	CSTRING			srv_host = (compo->prefix ? &compo->prefix[1] :
				    "auth");
	PRINTTEXT_CONTEXT	ctx;

	if (msg == nullptr || strings_match(++msg, ""))
		return;
	printtext_context_init(&ctx, g_status_window, TYPE_SPEC_NONE, true);
	printtext(&ctx, "%s!%s%c %s", COLOR3, srv_host, NORMAL, msg);
}

static void
handle_notice_from_my_server(const struct notice_context *ctx)
{
	if (g_my_nickname && strings_match_ignore_case(ctx->dest,
	    g_my_nickname)) {
		PRINTTEXT_CONTEXT	ptext_ctx;

		printtext_context_init(&ptext_ctx, g_status_window,
		    TYPE_SPEC_NONE, true);
		printtext(&ptext_ctx, "%s!%s%c %s", COLOR3, ctx->srv_name,
		    NORMAL, ctx->msg);
	}
}

static void
output_ctcp_reply(CSTRING cmd, const struct special_msg_context *ctx,
    CSTRING msg)
{
	PRINTTEXT_CONTEXT	ptext_ctx;

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
	STRING msg = sw_strdup(ctx->msg);

	squeeze(msg, "\x01");
	msg = trim(msg);
	if (strncmp(msg, "TIME ", 5) == STRINGS_MATCH)
		output_ctcp_reply("TIME", ctx, &msg[5]);
	else if (strncmp(msg, "USERINFO ", 9) == STRINGS_MATCH)
		output_ctcp_reply("USERINFO", ctx, &msg[9]);
	else if (strncmp(msg, "VERSION ", 8) == STRINGS_MATCH)
		output_ctcp_reply("VERSION", ctx, &msg[8]);
	free(msg);
}

/* event_notice

   Examples:
     :irc.server.com NOTICE <dest> :<msg>
     :<nick>!<user>@<host> NOTICE <dest> :<msg> */
void
event_notice(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ptext_ctx;

	try {
		CSTRING dest, msg;
		CSTRING nick, user, host;
		STRING	prefix;
		STRING	state[2];

		printtext_context_init(&ptext_ctx, nullptr, TYPE_SPEC_NONE,
		    true);
		state[0] = state[1] = const_cast<STRING>("");

		if (has_server_time(compo)) {
			set_timestamp(ptext_ctx.server_time,
			    ARRAY_SIZE(ptext_ctx.server_time), compo);
			ptext_ctx.has_server_time = true;
		}

		if (atomic_load_bool(&g_connection_in_progress) ||
		    g_server_hostname == nullptr) {
			handle_notice_while_connecting(compo);
			return;
		} else if ((prefix = compo->prefix) == nullptr)
			throw std::runtime_error("no prefix!");
		else if (strFeed(compo->params, 1) != 1)
			throw std::runtime_error("strFeed");
		else if ((dest = strtok_r(compo->params, "\n", &state[0])) ==
		    nullptr)
			throw std::runtime_error("no destination");
		else if ((msg = strtok_r(nullptr, "\n", &state[0])) == nullptr)
			throw std::runtime_error("no message");

		if (*prefix == ':')
			prefix++;
		if (*msg == ':')
			msg++;
		if (strings_match_ignore_case(prefix, g_server_hostname)) {
			struct notice_context ctx(prefix, dest, msg);

			handle_notice_from_my_server(&ctx);
			return;
		}

		if ((nick = strtok_r(prefix, "!@", &state[1])) == nullptr)
			throw std::runtime_error("no nickname");
		if ((user = strtok_r(nullptr, "!@", &state[1])) == nullptr)
			user = "<no user>";
		if ((host = strtok_r(nullptr, "!@", &state[1])) == nullptr)
			host = "<no host>";
		if (is_in_ignore_list(nick, user, host))
			return;

		if (*msg == g_ascii_soh) {
			/*
			 * Special message
			 */

			struct special_msg_context msg_ctx(nick, user, host,
			    dest, msg);

			handle_special_msg(&msg_ctx);
			return;
		} else if ((ptext_ctx.window =
		    window_by_label(dest)) != nullptr && is_irc_channel(dest)) {
			/*
			 * Output notice in IRC channel
			 */

			std::string	str(Theme("notice_lb"));

			(void) str.append(NCOLOR1).append(nick).append(TXT_NORMAL);
			(void) str.append(Theme("notice_sep"));
			(void) str.append(NCOLOR2).append(dest).append(TXT_NORMAL);
			(void) str.append(Theme("notice_rb"));

			printtext(&ptext_ctx, "%s %s", str.c_str(), msg);
		} else {
			STRING str;

			if (strings_match_ignore_case(dest, g_my_nickname)) {
				if ((ptext_ctx.window = window_by_label(nick))
				    == nullptr)
					ptext_ctx.window = g_active_window;
			} else {
				if ((ptext_ctx.window = window_by_label(dest))
				    == nullptr)
					ptext_ctx.window = g_status_window;
			}

			str = get_notice(nick, user, host);
			printtext(&ptext_ctx, "%s %s", str, msg);
			free(str);
		}
	} catch (const std::bad_alloc &e) {
		err_exit(ENOMEM, "%s: error: %s", __func__, e.what());
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ptext_ctx, g_status_window,
		    TYPE_SPEC1_WARN, true);
		printtext(&ptext_ctx, "%s: error: %s", __func__, e.what());
	} catch (...) {
		err_log(0, "%s: error: unknown exception", __func__);
	}
}

STRING
get_notice(CSTRING nick, CSTRING user, CSTRING host)
{
	std::string str("");

	if (nick == nullptr || user == nullptr || host == nullptr)
		return sw_strdup("");

	(void) str.append(Theme("notice_lb"));
	(void) str.append(NCOLOR1).append(nick).append(TXT_NORMAL);
	(void) str.append(INNER_B1);
	(void) str.append(NCOLOR2);
	(void) str.append(user).append("@").append(host);
	(void) str.append(TXT_NORMAL);
	(void) str.append(INNER_B2);
	(void) str.append(Theme("notice_rb"));

	return sw_strdup(str.c_str());
}
