/* Handles event PRIVMSG
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

#include <stdexcept>

#include "../commands/dcc.h"
#include "../commands/ignore.h"

#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../irc.h"
#include "../libUtils.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../strdup_printf.h"
#include "../theme.h"

#if defined(WIN32) && defined(TOAST_NOTIFICATIONS)
#include "../DesktopNotificationManagerCompat.hpp"
#include "../ToastsAPI.hpp"
#elif defined(UNIX) && USE_LIBNOTIFY
#include <libnotify/notify.h>
#define SUMMARY_TEXT "Swirc IRC client"
#endif

#define NICK_S1 Theme("nick_s1")
#define NICK_S2 Theme("nick_s2")

#include "names.h"
#include "notice.h"
#include "privmsg.h"
#include "special-msg-context.hpp"
#if UNIX
#include "swircpaths.h"
#endif

/*
 * Notification message max length
 */
static const size_t nmsg_maxlen = 150;

static bool	shouldHighlightMessage_case1(CSTRING) NONNULL;
static bool	shouldHighlightMessage_case2(CSTRING) NONNULL;

static void
acknowledge_ctcp_request(CSTRING cmd, const struct special_msg_context *ctx)
{
	PRINTTEXT_CONTEXT	ptext_ctx;

	printtext_context_init(&ptext_ctx, g_active_window, TYPE_SPEC3, true);
	printtext(&ptext_ctx, "%c%s%c %s%s@%s%s requested CTCP %c%s%c from "
	    "%c%s%c",
	    BOLD, ctx->nick, BOLD, LEFT_BRKT, ctx->user, ctx->host, RIGHT_BRKT,
	    BOLD, cmd, BOLD, BOLD, ctx->dest, BOLD);
}

static void
handle_special_msg(const struct special_msg_context *ctx)
{
	PRINTTEXT_CONTEXT ptext_ctx;
	STRING msg = sw_strdup(ctx->msg);

	printtext_context_init(&ptext_ctx, NULL, TYPE_SPEC_NONE, true);
	squeeze(msg, "\001");
	msg = trim(msg);

	if (strings_match_ignore_case(ctx->dest, g_my_nickname)) {
		if (!strncmp(msg, "ACTION ", 7) &&
		    (ptext_ctx.window = window_by_label(ctx->nick)) == NULL)
			spawn_chat_window(ctx->nick, ctx->nick);
		ptext_ctx.window = window_by_label(ctx->nick);
	} else {
		ptext_ctx.window = window_by_label(ctx->dest);
	}

	if (!(ptext_ctx.window))
		ptext_ctx.window = g_active_window;

	if (!strncmp(msg, "ACTION ", 7)) {
		printtext(&ptext_ctx, " - %s %s", ctx->nick, &msg[7]);
	} else if (!strncmp(msg, "SW_DCC SEND ", 12) && config_bool("dcc",
	    true)) {
		dcc::add_file(ctx->nick, ctx->user, ctx->host, &msg[12]);
	} else if (!strncmp(msg, "TIME", 5) && config_bool("ctcp_reply",
	    true)) {
		if (net_send("NOTICE %s :\001TIME %s\001", ctx->nick,
		    current_time("%c")) < 0)
			g_connection_lost = true;
		acknowledge_ctcp_request("TIME", ctx);
	} else if (!strncmp(msg, "VERSION", 8) && config_bool("ctcp_reply",
	    true)) {
		if (net_send("NOTICE %s :"
		    "\001VERSION Swirc %s by %s  --  %s\001", ctx->nick,
		    g_swircVersion, g_swircAuthor, g_swircWebAddr) < 0)
			g_connection_lost = true;
		acknowledge_ctcp_request("VERSION", ctx);
	} else {
		/* do nothing */;
	}
	free(msg);
}

static void
broadcast_window_activity(const IRC_WINDOW *src)
{
	PRINTTEXT_CONTEXT	ctx;

	if (src) {
		printtext_context_init(&ctx, g_active_window,
		    TYPE_SPEC1_SUCCESS, true);
		printtext(&ctx, "activity at window %c%s%c (refnum: %d)",
		    BOLD, src->label, BOLD, src->refnum);
	}
}

static bool
shouldHighlightMessage_case1(CSTRING msg)
{
	bool	 result = false;
	STRING	 s1 = strdup_printf("%s:", g_my_nickname);
	STRING	 s2 = strdup_printf("%s,", g_my_nickname);
	STRING	 s3 = strdup_printf("%s ", g_my_nickname);
	STRING	 s4 = strdup_printf(" %s ", g_my_nickname);

	if (!strncasecmp(msg, s1, strlen(s1)) ||
	    !strncasecmp(msg, s2, strlen(s2)) ||
	    !strncasecmp(msg, s3, strlen(s3)) ||
	    strcasestr(msg, s4) != NULL ||
	    strings_match_ignore_case(msg, g_my_nickname))
		result = true;
	free(s1);
	free(s2);
	free(s3);
	free(s4);
	return result;
}

