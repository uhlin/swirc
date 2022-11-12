#include "common.h"

#include "../dataClassify.h"
#include "../icb.h"
#include "../irc.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "nick.h"

/*
 * usage: /nick <new nickname>
 */
void
cmd_nick(const char *data)
{
	PRINTTEXT_CONTEXT	ctx;
	static const char	cmd[] = "/nick";

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);

	if (strings_match(data, ""))
		printtext(&ctx, "%s: missing arguments", cmd);
	else if (!is_valid_nickname(data))
		printtext(&ctx, "%s: bogus nickname", cmd);
	else if (strings_match_ignore_case(g_my_nickname, data))
		printtext(&ctx, "%s: no change", cmd);
	else if (g_icb_mode)
		icb_send_name(data);
	else
		(void) net_send("NICK %s", data);
}
