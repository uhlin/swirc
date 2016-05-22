#include "common.h"

#include "../dataClassify.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "topic.h"

/* usage: /topic [new topic] */
void
cmd_topic(const char *data)
{
    struct printtext_context ctx = {
	.window	    = g_active_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    if (Strings_match(data, "") && is_irc_channel(g_active_window->label)) {
	if (net_send("TOPIC %s", g_active_window->label) < 0)
	    g_on_air = false;
    } else if (!Strings_match(data, "") && is_irc_channel(g_active_window->label)) {
	if (net_send("TOPIC %s :%s", g_active_window->label, data) < 0)
	    g_on_air = false;
    } else {
	printtext(&ctx, "/topic: switch to a channel in order to set a new topic for it");
    }
}
