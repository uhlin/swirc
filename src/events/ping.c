/* Copyright (C) 2014, 2016 Markus Uhlin. All rights reserved. */

#include "common.h"

#include "../config.h"
#include "../irc.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "ping.h"

void
event_ping(struct irc_message_compo *compo)
{
    char *cp = &compo->params[0];
    int n_sent;

    if (*cp == ':') {
	cp++;
    }

    if (strings_match(cp, "")) {
	return;
    } else if ((n_sent = net_send("PONG %s", cp)) == -1) {
	g_on_air = false;
    } else if (n_sent > 0 && config_bool_unparse("show_ping_pong", true)) {
	PRINTTEXT_CONTEXT ctx;

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC_NONE, true);
	printtext(&ctx, "PING? PONG!");
    } else {
	/* do nothing */;
    }
}
