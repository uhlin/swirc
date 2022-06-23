#include "common.h"

#include "../errHand.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "cap.h"

/*
 * usage: /cap [ls | list]
 */
void
cmd_cap(const char *data)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	if (strings_match(data, "")) {
		if (net_send("CAP LS") < 0 || net_send("CAP LIST") < 0)
			err_log(ENOTCONN, "/cap");
	} else if (strings_match(data, "ls") || strings_match(data, "LS")) {
		(void) net_send("CAP LS 302");
	} else if (strings_match(data, "list") || strings_match(data, "LIST")) {
		(void) net_send("CAP LIST");
	} else {
		ctx.spec_type = TYPE_SPEC1_FAILURE;
		printtext(&ctx, "what? ls or list?");
	}
}
