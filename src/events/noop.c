#include "common.h"

#include "../irc.h"
#include "../printtext.h"

#include "noop.h"

void
event_noop(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC3, true);
    printtext(&ctx, "Got event %s. Currently a noop (aka no operation)",
	      compo->command);
    printtext(&ctx, "params = %s", compo->params);
    printtext(&ctx, "prefix = %s", compo->prefix ? compo->prefix : "none");
}
