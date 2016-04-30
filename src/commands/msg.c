#include "common.h"

#include "../dataClassify.h"
#include "../irc.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "msg.h"

/* usage: /msg <recipient> <message> */
void
cmd_msg(const char *data)
{
    char *dcopy = sw_strdup(data);
    char *recipient, *message;
    char *state = "";
    struct printtext_context ctx = {
	.window	    = g_active_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    if (Strings_match(dcopy, "")
	|| Strfeed(dcopy, 1) != 1
	|| (recipient = strtok_r(dcopy, "\n", &state)) == NULL
	|| (message = strtok_r(NULL, "\n", &state)) == NULL) {
	printtext(&ctx, "/msg: missing arguments");
	free(dcopy);
	return;
    } else if (!is_valid_nickname(recipient) && !is_irc_channel(recipient)) {
	printtext(&ctx, "/msg: neither a nickname or irc channel");
	free(dcopy);
	return;
    } else {
	if (window_by_label(recipient) == NULL && spawn_chat_window(recipient, "") != 0) {
	    printtext(&ctx, "/msg: fatal: cannot spawn chat window!");
	    free(dcopy);
	    return;
	} else {
	    ctx.window	  = window_by_label(recipient);
	    ctx.spec_type = TYPE_SPEC_NONE;
	    if (net_send("PRIVMSG %s :%s", recipient, message) < 0)
		g_on_air = false;
	    else
		printtext(&ctx, "%s%s%s %s", Theme("nick_s1"), g_my_nickname, Theme("nick_s2"), message);
	    free(dcopy);
	}
    }
}
