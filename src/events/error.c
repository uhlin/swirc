/* Copyright (C) 2014, 2016 Markus Uhlin. All rights reserved. */

#include "common.h"

#include "../irc.h"
#include "../network.h"
#include "../printtext.h"

#include "error.h"

void
event_error(struct irc_message_compo *compo)
{
    char *cp = &compo->params[0];

    if (*cp == ':') {
	cp++;
    }

    if (*cp) {
	struct printtext_context ctx = {
	    .window     = g_active_window,
	    .spec_type  = TYPE_SPEC1,
	    .include_ts = true,
	};

	printtext(&ctx, "%s", cp);
    }

    g_on_air = false;
}
