#include "common.h"

#include "../irc.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "say.h"

/* usage: /say <message> */
void
cmd_say(const char *data)
{
    struct printtext_context ptext_ctx = {
	.window	    = g_active_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    if (Strings_match(data, "")) {
	printtext(&ptext_ctx, "/say: missing arguments");
    } else if (Strings_match_ignore_case(g_active_window->label, g_status_window_label)) {
	printtext(&ptext_ctx, "/say: cannot say to status window");
    } else {
	ptext_ctx.spec_type = TYPE_SPEC_NONE;

	if (net_send("PRIVMSG %s :%s", g_active_window->label, data) < 0)
	    g_on_air = false;
	else
	    printtext(&ptext_ctx, "%s%s%s %s", Theme("nick_s1"), g_my_nickname, Theme("nick_s2"), data);
    }
}
