#include "common.h"

#include "../assertAPI.h"
#include "../irc.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "privmsg.h"

struct special_msg_context {
    char *nick;
    char *user;
    char *host;
    char *dest;
    char *msg;
};

static void
handle_special_msg(const struct special_msg_context *ctx)
{
    char *msg = sw_strdup(ctx->msg);
    struct printtext_context pt_ctx;

    squeeze(msg, "\001");
    msg = trim(msg);

    if (Strings_match_ignore_case(ctx->dest, g_my_nickname)) {
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
	if (net_send("PRIVMSG %s :\001VERSION Swirc %s by %s\001",
		     ctx->nick, g_swircVersion, g_swircAuthor) < 0)
	    g_on_air = false;
	pt_ctx.spec_type = TYPE_SPEC3;
	printtext(&pt_ctx, "%c%s%c %s%s@%s%s requested CTCP VERSION form %c%s%c",
		  BOLD, ctx->nick, BOLD, LEFT_BRKT, ctx->user, ctx->host, RIGHT_BRKT,
		  BOLD, ctx->dest, BOLD);
    } else {
	/* do nothing */;
    }

    free(msg);
}

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
    struct printtext_context ctx = {
	.window	    = NULL,
	.spec_type  = TYPE_SPEC_NONE,
	.include_ts = true,
    };

    state1 = state2 = "";

    if (!prefix)
	return;
    if (*prefix == ':')
	prefix++;
    if ((nick = strtok_r(prefix, "!@", &state1)) == NULL
	|| (user = strtok_r(NULL, "!@", &state1)) == NULL
	|| (host = strtok_r(NULL, "!@", &state1)) == NULL)
	return;
    if (Strfeed(params, 1) != 1
	|| (dest = strtok_r(params, "\n", &state2)) == NULL
	|| (msg = strtok_r(NULL, "\n", &state2)) == NULL)
	return;
    if (*msg == ':')
	msg++;
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
    }
    if (Strings_match_ignore_case(dest, g_my_nickname)) {
	if (window_by_label(nick) == NULL && spawn_chat_window(nick, "") != 0)
	    return;
    } else {
	if (window_by_label(dest) == NULL && spawn_chat_window(dest, "") != 0)
	    return;
    }

    ctx.window = window_by_label(Strings_match_ignore_case(dest, g_my_nickname) ? nick : dest);
    sw_assert(ctx.window != NULL);
    printtext(&ctx, "%s%s%s %s", Theme("nick_s1"), nick, Theme("nick_s2"), msg);
}
