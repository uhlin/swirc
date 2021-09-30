#include "common.h"

#include "../io-loop.h"
#include "../printtext.h"
#include "../strHand.h"

#include "say.h"

/*
 * usage: /say <message>
 */
void
cmd_say(const char *data)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);

	if (strings_match(data, ""))
		printtext(&ctx, "/say: missing arguments");
	else if (strings_match_ignore_case(ACTWINLABEL, g_status_window_label))
		printtext(&ctx, "/say: cannot say to status window");
	else
		transmit_user_input(ACTWINLABEL, data);
}
