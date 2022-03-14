/* Miscellaneous commands
   Copyright (C) 2016-2022 Markus Uhlin. All rights reserved.

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

#ifdef UNIX
#include <sys/socket.h> /* shutdown() */
#endif

#include "../irc.h"
#include "../events/welcome.h"

#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../icb.h"
#include "../io-loop.h"
#include "../libUtils.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../terminal.h"

#include "connect.h"
#include "misc.h"

static void
output_error(const char *message)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);
    printtext(&ctx, "%s", message);
}

/* usage: /away [reason] */
void
cmd_away(const char *data)
{
    const bool has_reason = !strings_match(data, "");

    if (has_reason) {
	if (net_send("AWAY :%s", data) < 0)
	    g_on_air = false;
	else
	    g_is_away = true;
    } else {
	if (net_send("AWAY") < 0)
	    g_on_air = false;
	else
	    g_is_away = false;
    }
}

/* usage: /banlist [channel] */
void
cmd_banlist(const char *data)
{
    if (strings_match(data, "")) {
	/* -------------------------------------------------- */

	if (is_irc_channel(g_active_window->label)) {
	    if (net_send("MODE %s +b", g_active_window->label) < 0)
		g_on_air = false;
	} else {
	    output_error("/banlist: missing arguments");
	}

	/* -------------------------------------------------- */
    } else if (!is_irc_channel(data)) {
	output_error("/banlist: bogus irc channel");
    } else {
	if (net_send("MODE %s +b", data) < 0)
	    g_on_air = false;
    }
}

/* usage: /beep <nickname> */
void
cmd_beep(const char *data)
{
    if (!g_icb_mode)
	return;
    if (!strings_match(data, ""))
	icb_send_beep(data);
}

/* usage: /boot <victim> */
void
cmd_boot(const char *data)
{
    if (!g_icb_mode)
	return;
    if (!strings_match(data, ""))
	icb_send_boot(data);
}

/*
 * usage: /close
 */
void
cmd_close(const char *data)
{
	if (!strings_match(data, ""))
		output_error("/close: implicit trailing data");
	else if (g_active_window == g_status_window)
		output_error("/close: cannot close status window");
	else if (is_irc_channel(g_active_window->label) && g_on_air)
		output_error("/close: cannot close window (connected)");
	else
		destroy_chat_window(g_active_window->label);
}

static bool
has_channel_key(const char *channel, char **key)
{
	PIRC_WINDOW	 win;
	char		*chanmodes_copy = NULL;
	char		*last = "";
	char		*modes;
	char		*params[4] = { NULL };
	size_t		 assigned_params = 0;

	if ((win = window_by_label(channel)) == NULL)
		goto no;

	chanmodes_copy = sw_strdup(win->chanmodes);

	if ((modes = strtok_r(chanmodes_copy, " ", &last)) == NULL ||
	    strchr(modes, 'k') == NULL)
		goto no;

	/*
	 * We only want the modes that takes a value. For example 'l'
	 * (user limit), and of course 'k' (channel key). The rest we
	 * squeeze...
	 */
	squeeze(modes,
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ+abcde"
	    "ghi"
	    "mnopqrstuvwxyz");

	while (assigned_params < ARRAY_SIZE(params)) {
		char *token;

		if ((token = strtok_r(NULL, " ", &last)) == NULL)
			break;
		params[assigned_params] = token;
		assigned_params++;
	}

	const size_t spanned = strcspn(modes, "k");

	if (assigned_params != strlen(modes) || spanned >= ARRAY_SIZE(params) ||
	    params[spanned] == NULL)
		goto no;

	*key = sw_strdup(params[spanned]);
	free(chanmodes_copy);
	return true;

  no:
	*key = NULL;
	free(chanmodes_copy);
	return false;
}

static void
do_part_and_join(const char *_channel)
{
	char	*channel = sw_strdup(_channel);
	char	*key = NULL;

	if (has_channel_key(channel, &key)) {
		(void) net_send("PART %s", channel);
		(void) net_send("JOIN %s %s", channel, key);
	} else {
		(void) net_send("PART %s", channel);
		(void) net_send("JOIN %s", channel);
	}

	free(channel);
	free(key);
}

/*
 * usage: /cycle [channel]
 */
void
cmd_cycle(const char *data)
{
	if (strings_match(data, "")) {

		if (is_irc_channel(g_active_window->label)) {
			do_part_and_join(g_active_window->label);
		} else {
			output_error("/cycle: missing arguments");
		}

	} else if (!is_irc_channel(data)) {
		output_error("/cycle: bogus irc channel");
	} else {
		do_part_and_join(data);
	}
}

/*
 * usage: /exlist [channel]
 */
void
cmd_exlist(const char *data)
{
	if (strings_match(data, "")) {

		if (is_irc_channel(g_active_window->label)) {
			if (net_send("MODE %s +e", g_active_window->label) < 0)
				output_error("/exlist: cannot send");
		} else {
			output_error("/exlist: missing arguments");
		}

	} else if (!is_irc_channel(data)) {
		output_error("/exlist: bogus irc channel");
	} else {
		if (net_send("MODE %s +e", data) < 0)
			output_error("/exlist: cannot send");
	}
}

/*
 * usage: /group <name>
 */
void
cmd_group(const char *data)
{
	if (!g_icb_mode)
		return;
	if (!strings_match(data, ""))
		icb_send_group(data);
}

/*
 * usage: /ilist [channel]
 */
void
cmd_ilist(const char *data)
{
	if (strings_match(data, "")) {

		if (is_irc_channel(g_active_window->label)) {
			if (net_send("MODE %s +I", g_active_window->label) < 0)
				output_error("/ilist: cannot send");
		} else {
			output_error("/ilist: missing arguments");
		}

	} else if (!is_irc_channel(data)) {
		output_error("/ilist: bogus irc channel");
	} else {
		if (net_send("MODE %s +I", data) < 0)
			output_error("/ilist: cannot send");
	}
}

