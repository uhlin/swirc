#include "common.h"

#include "../dataClassify.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "topic.h"

#define ACTWINLABEL g_active_window->label

/* usage: /topic [new topic] */
void
cmd_topic(const char *data)
{
    if (strings_match(data, "") && is_irc_channel(ACTWINLABEL)) {
	if (net_send("TOPIC %s", ACTWINLABEL) < 0)
	    g_on_air = false;
    } else if (!strings_match(data, "") && is_irc_channel(ACTWINLABEL)) {
	if (net_send("TOPIC %s :%s", ACTWINLABEL, data) < 0)
	    g_on_air = false;
    } else {
	PRINTTEXT_CONTEXT ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "/topic: "
	    "switch to a channel in order to set a new topic for it");
    }
}