static bool
shouldHighlightMessage_case2(CSTRING msg)
{
	bool	 result = false;
	char	*last = const_cast<char *>("");
	char	*nickname_aliases = sw_strdup(Config("nickname_aliases"));

	for (char *cp = &nickname_aliases[0];; cp = NULL) {
		CSTRING token;

		if ((token = strtok_r(cp, " ", &last)) == NULL) {
			result = false;
			break;
		} else if (!is_valid_nickname(token)) {
#if RISK_FLOODED_LOGS
			err_log(0, "config option nickname_aliases contains "
			    "invalid nicknames");
#endif
			continue;
		} else {
			STRING	 s1 = strdup_printf("%s:", token);
			STRING	 s2 = strdup_printf("%s,", token);
			STRING	 s3 = strdup_printf("%s ", token);

			if (!strncasecmp(msg, s1, strlen(s1)) ||
			    !strncasecmp(msg, s2, strlen(s2)) ||
			    !strncasecmp(msg, s3, strlen(s3)) ||
			    strings_match_ignore_case(msg, token))
				result = true;

			free_and_null(&s1);
			free_and_null(&s2);
			free_and_null(&s3);

			if (result)
				break;
		}
	}

	free_and_null(&nickname_aliases);
	return result;
}

#if defined(WIN32) && defined(TOAST_NOTIFICATIONS)
static wchar_t *
get_converted_wcs(CSTRING s)
{
	const size_t	 size = strlen(s) + 1;
	wchar_t		*out = new wchar_t[size];

	if (MultiByteToWideChar(CP_UTF8, 0, s, -1, out, size_to_int(size)) > 0)
		return out;
	*out = L'\0';
	return out;
}

static wchar_t *
get_message(const wchar_t *s1, const wchar_t *s2, const wchar_t *s3,
    const wchar_t *s4, const wchar_t *s5)
{
	static wchar_t	message[1001] = { L'\0' };

	(void) wmemset(message, 0L, ARRAY_SIZE(message));

	(void) sw_wcscpy(message, s1, ARRAY_SIZE(message));
	(void) sw_wcscat(message, s2, ARRAY_SIZE(message));
	(void) sw_wcscat(message, s3, ARRAY_SIZE(message));
	(void) sw_wcscat(message, s4, ARRAY_SIZE(message));
	(void) sw_wcscat(message, s5, ARRAY_SIZE(message));

	return (&message[0]);
}
#endif /* ----- WIN32 and TOAST_NOTIFICATIONS ----- */

static void
handle_msgs_from_my_server(PPRINTTEXT_CONTEXT ctx, CSTRING dest,
    CSTRING msg)
{
	if (g_my_nickname && strings_match_ignore_case(dest, g_my_nickname))
		ctx->window = g_active_window;
	else if ((ctx->window = window_by_label(dest)) == NULL)
		ctx->window = g_active_window;
	printtext(ctx, "%s!%s%s %s", COLOR3, g_server_hostname, TXT_NORMAL,
	    msg);
}

static void
handle_znc_msgs(PPRINTTEXT_CONTEXT ctx, CSTRING notice, CSTRING msg)
{
	ctx->window = g_active_window;
	printtext(ctx, "%s %s", notice, msg);
}

static void
handle_private_msgs(PPRINTTEXT_CONTEXT ctx, CSTRING nick, CSTRING msg)
{
	STRING msg_copy;

	if ((ctx->window = window_by_label(nick)) == NULL)
		throw std::runtime_error("window lookup error");

	printtext(ctx, "%s%s%s%c%s %s", NICK_S1, COLOR2, nick, NORMAL, NICK_S2,
	    msg);
	msg_copy = sw_strdup(msg);
	if (strlen(msg_copy) > nmsg_maxlen)
		msg_copy[nmsg_maxlen] = '\0';

#if defined(WIN32) && defined(TOAST_NOTIFICATIONS)
	wchar_t *wNick = get_converted_wcs(nick);
	wchar_t *wMsg = get_converted_wcs(msg_copy);

	Toasts::SendBasicToast(get_message(L"[PM]", L" <", wNick, L"> ", wMsg));

	delete[] wNick;
	delete[] wMsg;
#elif defined(UNIX) && USE_LIBNOTIFY
	STRING body = strdup_printf("[PM] &lt;%s&gt; %s", nick, msg_copy);
	NotifyNotification *notification = notify_notification_new(SUMMARY_TEXT,
	    body, SWIRC_ICON_PATH);

	notify_notification_show(notification, NULL);

	free(body);
	g_object_unref(G_OBJECT(notification));
#endif

	free(msg_copy);
	if (ctx->window != g_active_window)
		broadcast_window_activity(ctx->window);
}