/*
 * usage: /kill <nickname> <comment>
 */
void
cmd_kill(const char *data)
{
	if (strings_match(data, ""))
		output_error("/kill: missing arguments");
	else
		(void) net_send("KILL %s", data);
}

/*
 * usage: /list [<max_users[,>min_users][,pattern][...]]
 */
void
cmd_list(const char *data)
{
	const bool	has_params = !strings_match(data, "");

	if (g_icb_mode) {
		icb_send_who("-g");
		return;
	}

	if (has_params) {
		if (net_send("LIST %s", data) < 0) {
			err_log(ENOTCONN, "/list");
			g_connection_lost = true;
		}
	} else {
		if (net_send("LIST") < 0) {
			err_log(ENOTCONN, "/list");
			g_connection_lost = true;
		}
	}
}

/*
 * usage: /mode <modes> [...]
 */
void
cmd_mode(const char *data)
{
	if (strings_match(data, ""))
		output_error("/mode: missing arguments");
	else if (net_send("MODE %s", data) < 0)
		g_connection_lost = true;
}

/*
 * usage: /oper <name> <password>
 */
void
cmd_oper(const char *data)
{
	if (strings_match(data, ""))
		output_error("/oper: missing arguments");
	else if (net_send("OPER %s", data) < 0)
		g_connection_lost = true;
}

/*
 * usage: /passmod <nickname>
 */
void
cmd_passmod(const char *data)
{
	if (!g_icb_mode)
		return;
	if (!strings_match(data, ""))
		icb_send_pass_mod(data);
}

/*
 * usage: /query [nick]
 */
void
cmd_query(const char *data)
{
	if (strings_match(data, "")) {
		if (is_valid_nickname(g_active_window->label)) {
			switch (destroy_chat_window(g_active_window->label)) {
			case EINVAL:
				err_exit(EINVAL, "destroy_chat_window");
				/* NOTREACHED */
				break;
			case ENOENT:
				output_error("/query: cannot find window!");
				break;
			default:
				break;
			}
		} else {
			output_error("/query: missing arguments");
		}
	} else if (!is_valid_nickname(data)) {
		output_error("/query: bogus nickname");
	} else {
		/* -------------------------------------------------- */

		switch (spawn_chat_window(data, data)) {
		case EINVAL:
			err_exit(EINVAL, "spawn_chat_window");
			/* NOTREACHED */
			break;
		case ENOSPC:
			output_error("/query: too many windows open!");
			break;
		default:
			break;
		}

		/* -------------------------------------------------- */
	}
}

/*
 * usage: /quit [message]
 */
void
cmd_quit(const char *data)
{
	const bool has_message = !strings_match(data, "");

	if (g_on_air) {
		g_disconnect_wanted = true;
		g_connection_lost = g_on_air = false;

		if (g_icb_mode)
			/* empty */;
		else if (has_message)
			(void) net_send("QUIT :%s", data);
		else
			(void) net_send("QUIT :%s", Config("quit_message"));

		if (atomic_load_bool(&g_connection_in_progress))
			event_welcome_signalit();
		while (atomic_load_bool(&g_irc_listening))
			(void) napms(1);
	}

	g_io_loop = false;
}

/*
 * usage: /resize
 */
void
cmd_resize(const char *data)
{
	if (!strings_match(data, ""))
		output_error("/resize: implicit trailing data");
	else
		term_resize_all();
}

/*
 * usage: /rules
 */
void
cmd_rules(const char *data)
{
	if (!strings_match(data, ""))
		output_error("/rules: implicit trailing data");
	else if (net_send("RULES") < 0)
		g_connection_lost = true;
}

static void
confirm_ctcp_sent(const char *cmd, const char *target)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_SUCCESS, true);
	printtext(&ctx, "CTCP %c%s%c request sent to %c%s%c", BOLD, cmd, BOLD,
	    BOLD, target, BOLD);
}

/*
 * usage: /time <target>
 */
void
cmd_time(const char *data)
{
	if (strings_match(data, ""))
		output_error("/time: missing arguments");
	else if (!is_valid_nickname(data) && !is_irc_channel(data))
		output_error("/time: neither a nickname or irc channel");
	else if (net_send("PRIVMSG %s :\001TIME\001", data) > 0)
		confirm_ctcp_sent("TIME", data);
	else
		err_log(ENOTCONN, "/time");
}

/*
 * usage: /version <target>
 */
void
cmd_version(const char *data)
{
	if (strings_match(data, ""))
		output_error("/version: missing arguments");
	else if (!is_valid_nickname(data) && !is_irc_channel(data))
		output_error("/version: neither a nickname or irc channel");
	else if (net_send("PRIVMSG %s :\001VERSION\001", data) > 0)
		confirm_ctcp_sent("VERSION", data);
	else
		err_log(ENOTCONN, "/version");
}

/*
 * usage: /who <mask>
 */
void
cmd_who(const char *data)
{
	const bool has_mask = !strings_match(data, "");

	if (has_mask) {
		if (net_send("WHO %s", data) < 0)
			g_connection_lost = true;
	} else {
		if (net_send("WHO") < 0)
			g_connection_lost = true;
	}
}

/*
 * usage: /whois <nick>
 */
void
cmd_whois(const char *data)
{
	if (strings_match(data, ""))
		output_error("/whois: missing arguments");
	else if (!is_valid_nickname(data))
		output_error("/whois: bogus nickname");
	else if (net_send("WHOIS %s %s", data, data) < 0)
		g_connection_lost = true;
}
