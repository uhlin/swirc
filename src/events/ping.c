/* Copyright (C) 2014-2021 Markus Uhlin. All rights reserved. */

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
	char	*cp;
	int	 n_sent;

	if (*(cp = &compo->params[0]) == ':')
		cp++;

	if (strings_match(cp, "")) {
		return;
	} else if ((n_sent = net_send("PONG %s", cp)) == -1) {
		g_connection_lost = true;
	} else if (n_sent > 0 && config_bool("show_ping_pong", true)) {
		PRINTTEXT_CONTEXT	ctx;

		printtext_context_init(&ctx, g_status_window, TYPE_SPEC_NONE,
		    true);
		printtext(&ctx, "PING? PONG!");
	}
}
