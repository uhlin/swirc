#include "common.h"

#include "../config.h"
#include "../irc.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "cap.h"

bool
is_sasl_mechanism_supported(const char *mechanism)
{
    return (mechanism && Strings_match(mechanism, "PLAIN"));
}

const char *
get_sasl_mechanism(void)
{
    const char *mechanism = Config("sasl_mechanism");

    return (Strings_match(mechanism, "") ? "PLAIN" : mechanism);
}

void
event_cap(struct irc_message_compo *compo)
{
    struct printtext_context ctx = {
	.window	    = g_status_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    squeeze(compo->params, ":");

    if (strstr(compo->params, "ACK sasl")) {
	const char *mechanism = get_sasl_mechanism();

	if (is_sasl_mechanism_supported(mechanism)) {
	    net_send("AUTHENTICATE %s", mechanism);
	    return;
	}
    }

    net_send("CAP END");
    printtext(&ctx, "SASL exchange aborted!");
}
