/* misc.c
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
#include "../errHand.h"
#include "../io-loop.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../terminal.h"

#include "misc.h"

static struct printtext_context ptext_ctx = {
    .window	= NULL,
    .spec_type	= TYPE_SPEC1_FAILURE,
    .include_ts = true,
};

/* usage: /quit [message] */
void
cmd_quit(const char *data)
{
    const bool has_message = !Strings_match(data, "");

    if (g_on_air) {
	if (has_message)
	    (void) net_send("QUIT :%s", data);
	else
	    (void) net_send("QUIT :%s", Config("quit_message"));
	g_on_air = false;
	net_listenThread_join();
    }

    g_io_loop = false;
}

/* usage: /whois <nick> */
void
cmd_whois(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	printtext(&ptext_ctx, "/whois: missing arguments");
    } else if (!is_valid_nickname(data)) {
	printtext(&ptext_ctx, "/whois: bogus nickname");
    } else {
	if (net_send("WHOIS %s %s", data, data) < 0)
	    g_on_air = false;
    }
}

/* usage: /query [nick] */
void
cmd_query(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	if (is_valid_nickname(g_active_window->label)) {
	    switch (destroy_chat_window(g_active_window->label)) {
	    case EINVAL:
		err_exit(EINVAL, "destroy_chat_window");
	    case ENOENT:
		printtext(&ptext_ctx, "/query: cannot find window!");
		break;
	    }
	} else {
	    printtext(&ptext_ctx, "/query: missing arguments");
	}
    } else if (!is_valid_nickname(data)) {
	printtext(&ptext_ctx, "/query: bogus nickname");
    } else {
	switch (spawn_chat_window(data, "")) {
	case EINVAL:
	    err_exit(EINVAL, "spawn_chat_window");
	case ENOSPC:
	    printtext(&ptext_ctx, "/query: too many windows open!");
	    break;
	}
    }
}

/* usage: /n [channel] */
void
cmd_names(const char *data)
{
    extern int event_names_print_all(const char *channel);

    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	if (is_irc_channel(g_active_window->label)) {
	    event_names_print_all(g_active_window->label);
	} else {
	    printtext(&ptext_ctx, "/n: missing arguments");
	}
    } else if (!is_irc_channel(data)) {
	printtext(&ptext_ctx, "/n: bogus irc channel");
    } else {
	event_names_print_all(data);
    }
}

/* usage: /mode <modes> [...] */
void
cmd_mode(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	printtext(&ptext_ctx, "/mode: missing arguments");
    } else {
	if (net_send("MODE %s", data) < 0)
	    g_on_air = false;
    }
}

/* usage: /resize */
void
cmd_resize(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (!Strings_match(data, "")) {
	printtext(&ptext_ctx, "/resize: implicit trailing data");
    } else {
	term_resize_all();
    }
}

/* usage: /away [reason] */
void
cmd_away(const char *data)
{
    const bool has_reason = !Strings_match(data, "");

    if (has_reason) {
	if (net_send("AWAY :%s", data) < 0)
	    g_on_air = false;
    } else {
	if (net_send("AWAY") < 0)
	    g_on_air = false;
    }
}

/* usage: /list [<max_users[,>min_users][,pattern][...]] */
void
cmd_list(const char *data)
{
    const bool has_params = !Strings_match(data, "");

    if (has_params) {
	if (net_send("LIST %s", data) < 0)
	    g_on_air = false;
    } else {
	if (net_send("LIST") < 0)
	    g_on_air = false;
    }
}

/* usage: /banlist [channel] */
void
cmd_banlist(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	if (is_irc_channel(g_active_window->label)) {
	    if (net_send("MODE %s +b", g_active_window->label) < 0)
		g_on_air = false;
	} else {
	    printtext(&ptext_ctx, "/banlist: missing arguments");
	}
    } else if (!is_irc_channel(data)) {
	printtext(&ptext_ctx, "/banlist: bogus irc channel");
    } else {
	if (net_send("MODE %s +b", data) < 0)
	    g_on_air = false;
    }
}

/* usage: /exlist [channel] */
void
cmd_exlist(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	if (is_irc_channel(g_active_window->label)) {
	    if (net_send("MODE %s +e", g_active_window->label) < 0)
		g_on_air = false;
	} else {
	    printtext(&ptext_ctx, "/exlist: missing arguments");
	}
    } else if (!is_irc_channel(data)) {
	printtext(&ptext_ctx, "/exlist: bogus irc channel");
    } else {
	if (net_send("MODE %s +e", data) < 0)
	    g_on_air = false;
    }
}

/* usage: /ilist [channel] */
void
cmd_ilist(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	if (is_irc_channel(g_active_window->label)) {
	    if (net_send("MODE %s +I", g_active_window->label) < 0)
		g_on_air = false;
	} else {
	    printtext(&ptext_ctx, "/ilist: missing arguments");
	}
    } else if (!is_irc_channel(data)) {
	printtext(&ptext_ctx, "/ilist: bogus irc channel");
    } else {
	if (net_send("MODE %s +I", data) < 0)
	    g_on_air = false;
    }
}

/* usage: /who [mask] */
void
cmd_who(const char *data)
{
    const bool has_mask = !Strings_match(data, "");

    if (has_mask) {
	if (net_send("WHO %s", data) < 0)
	    g_on_air = false;
    } else {
	if (net_send("WHO") < 0)
	    g_on_air = false;
    }
}

/* usage: /rules */
void
cmd_rules(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (!Strings_match(data, "")) {
	printtext(&ptext_ctx, "/rules: implicit trailing data");
    } else {
	if (net_send("RULES") < 0)
	    g_on_air = false;
    }
}

/* usage: /cycle [channel] */
void
cmd_cycle(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	if (is_irc_channel(g_active_window->label)) {
	    if (net_send("PART %s", g_active_window->label) < 0 ||
		net_send("JOIN %s", g_active_window->label) < 0)
		g_on_air = false;
	} else {
	    printtext(&ptext_ctx, "/cycle: missing arguments");
	}
    } else if (!is_irc_channel(data)) {
	printtext(&ptext_ctx, "/cycle: bogus irc channel");
    } else {
	if (net_send("PART %s", data) < 0 || net_send("JOIN %s", data) < 0)
	    g_on_air = false;
    }
}

static void
confirm_ctcp_sent(const char *cmd, const char *target)
{
    struct printtext_context ctx = {
	.window	    = g_active_window,
	.spec_type  = TYPE_SPEC1_SUCCESS,
	.include_ts = true,
    };

    printtext(&ctx, "CTCP %c%s%c request sent to %c%s%c",
	      BOLD, cmd, BOLD, BOLD, target, BOLD);
}

/* usage: /version <target> */
void
cmd_version(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	printtext(&ptext_ctx, "/version: missing arguments");
    } else if (!is_valid_nickname(data) && !is_irc_channel(data)) {
	printtext(&ptext_ctx, "/version: neither a nickname or irc channel");
    } else {
	if (net_send("PRIVMSG %s :\001VERSION\001", data) > 0)
	    confirm_ctcp_sent("VERSION", data);
    }
}

/* usage: /time <target> */
void
cmd_time(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	printtext(&ptext_ctx, "/time: missing arguments");
    } else if (!is_valid_nickname(data) && !is_irc_channel(data)) {
	printtext(&ptext_ctx, "/time: neither a nickname or irc channel");
    } else {
	if (net_send("PRIVMSG %s :\001TIME\001", data) > 0)
	    confirm_ctcp_sent("TIME", data);
    }
}