static void
handle_chan_msgs(PPRINTTEXT_CONTEXT ctx, CSTRING nick, CSTRING dest,
    CSTRING msg)
{
	PNAMES	n = NULL;
	char	c = '!';

	if ((ctx->window = window_by_label(dest)) == NULL) {
		throw std::runtime_error("bogus window label");
	} else if ((n = event_names_htbl_lookup(nick, dest)) != NULL) {
		if (n->is_owner)
			c = '~';
		else if (n->is_superop)
			c = '&';
		else if (n->is_op)
			c = '@';
		else if (n->is_halfop)
			c = '%';
		else if (n->is_voice)
			c = '+';
		else
			c = ' ';
	}

	if (shouldHighlightMessage_case1(msg) ||
	    shouldHighlightMessage_case2(msg)) {
		STRING msg_copy;

		printtext(ctx, "%s%c%s%s%c%s %s",
		    NICK_S1, c, COLOR4, nick, NORMAL, NICK_S2,
		    msg);
		msg_copy = sw_strdup(msg);
		if (strlen(msg_copy) > nmsg_maxlen)
			msg_copy[nmsg_maxlen] = '\0';

		if (ctx->window != g_active_window)
			broadcast_window_activity(ctx->window);

#if defined(WIN32) && defined(TOAST_NOTIFICATIONS)
		wchar_t *wNick = get_converted_wcs(nick);
		wchar_t *wDest = get_converted_wcs(dest);
		wchar_t *wMsg = get_converted_wcs(msg_copy);

		Toasts::SendBasicToast(get_message(wNick, L" @ ", wDest, L": ",
		    wMsg));

		delete[] wNick;
		delete[] wDest;
		delete[] wMsg;
#elif defined(UNIX) && USE_LIBNOTIFY
		STRING body = strdup_printf("%s @ %s: %s", nick, dest,
		    msg_copy);
		NotifyNotification *notification = notify_notification_new
		    (SUMMARY_TEXT, body, SWIRC_ICON_PATH);

		notify_notification_show(notification, NULL);

		free(body);
		g_object_unref(G_OBJECT(notification));
#endif
		free(msg_copy);
	} else {
		/*
		 * Normal message with no highlighting
		 */

		printtext(ctx, "%s%c%s%s%c%s %s",
		    NICK_S1, c, COLOR2, nick, NORMAL, NICK_S2,
		    msg);
	}
}

/* event_privmsg

   Examples:
     :<nick>!<user>@<host> PRIVMSG <dest> :<msg>
     :<nick>!<user>@<host> PRIVMSG <dest> :\001ACTION ...\001
     :<nick>!<user>@<host> PRIVMSG <dest> :\001VERSION\001 */
void
event_privmsg(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	try {
		char *dest, *msg;
		char *nick, *user, *host;
		char *prefix;
		char *state1 = const_cast<char *>("");
		char *state2 = const_cast<char *>("");

		printtext_context_init(&ctx, NULL, TYPE_SPEC_NONE, true);

		if (has_server_time(compo)) {
			set_timestamp(ctx.server_time,
			    ARRAY_SIZE(ctx.server_time), compo);
			ctx.has_server_time = true;
		}

		if ((prefix = compo->prefix) == NULL)
			throw std::runtime_error("no prefix");
		else if (*prefix == ':')
			prefix++;

		if (strFeed(compo->params, 1) != 1)
			throw std::runtime_error("strFeed");
		if ((dest = strtok_r(compo->params, "\n", &state2)) == NULL)
			throw std::runtime_error("no destination");
		else if ((msg = strtok_r(NULL, "\n", &state2)) == NULL)
			throw std::runtime_error("no message");
		else if (*msg == ':')
			msg++;

		if (g_server_hostname &&
		    strings_match_ignore_case(prefix, g_server_hostname)) {
			handle_msgs_from_my_server(&ctx, dest, msg);
			return;
		}

		if ((nick = strtok_r(prefix, "!@", &state1)) == NULL)
			throw std::runtime_error("no nickname");
		if ((user = strtok_r(NULL, "!@", &state1)) == NULL)
			user = const_cast<char *>("<no user>");
		if ((host = strtok_r(NULL, "!@", &state1)) == NULL)
			host = const_cast<char *>("<no host>");
		if (is_in_ignore_list(nick, user, host))
			return;

		if (*msg == '\001') {
			struct special_msg_context msg_ctx(nick, user, host,
			    dest, msg);

			handle_special_msg(&msg_ctx);
			return;
		}

		if (strings_match_ignore_case(dest, g_my_nickname)) {
			if (*nick == '*' && !g_icb_mode) {
				char *notice;

				notice = get_notice(nick + 1, "znc", "znc.in");
				handle_znc_msgs(&ctx, notice, msg);
				free(notice);
			} else {
				if (window_by_label(nick) == NULL &&
				    spawn_chat_window(nick, nick) != 0)
					throw std::runtime_error("cannot spawn "
					    "chat window");
				handle_private_msgs(&ctx, nick, msg);
			}
		} else {
			/*
			 * Dest is an IRC channel
			 */

			if (window_by_label(dest) == NULL &&
			    spawn_chat_window(dest, "No title.") != 0)
				throw std::runtime_error("spawn_chat_window");
			handle_chan_msgs(&ctx, nick, dest, msg);
		}
	} catch (const std::bad_alloc &e) {
		err_exit(ENOMEM, "%s: error: %s", __func__, e.what());
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN,
		    true);
		printtext(&ctx, "%s: error: %s", __func__, e.what());
	} catch (...) {
		err_log(0, "%s: error: unknown exception", __func__);
	}
}
