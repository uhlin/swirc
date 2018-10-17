/* Handles event PRIVMSG
   Copyright (C) 2016-2018 Markus Uhlin. All rights reserved.

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
#include "../DesktopToastsApp.hpp"
#endif

#include "names.h"
#include "privmsg.h"

struct special_msg_context {
    char *nick;
    char *user;
    char *host;
    char *dest;
    char *msg;

    special_msg_context() {
	this->nick = NULL;
	this->user = NULL;
	this->host = NULL;
	this->dest = NULL;
	this->msg  = NULL;
    }

    special_msg_context(
	char *nick,
	char *user,
	char *host,
	char *dest,
	char *msg) {
	this->nick = nick;
	this->user = user;
	this->host = host;
	this->dest = dest;
	this->msg  = msg;
    }
};

static void
acknowledge_ctcp_request(const char *cmd, const struct special_msg_context *ctx)
{
    struct printtext_context pt_ctx(g_active_window, TYPE_SPEC3, true);

    printtext(&pt_ctx, "%c%s%c %s%s@%s%s requested CTCP %c%s%c form %c%s%c",
	BOLD, ctx->nick, BOLD, LEFT_BRKT, ctx->user, ctx->host, RIGHT_BRKT,
	BOLD, cmd, BOLD, BOLD, ctx->dest, BOLD);
}

static void
handle_special_msg(const struct special_msg_context *ctx)
{
    char *msg = sw_strdup(ctx->msg);
    struct printtext_context pt_ctx;

    squeeze(msg, "\001");
    msg = trim(msg);

    if (strings_match_ignore_case(ctx->dest, g_my_nickname)) {
	if (!strncmp(msg, "ACTION ", 7) &&
	    (pt_ctx.window = window_by_label(ctx->nick)) == NULL)
	    spawn_chat_window(ctx->nick, ctx->nick);
	pt_ctx.window = window_by_label(ctx->nick);
    } else {
	pt_ctx.window = window_by_label(ctx->dest);
    }

    if (! (pt_ctx.window))
	pt_ctx.window = g_active_window;
    pt_ctx.spec_type  = TYPE_SPEC_NONE;
    pt_ctx.include_ts = true;

    if (!strncmp(msg, "ACTION ", 7)) {
	printtext(&pt_ctx, " - %s %s", ctx->nick, &msg[7]);
    } else if (!strncmp(msg, "VERSION", 8)) {
	if (net_send("NOTICE %s :\001VERSION Swirc %s by %s  --  %s\001",
	    ctx->nick, g_swircVersion, g_swircAuthor, g_swircWebAddr) < 0)
	    g_on_air = false;
	acknowledge_ctcp_request("VERSION", ctx);
    } else if (!strncmp(msg, "TIME", 5)) {
	if (net_send("NOTICE %s :\001TIME %s\001",
		     ctx->nick, current_time("%c")) < 0)
	    g_on_air = false;
	acknowledge_ctcp_request("TIME", ctx);
    } else {
	/* do nothing */;
    }

    free(msg);
}

static void
broadcast_window_activity(PIRC_WINDOW src)
{
    struct printtext_context ctx(g_active_window, TYPE_SPEC1_SUCCESS, true);

    if (src)
	printtext(&ctx, "activity at window %c%s%c (refnum: %d)",
		  BOLD, src->label, BOLD, src->refnum);
}

static PTR_ARGS_NONNULL bool
shouldHighlightMessage_case1(const char *msg)
{
    bool result = false;
    char *s1 = strdup_printf("%s:", g_my_nickname);
    char *s2 = strdup_printf("%s,", g_my_nickname);
    char *s3 = strdup_printf("%s ", g_my_nickname);

    if (!strncasecmp(msg, s1, strlen(s1)) ||
	!strncasecmp(msg, s2, strlen(s2)) ||
	!strncasecmp(msg, s3, strlen(s3)) ||
	strings_match_ignore_case(msg, g_my_nickname))
	result = true;
    free(s1);
    free(s2);
    free(s3);
    return result;
}

