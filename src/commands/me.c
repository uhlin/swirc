#include "common.h"

#include "../irc.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "me.h"

/* usage: /me <message> */
void
cmd_me(const char *data)
{
    struct printtext_context ctx = {
	.window	    = g_active_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    if (Strings_match(data, "")) {
	printtext(&ctx, "/me: missing arguments");
    } else if (strings_match_ignore_case(g_active_window->label,
					 g_status_window_label)) {
	printtext(&ctx, "/me: cannot send to status window!");
    } else {
	ctx.spec_type = TYPE_SPEC_NONE;

	if (net_send("PRIVMSG %s :\001ACTION %s\001",
	    g_active_window->label, data) < 0)
	    g_on_air = false;
	else
	    printtext(&ctx, " - %s %s", g_my_nickname, data);
    }
}
