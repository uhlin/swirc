#include "common.h"

#include "../dataClassify.h"
#include "../icb.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "topic.h"

/* usage: /topic [new topic] */
void
cmd_topic(const char *data)
{
    if (strings_match(data, "") && is_irc_channel(ACTWINLABEL)) {
	if (g_icb_mode)
	    icb_send_topic("");
	else
	    (void) net_send("TOPIC %s", ACTWINLABEL);
    } else if (!strings_match(data, "") && is_irc_channel(ACTWINLABEL)) {
	if (g_icb_mode)
	    icb_send_topic(data);
	else
	    (void) net_send("TOPIC %s :%s", ACTWINLABEL, data);
    } else {
	PRINTTEXT_CONTEXT ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "/topic: "
	    "switch to a channel in order to set a new topic for it");
    }
}
