#include "common.h"

#include "../irc.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "me.h"

#define ACTWINLABEL g_active_window->label

/* usage: /me <message> */
void
cmd_me(const char *data)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);

    if (strings_match(data, "")) {
	printtext(&ctx, "/me: missing arguments");
    } else if (strings_match_ignore_case(ACTWINLABEL, g_status_window_label)) {
	printtext(&ctx, "/me: cannot send to status window!");
    } else {
	ctx.spec_type = TYPE_SPEC_NONE;

	if (net_send("PRIVMSG %s :\001ACTION %s\001", ACTWINLABEL, data) < 0)
	    g_on_air = false;
	else
	    printtext(&ctx, " - %s %s", g_my_nickname, data);
    }
}
