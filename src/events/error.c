/* Copyright (C) 2014-2021 Markus Uhlin. All rights reserved. */

#include "common.h"

#include "../irc.h"
#include "../printtext.h"

#include "error.h"

void
event_error(struct irc_message_compo *compo)
{
	char	*cp;

	if (*(cp = &compo->params[0]) == ':')
		cp++;
	if (*cp) {
		PRINTTEXT_CONTEXT	ctx;

		printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);
		printtext(&ctx, "%s", cp);
	}
}
