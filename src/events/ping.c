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
#if defined(UNIX)
    ssize_t n_sent;
#elif defined(WIN32)
    int n_sent;
#endif

    if (*cp == ':') {
	cp++;
    }

    if (Strings_match(cp, "")) {
	return;
    } else if ((n_sent = net_send(g_socket, 0, "PONG %s", cp)) == -1) {
	g_on_air = false;
    } else if (n_sent > 0 && config_bool_unparse("show_ping_pong", true)) {
	struct printtext_context ctx = {
	    .window     = g_status_window,
	    .spec_type  = TYPE_SPEC_NONE,
	    .include_ts = true,
	};

	printtext(&ctx, "PING? PONG!");
    } else {
	/* do nothing */;
    }
}
