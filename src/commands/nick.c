#include "common.h"

#include "../dataClassify.h"
#include "../irc.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "nick.h"

/* usage: /nick <new nickname> */
void
cmd_nick(const char *data)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);

    if (strings_match(data, "")) {
	printtext(&ctx, "/nick: missing arguments");
    } else if (!is_valid_nickname(data)) {
	printtext(&ctx, "/nick: bogus nickname");
    } else if (strings_match_ignore_case(g_my_nickname, data)) {
	printtext(&ctx, "/nick: no change");
    } else {
	if (net_send("NICK %s", data) < 0)
	    g_on_air = false;
    }
}