static PTR_ARGS_NONNULL bool
shouldHighlightMessage_case2(const char *msg)
{
    bool result = false;
    char *last = (char *) "";
    char *nickname_aliases = sw_strdup(Config("nickname_aliases"));

    for (char *cp = &nickname_aliases[0];; cp = NULL) {
	char *token = NULL;

	if ((token = strtok_r(cp, " ", &last)) == NULL) {
	    result = false;
	    break;
	} else if (!is_valid_nickname(token)) {
#if RISK_FLOODED_LOGS
	    err_log(0, "config option nickname_aliases "
		"contains invalid nicknames");
#endif
	    continue;
	} else {
	    char *s1 = strdup_printf("%s:", token);
	    char *s2 = strdup_printf("%s,", token);
	    char *s3 = strdup_printf("%s ", token);

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
get_converted_wcs(const char *s)
{
    const size_t sz1 = strlen(s) + 1;
    const size_t sz2 = size_product(sizeof (wchar_t), sz1);
    wchar_t *out = (wchar_t *) xmalloc(sz2);

    if (MultiByteToWideChar(CP_UTF8, 0, s, -1, out, size_to_int(sz1)) > 0)
	return out;
    wmemset(out, 0L, sz1);
    return out;
}

static wchar_t *
get_message(
    const wchar_t *s1,
    const wchar_t *s2,
    const wchar_t *s3,
    const wchar_t *s4,
    const wchar_t *s5)
{
    static wchar_t message[1001];

    wmemset(message, 0L, ARRAY_SIZE(message));

    sw_wcscpy(message, s1, ARRAY_SIZE(message));
    sw_wcscat(message, s2, ARRAY_SIZE(message));
    sw_wcscat(message, s3, ARRAY_SIZE(message));
    sw_wcscat(message, s4, ARRAY_SIZE(message));
    sw_wcscat(message, s5, ARRAY_SIZE(message));

    return (&message[0]);
}
#endif /* ----- WIN32 and TOAST_NOTIFICATIONS ----- */

/* event_privmsg

   Examples:
     :<nick>!<user>@<host> PRIVMSG <dest> :<msg>
     :<nick>!<user>@<host> PRIVMSG <dest> :\001ACTION ...\001
     :<nick>!<user>@<host> PRIVMSG <dest> :\001VERSION\001 */
void
event_privmsg(struct irc_message_compo *compo)
{
    char	*dest, *msg;
    char	*nick, *user, *host;
    char	*params = &compo->params[0];
    char	*prefix = compo->prefix ? &compo->prefix[0] : NULL;
    char	*state1, *state2;
    struct printtext_context ctx(NULL, TYPE_SPEC_NONE, true);

    state1 = state2 = (char *) "";

    if (has_server_time(compo))
	set_timestamp(ctx.timestamp, sizeof ctx.timestamp, compo);

    if (!prefix)
	return;
    if (*prefix == ':')
	prefix++;
    if ((nick = strtok_r(prefix, "!@", &state1)) == NULL)
	return;
    user = strtok_r(NULL, "!@", &state1);
    host = strtok_r(NULL, "!@", &state1);
    if (!user || !host) {
	user = (char *) "<no user>";
	host = (char *) "<no host>";
    }
    if (strFeed(params, 1) != 1 ||
	(dest = strtok_r(params, "\n", &state2)) == NULL ||
	(msg = strtok_r(NULL, "\n", &state2)) == NULL)
	return;
    if (*msg == ':')
	msg++;
    if (*msg == '\001') {
	struct special_msg_context msg_ctx(nick, user, host, dest, msg);

	handle_special_msg(&msg_ctx);

	return;
    }
    if (strings_match_ignore_case(dest, g_my_nickname)) {
	if (window_by_label(nick) == NULL &&
	    spawn_chat_window(nick, nick) != 0)
	    return;
    } else {
	if (window_by_label(dest) == NULL &&
	    spawn_chat_window(dest, "No title.") != 0)
	    return;
    }

    if (strings_match_ignore_case(dest, g_my_nickname)) {
	if ((ctx.window = window_by_label(nick)) == NULL) {
	    err_log(0, "In event_privmsg: can't find a window with label %s",
		    nick);
	    return;
	}

	printtext(&ctx, "%s%s%s%c%s %s",
	    Theme("nick_s1"), COLOR2, nick, NORMAL, Theme("nick_s2"), msg);

#if defined(WIN32) && defined(TOAST_NOTIFICATIONS)
	wchar_t *wNick = get_converted_wcs(nick);
	wchar_t *wMsg  = get_converted_wcs(msg);

	DesktopToastsApp::SendBasicToast(
	    get_message(L"[PM]", L" <", wNick, L"> ", wMsg));

	free(wNick);
	free(wMsg);
#endif

	if (ctx.window != g_active_window)
	    broadcast_window_activity(ctx.window);
    } else {
	PNAMES	n = NULL;
	char	c = ' ';

	if ((ctx.window = window_by_label(dest)) == NULL ||
	    (n = event_names_htbl_lookup(nick, dest)) == NULL) {
	    err_log(0, "In event_privmsg: "
		"bogus window label / hash table lookup error");
	    return;
	}

	if (n->is_owner)        c = '~';
	else if (n->is_superop) c = '&';
	else if (n->is_op)      c = '@';
	else if (n->is_halfop)  c = '%';
	else if (n->is_voice)   c = '+';
	else c = ' ';

	if (shouldHighlightMessage_case1(msg) ||
	    shouldHighlightMessage_case2(msg)) {
	    printtext(&ctx, "%s%c%s%s%c%s %s",
		Theme("nick_s1"), c, COLOR4, nick, NORMAL, Theme("nick_s2"),
		msg);

#if defined(WIN32) && defined(TOAST_NOTIFICATIONS)
	    wchar_t *wNick = get_converted_wcs(nick);
	    wchar_t *wDest = get_converted_wcs(dest);
	    wchar_t *wMsg  = get_converted_wcs(msg);

	    DesktopToastsApp::SendBasicToast(
		get_message(wNick, L" @ ", wDest, L": ", wMsg));

	    free(wNick);
	    free(wDest);
	    free(wMsg);
#endif

	    if (ctx.window != g_active_window)
		broadcast_window_activity(ctx.window);
	} else {
	    printtext(&ctx, "%s%c%s%s%c%s %s",
		Theme("nick_s1"), c, COLOR2, nick, NORMAL, Theme("nick_s2"),
		msg);
	}
    }
}
