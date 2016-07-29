#include "common.h"

#include "../io-loop.h"
#include "../printtext.h"
#include "../strHand.h"

#include "say.h"

/* usage: /say <message> */
void
cmd_say(const char *data)
{
    struct printtext_context ptext_ctx = {
	.window	    = g_active_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    if (Strings_match(data, "")) {
	printtext(&ptext_ctx, "/say: missing arguments");
    } else if (Strings_match_ignore_case(g_active_window->label, g_status_window_label)) {
	printtext(&ptext_ctx, "/say: cannot say to status window");
    } else {
	transmit_user_input(g_active_window->label, data);
    }
}
