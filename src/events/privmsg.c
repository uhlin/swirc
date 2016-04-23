#include "common.h"

#include "../assertAPI.h"
#include "../irc.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "privmsg.h"

static void
handle_special_msg()
{
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
    if (Strings_match_ignore_case(dest, g_my_nickname)) {
	if (window_by_label(nick) == NULL && spawn_chat_window(nick, "") != 0)
	    return;
    } else {
	if (window_by_label(dest) == NULL && spawn_chat_window(dest, "") != 0)
	    return;
    }

    ctx.window = window_by_label(Strings_match_ignore_case(dest, g_my_nickname) ? nick : dest);
    sw_assert(ctx.window != NULL);

    if (*msg == '\001') {
	handle_special_msg();
	return;
    } else {
	printtext(&ctx, "%s%s%s %s", Theme("nick_s1"), nick, Theme("nick_s2"), msg);
    }
}
