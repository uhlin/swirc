/* Copyright (C) 2014-2019 Markus Uhlin. All rights reserved. */

#include "common.h"

#include "../irc.h"
#include "../network.h"
#include "../printtext.h"

#include "error.h"

void
event_error(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;
    char *cp = &compo->params[0];

    if (*cp == ':') {
	cp++;
    }

    if (*cp) {
	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);
	printtext(&ctx, "%s", cp);
    }

    //g_connection_lost = true;
}
