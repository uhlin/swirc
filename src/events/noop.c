#include "common.h"

#include "../irc.h"
#include "../printtext.h"

#include "noop.h"

void
event_noop(struct irc_message_compo *compo)
{
    struct printtext_context ctx = {
	.window     = g_status_window,
	.spec_type  = TYPE_SPEC3,
	.include_ts = true,
    };

    printtext(&ctx, "Got event %s. Currently a noop (aka no operation)",
	      compo->command);
    printtext(&ctx, "params = %s", compo->params);
    printtext(&ctx, "prefix = %s", compo->prefix ? compo->prefix : "none");
}
